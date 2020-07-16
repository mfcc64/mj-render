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
#include <sys/time.h>
#include "mj-calc.h"
#include "mj-adaptive-render.h"
#include "mj-antialias.h"
#include "mj-parseval.h"
#include "mj-color.h"
#include "mj-png.h"

inline double mj_gettimeofday()
{
    timeval tbuf;
    gettimeofday(&tbuf, NULL);
    return tbuf.tv_sec + 1e-6 * tbuf.tv_usec;
}

template<typename T>
static void mj_render(MJ_Surface<MJ_Color> const& csurface, MJ_ColorPalette const& color, T cx, T cy, double pixel_width,
                      double antialias_threshold, double color_period, int max_iter, int is_julia)
{
    MJ_Surface<double> dsurface(csurface.width() + 2,
                                is_julia ? (csurface.height() + 1) / 2 + 2 :
                                csurface.height()+ 2);
    double center_x = 0.5 * (csurface.width() - 1) + 1;
    double center_y = 0.5 * (csurface.height() - 1) + 1;
    double last_time, current_time;

    last_time = mj_gettimeofday();
    fprintf(stderr, "Rendering       :");
    fflush(stderr);

    mj_adaptive_render(dsurface, cx, cy, center_x, center_y, pixel_width, max_iter, is_julia);

    current_time = mj_gettimeofday();
    fprintf(stderr, " complete in %8.3f seconds.\n", current_time - last_time);
    last_time = current_time;

    for (int pass = 0; ; pass++) {
        fprintf(stderr, "Antialiasing    :");
        fflush(stderr);

        int modified = mj_antialias(csurface, dsurface, color, cx, cy, center_x, center_y, pixel_width,
                                    antialias_threshold, color_period, pass, max_iter, is_julia);

        current_time = mj_gettimeofday();
        fprintf(stderr, " complete in %8.3f seconds.\n", current_time - last_time);
        last_time = current_time;

        if (!modified)
            break;
    }

    if (is_julia) {
        for (int y0 = 0, y1 = csurface.height() - 1; y0 < y1; y0++, y1--)
            for (int x = 0; x < csurface.width(); x++)
                csurface(x, y1) = csurface(csurface.width() - 1 - x, y0);
    }
}

static void print_help()
{
    fprintf(stderr,
    "Mandelbrot and Julia set renderer\n"
    "Usage:\n"
    "  mj-render [OPTIONS...]\n"
    "OPTIONS:\n"
    "  -o output.png\n"
    "  -w width\n"
    "  -h height\n"
    "  -i iteration\n"
    "  -v width view\n"
    "  -x center x\n"
    "  -y center y\n"
    "  -p color period\n"
    "  -t antialias threshold\n"
    "  -r radius of julia set (also switch to render julia set)\n"
    "  -a angle of julia set (also switch to render julia set)\n"
    "  -q computation bits (64, 80)\n"
    "  -b png bits (8, 16)\n");
}

int main(int argc, char **argv)
{
    try {
        const char *cx_str = "0.0";
        const char *cy_str = "0.0";
        int width = 640, height = 480;
        int max_iter = 1024;
        double width_view = 4.0;
        double color_period = 64.0;
        double radius = 0.0;
        double angle = 0.0;
        double antialias_threshold = 3.0;
        int is_julia = 0;
        int computation_bits = 64;
        int png_bits = 8;
        const char *filename = NULL;

        if ((argc - 1) % 2)
            throw "invalid argument";

        for (int k = 1; k < argc; k += 2) {
            if (argv[k][0] != '-' || !argv[k][1] || argv[k][2])
                throw "invalid argument";
            switch (argv[k][1]) {
            case 'w':
                width = mj_parseval<int>(argv[k+1], 16, 16384);
                break;
            case 'h':
                height = mj_parseval<int>(argv[k+1], 16, 16384);
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
                is_julia = 1;
                break;
            case 'a':
                angle = mj_parseval<double>(argv[k+1], -10000.0, 10000.0);
                is_julia =  1;
                break;
            case 'o':
                filename = argv[k+1];
                break;
            case 'q':
                computation_bits = mj_parseval<int>(argv[k+1], (const int[]){64, 80}, 2);
                break;
            case 'b':
                png_bits = mj_parseval<int>(argv[k+1], (const int[]){8, 16}, 2);
                break;
            default:
                throw "invalid argument";
            }
        }

        if (!filename)
            throw "no output file specified";

        double jx = radius * cos(angle);
        double jy = radius * sin(angle);

        MJ_ColorPalette color;
        MJ_Surface<MJ_Color> csurface(width, height);

        double last_time, current_time;
        last_time = mj_gettimeofday();

        switch (computation_bits) {
        case 64:
            mj_render(csurface, color, mj_parseval<double>(cx_str, -10000.0, 10000.0) + jx,
                      mj_parseval<double>(cx_str, -10000.0, 10000.0) + jy, width_view / width,
                      antialias_threshold, color_period, max_iter, is_julia);
            break;
        case 80:
            mj_render(csurface, color, mj_parseval<long double>(cx_str, -10000.0, 10000.0) + jx,
                      mj_parseval<long double>(cx_str, -10000.0, 10000.0) + jy, width_view / width,
                      antialias_threshold, color_period, max_iter, is_julia);
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
            mj_output_png<uint8_t>(csurface, filename);
            break;
        case 16:
            mj_output_png<uint16_t>(csurface, filename);
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
