/*
 * Copyright (C) 2020 Muhammad Faiz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <SDL2/SDL.h>
#include "mj-calc.h"
#include "mj-adaptive-render.h"
#include "mj-antialias.h"
#include "mj-parseval.h"
#include "mj-color.h"
#include "mj-f128.h"
#include "mj-fixed.h"
#include "mj-png.h"

inline double mj_gettimeofday()
{
    timeval tbuf;
    gettimeofday(&tbuf, NULL);
    return tbuf.tv_sec + 1e-6 * tbuf.tv_usec;
}

template<typename T>
static void mj_render(MJ_Surface<MJ_Color> const& csurface, MJ_ColorPalette const& color, T cx, T cy, double pixel_width,
                      double antialias_threshold, double color_period, int max_iter, int julia_mode)
{
    int is_sym = (julia_mode == MJ_JULIA_MODE_JULIA_AT_0 || julia_mode == MJ_JULIA_MODE_MANDELBROT_JULIA);
    is_sym = is_sym && (MJ_MANDELBROT_POWER % 2 == 0);
    MJ_Surface<double> dsurface(csurface.width() + 2,
                                is_sym ? (csurface.height() + 1) / 2 + 2 :
                                csurface.height()+ 2);
    double center_x = 0.5 * (csurface.width() - 1) + 1;
    double center_y = 0.5 * (csurface.height() - 1) + 1;
    double last_time, current_time;

    last_time = mj_gettimeofday();
    fprintf(stderr, "Rendering       :");
    fflush(stderr);

    mj_adaptive_render(dsurface, cx, cy, center_x, center_y, pixel_width, max_iter, julia_mode);

    current_time = mj_gettimeofday();
    fprintf(stderr, " complete in %8.3f seconds.\n", current_time - last_time);
    last_time = current_time;

    for (int pass = 0; ; pass++) {
        fprintf(stderr, "Antialiasing    :");
        fflush(stderr);

        int modified = mj_antialias(csurface, dsurface, color, cx, cy, center_x, center_y, pixel_width,
                                    antialias_threshold, color_period, pass, max_iter, julia_mode);

        current_time = mj_gettimeofday();
        fprintf(stderr, " complete in %8.3f seconds.\n", current_time - last_time);
        last_time = current_time;

        if (!modified)
            break;
    }

    if (is_sym) {
        for (int y0 = 0, y1 = csurface.height() - 1; y0 < y1; y0++, y1--)
            for (int x = 0; x < csurface.width(); x++)
                csurface(x, y1) = csurface(csurface.width() - 1 - x, y0);
    }
}

template<typename T>
static void mj_preview(MJ_Surface<MJ_Color> const& csurface, MJ_ColorPalette const& color, T cx, T cy, double pixel_width,
                       double antialias_threshold, double color_period, int max_iter, int julia_mode)
{
    if (SDL_Init(SDL_INIT_VIDEO) == (-1))
        throw SDL_GetError();

    int is_locked = 0;
    SDL_Window *window = SDL_CreateWindow("mj-render-preview", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          csurface.width(), csurface.height(), 0);
    if (!window)
        throw SDL_GetError();

    SDL_Surface *surface = SDL_GetWindowSurface(window);

    for ( ; ; ) {
        const char *julia_mode_name = "unknown";
        switch (julia_mode) {
        case MJ_JULIA_MODE_MANDELBROT:
            julia_mode_name = is_locked ? "mandelbrot (locked)" : "mandelbrot (unlocked)";
            break;
        case MJ_JULIA_MODE_JULIA_AT_C:
            julia_mode_name = "julia at c";
            break;
        case MJ_JULIA_MODE_JULIA_AT_0:
            julia_mode_name = "julia at 0";
            break;
        case MJ_JULIA_MODE_MANDELBROT_JULIA:
            julia_mode_name = "mandelbrot julia";
            break;
        }

        SDL_FillRect(surface, 0, 0);
        SDL_UpdateWindowSurface(window);
        fprintf(stderr, "===============================================\n");
        mj_render(csurface, color, cx, cy, pixel_width, antialias_threshold,
                  color_period, max_iter, julia_mode);
        fprintf(stderr, "type = %s\n", julia_mode_name);
        fprintf(stderr, "x    = "); mj_printval(stderr, cx); fprintf(stderr, "\n");
        fprintf(stderr, "y    = "); mj_printval(stderr, cy); fprintf(stderr, "\n");
        fprintf(stderr, "w    = %d\n", csurface.width());
        fprintf(stderr, "h    = %d\n", csurface.height());
        fprintf(stderr, "v    = %.13e\n", pixel_width * csurface.width());
        fprintf(stderr, "t    = %.6f\n", antialias_threshold);
        fprintf(stderr, "p    = %.6f\n", color_period);
        fprintf(stderr, "i    = %d\n", max_iter);
        fprintf(stderr, "===============================================\n");

        uint32_t *line = (uint32_t *) surface->pixels;
        int line_width = surface->pitch / sizeof(*line);
        for (int y = 0; y < csurface.height(); y++, line += line_width)
            for (int x  = 0; x < csurface.width(); x++)
                line[x] = SDL_MapRGB(surface->format, lrintf(csurface(x,y).v[0] * 255.0f),
                                     lrintf(csurface(x,y).v[1] * 255.0f), lrintf(csurface(x,y).v[2] * 255.0f));

        SDL_Event event;
        while (SDL_PollEvent(&event))
            ;
        for (int event_processed = 0 ; !event_processed; ) {
            SDL_Delay(50);
            SDL_UpdateWindowSurface(window);
            while (!event_processed && SDL_PollEvent(&event)) {
                double mul = 0.0;
                if (event.type == SDL_QUIT) {
                    SDL_Quit();
                    return;
                }
                if (event.type != SDL_KEYUP)
                    continue;

                switch (event.key.keysym.sym) {
                case SDLK_1:
                    mul = 16.0;
                    break;
                case SDLK_2:
                    mul = 4.0;
                    break;
                case SDLK_3:
                    mul = 2.0;
                    break;
                case SDLK_4:
                    mul = sqrt(2.0);
                    break;
                case SDLK_5:
                    mul = 1.0;
                    break;
                case SDLK_6:
                    mul = 1.0/sqrt(sqrt(2.0));
                    break;
                case SDLK_7:
                    mul = 1.0/sqrt(2.0);
                    break;
                case SDLK_8:
                    mul = 1.0/2.0;
                    break;
                case SDLK_9:
                    mul = 1.0/4.0;
                    break;
                case SDLK_0:
                    mul = 1.0/16.0;
                    break;
                case SDLK_a:
                    mul = -1.0;
                    break;
                case SDLK_s:
                    mul = -2.0;
                    break;
                case SDLK_d:
                    mul = -3.0;
                    break;
                case SDLK_f:
                    mul = -4.0;
                    break;
                case SDLK_g:
                    mul = -5.0;
                    break;
                case SDLK_h:
                    mul = -6.0;
                    break;
                case SDLK_m:
                    mul = MJ_JULIA_MODE_MANDELBROT - 100.0;
                    break;
                case SDLK_j:
                    mul = MJ_JULIA_MODE_JULIA_AT_0 - 100.0;
                    break;
                case SDLK_k:
                    mul = MJ_JULIA_MODE_JULIA_AT_C - 100.0;
                    break;
                case SDLK_n:
                    mul = MJ_JULIA_MODE_MANDELBROT_JULIA - 100.0;
                    break;
                case SDLK_l:
                    mul = -1000.0;
                    break;
                case SDLK_ESCAPE:
                    SDL_Quit();
                    return;
                }

                if (mul != 0.0)
                    event_processed = 1;

                if (mul > 0.0) {
                    if (julia_mode == MJ_JULIA_MODE_MANDELBROT && mul <= 1.0 && !is_locked) {
                        int mx, my;
                        SDL_GetMouseState(&mx, &my);
                        mx = mx - csurface.width()/2;
                        my = csurface.height()/2 - my;
                        cx = cx + T(mx * pixel_width);
                        cy = cy + T(my * pixel_width);
                    }
                    pixel_width *= mul;
                }

                if (mul == -1.0)
                    max_iter = (max_iter > 8*1024*1024) ? 16*1024*1024 : 2*max_iter;

                if (mul == -2.0)
                    max_iter = (max_iter < 512) ? 256 : max_iter/2;

                if (mul == -3.0)
                    color_period = (color_period > 8192.0) ? 16384.0 : 2.0*color_period;

                if (mul == -4.0)
                    color_period = (color_period < 2.0) ? 1.0 : 0.5*color_period;

                if (mul == -5.0)
                    antialias_threshold = (antialias_threshold > 4096.0) ? 8192.0 : 2.0*antialias_threshold;

                if (mul == -6.0)
                    antialias_threshold = (antialias_threshold < 0.125) ? 0.06125 : 0.5*antialias_threshold;

                if (mul == -1000.0)
                    is_locked = !is_locked;

                if (mul == MJ_JULIA_MODE_MANDELBROT - 100.0 || mul == MJ_JULIA_MODE_JULIA_AT_0 - 100.0 ||
                    mul == MJ_JULIA_MODE_JULIA_AT_C - 100.0 || mul == MJ_JULIA_MODE_MANDELBROT_JULIA - 100.0) {
                    int new_mode = int(mul + 100.0);
                    if ((new_mode == MJ_JULIA_MODE_MANDELBROT || new_mode == MJ_JULIA_MODE_JULIA_AT_C) &&
                        (julia_mode == MJ_JULIA_MODE_JULIA_AT_0 || julia_mode == MJ_JULIA_MODE_MANDELBROT_JULIA))
                        pixel_width = pow(pixel_width * 0.25 * csurface.width(), MJ_MANDELBROT_POWER) / (0.25 * csurface.width());
                    if ((julia_mode == MJ_JULIA_MODE_MANDELBROT || julia_mode == MJ_JULIA_MODE_JULIA_AT_C) &&
                        (new_mode == MJ_JULIA_MODE_JULIA_AT_0 || new_mode == MJ_JULIA_MODE_MANDELBROT_JULIA))
                        pixel_width = pow(pixel_width * 0.25 * csurface.width(), 1.0/MJ_MANDELBROT_POWER) / (0.25 * csurface.width());
                    julia_mode = new_mode;
                }
            }
        }
    }
}

static void print_help()
{
    fprintf(stderr,
    "Mandelbrot and Julia set renderer\n"
    "Usage:\n"
    "  mj-render [OPTIONS...]\n"
    "OPTIONS:\n"
    "  -o output.png/preview\n"
    "  -w width\n"
    "  -h height\n"
    "  -i iteration\n"
    "  -v width view\n"
    "  -x center x\n"
    "  -y center y\n"
    "  -p color period\n"
    "  -t antialias threshold\n"
    "  -m global multisample antialias\n"
    "  -r radius of julia set (also switch to render julia-at-0)\n"
    "  -a angle of julia set (also switch to render julia-at-0)\n"
    "  -q computation bits (64, 80)\n"
    "  -b png bits (8, 16)\n"
    "  -j julia mode (julia-at-c, julia-at-0, mandelbrot-julia)\n");
}

int main(int argc, char **argv)
{
    try {
        const char *cx_str = "0";
        const char *cy_str = "0";
        int width = 640, height = 480;
        int max_iter = 1024;
        double width_view = 4.0;
        double color_period = 64.0;
        double radius = 0.0;
        double angle = 0.0;
        double antialias_threshold = 3.0;
        int julia_mode = MJ_JULIA_MODE_MANDELBROT;
        int computation_bits = 64;
        int png_bits = 8;
        int multisample = 1;
        double color_offset = 0.0;
        const char *filename = NULL;
        const char *palette_filename = NULL;

        if ((argc - 1) % 2)
            throw "invalid argument";

        for (int k = 1; k < argc; k += 2) {
            if (argv[k][0] != '-' || !argv[k][1] || argv[k][2])
                throw "invalid argument";
            switch (argv[k][1]) {
            case 'w':
                width = mj_parseval<int>(argv[k+1], 16, 8192);
                break;
            case 'h':
                height = mj_parseval<int>(argv[k+1], 16, 8192);
                break;
            case 'i':
                max_iter = mj_parseval<int>(argv[k+1], 16, 1024*1024*16);
                break;
            case 'v':
                width_view = mj_parseval<double>(argv[k+1], 1.0e-100, 10000.0);
                break;
            case 'x':
                cx_str = argv[k+1];
                break;
            case 'y':
                cy_str = argv[k+1];
                break;
            case 'p':
                color_period = mj_parseval<double>(argv[k+1], 1.0, 65536.0);
                break;
            case 't':
                antialias_threshold = mj_parseval<double>(argv[k+1], 0.0, 1.0e100);
                break;
            case 'r':
                radius = mj_parseval<double>(argv[k+1], -10000.0, 10000.0);
                if (julia_mode == MJ_JULIA_MODE_MANDELBROT)
                    julia_mode = MJ_JULIA_MODE_JULIA_AT_0;
                break;
            case 'a':
                angle = mj_parseval<double>(argv[k+1], -10000.0, 10000.0);
                if (julia_mode == MJ_JULIA_MODE_MANDELBROT)
                    julia_mode = MJ_JULIA_MODE_JULIA_AT_0;
                break;
            case 'o':
                filename = argv[k+1];
                break;
            case 'q':
                computation_bits = mj_parseval<int>(argv[k+1], (const int[]){64, 80, 128, 256, 384, 512, 768, 1024}, 8);
                break;
            case 'b':
                png_bits = mj_parseval<int>(argv[k+1], (const int[]){8, 16}, 2);
                break;
            case 'm':
                multisample = mj_parseval<int>(argv[k+1], 1, 3);
                break;
            case 'c':
                palette_filename = argv[k+1];
                break;
            case 'C':
                color_offset = mj_parseval<double>(argv[k+1], 0.0, 1.0);
                break;
            case 'j': {
                    const char *str_list[] = {
                        "julia-at-c",
                        "julia-at-0",
                        "mandelbrot-julia"
                    };
                    int list[] = {
                        MJ_JULIA_MODE_JULIA_AT_C,
                        MJ_JULIA_MODE_JULIA_AT_0,
                        MJ_JULIA_MODE_MANDELBROT_JULIA
                    };
                    julia_mode = mj_parseval<int>(argv[k+1], str_list, list, 3);
                }
                break;
            default:
                throw "invalid argument";
            }
        }

        if (!filename)
            throw "no output file specified";

        double jx = radius * cos(angle);
        double jy = radius * sin(angle);
        int is_preview = !strcmp(filename, "preview");

        width = is_preview ? width : width * multisample;
        height = is_preview ? height : height * multisample;

        MJ_ColorPalette color(palette_filename, color_offset);
        MJ_Surface<MJ_Color> csurface(width, height);

#define MJ_PREVIEW_SELECT(type)                                                 \
    mj_preview(csurface, color, mj_parseval(cx_str, (type)0) + (type)jx,        \
               mj_parseval(cy_str, (type)0) + (type)jy, width_view / width,     \
               antialias_threshold, color_period, max_iter, julia_mode)

        if (is_preview) {
            switch (computation_bits) {
            case 64:
                MJ_PREVIEW_SELECT(double);
                break;
            case 80:
                MJ_PREVIEW_SELECT(long double);
                break;
            case 128:
                MJ_PREVIEW_SELECT(MJ_F128);
                break;
            case 256:
                MJ_PREVIEW_SELECT(MJ_Fixed<256>);
                break;
            case 384:
                MJ_PREVIEW_SELECT(MJ_Fixed<384>);
                break;
            case 512:
                MJ_PREVIEW_SELECT(MJ_Fixed<512>);
                break;
            case 768:
                MJ_PREVIEW_SELECT(MJ_Fixed<768>);
                break;
            case 1024:
                MJ_PREVIEW_SELECT(MJ_Fixed<1024>);
                break;
            default:
                throw "unreached";
            }
            return EXIT_SUCCESS;
        }

        double last_time, current_time;
        last_time = mj_gettimeofday();

#define MJ_RENDER_SELECT(type)                                                  \
    mj_render(csurface, color, mj_parseval(cx_str, (type)0) + (type)jx,         \
              mj_parseval(cy_str, (type)0) + (type)jy, width_view / width,      \
              antialias_threshold, color_period, max_iter, julia_mode)

        switch (computation_bits) {
        case 64:
            MJ_RENDER_SELECT(double);
            break;
        case 80:
            MJ_RENDER_SELECT(long double);
            break;
        case 128:
            MJ_RENDER_SELECT(MJ_F128);
            break;
        case 256:
            MJ_RENDER_SELECT(MJ_Fixed<256>);
            break;
        case 384:
            MJ_RENDER_SELECT(MJ_Fixed<384>);
            break;
        case 512:
            MJ_RENDER_SELECT(MJ_Fixed<512>);
            break;
        case 768:
            MJ_RENDER_SELECT(MJ_Fixed<768>);
            break;
        case 1024:
            MJ_RENDER_SELECT(MJ_Fixed<1024>);
            break;
        default:
            throw "unreached";
        }

        current_time = mj_gettimeofday();
        fprintf(stderr, "===============================================\n");
        fprintf(stderr, "Total Rendering : complete in %8.3f seconds.\n", current_time - last_time);
        last_time = current_time;

        fprintf(stderr, "Outputting      :");
        fflush(stderr);

        switch (png_bits) {
        case 8:
            mj_output_png<uint8_t>(csurface, filename, multisample);
            break;
        case 16:
            mj_output_png<uint16_t>(csurface, filename, multisample);
            break;
        default:
            throw "unreached";
        }

        current_time = mj_gettimeofday();
        fprintf(stderr, " complete in %8.3f seconds.\n", current_time - last_time);
        return EXIT_SUCCESS;
    } catch (const char *msg) {
        print_help();
        fprintf(stderr, "Error: %s\n", msg);
        return EXIT_FAILURE;
    }
}
