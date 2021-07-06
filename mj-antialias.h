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

#ifndef MJ_ANTIALIAS_H
#define MJ_ANTIALIAS_H 1

#include "mj-surface.h"
#include "mj-color.h"
#include "mj-calc.h"

template<typename T>
int mj_antialias(MJ_Surface<MJ_Color> const& output, MJ_Surface<double> const& input, MJ_ColorPalette const& palette,
                 T cx, T cy, double center_x, double center_y, double pixel_width, double threshold,
                 double period, int pass, int max_iter, int julia_mode)
{
    if (!pass) {
        for (int x = 0, y = 0; x < input.width(); x++)
            input(x,y) = (input(x,y) == MJ_INFINITY) ? 0.5 * input(x,y) : input(x,y);
        for (int x = 0, y = input.height() - 1; x < input.width(); x++)
            input(x,y) = (input(x,y) == MJ_INFINITY) ? 0.5 * input(x,y) : input(x,y);
        for (int x = 0, y = 1; y < input.height() - 1; y++)
            input(x,y) = (input(x,y) == MJ_INFINITY) ? 0.5 * input(x,y) : input(x,y);
        for (int x = input.width() - 1, y = 1; y < input.height() - 1; y++)
            input(x,y) = (input(x,y) == MJ_INFINITY) ? 0.5 * input(x,y) : input(x,y);

        for (int y = 1; y < input.height() - 1; y++)
            for (int x = 1; x < input.width() - 1; x++)
                output(x-1,y-1) = (input(x,y) == MJ_INFINITY) ?
                                  palette.infinity_color(0) : palette.color(input(x,y)/period, 0);
    }

    const int offset_x[8] = {
        -1, 0, 1,
        -1,    1,
        -1, 0, 1
    };

    const int offset_y[8] = {
        -1, -1, -1,
         0,      0,
         1,  1,  1
    };

    const double threshold_weight[8] = {
        1.3, 1.0, 1.3,
        1.0     , 1.0,
        1.3, 1.0, 1.3
    };

    const double antialias_step = 1.0/3.0;

    int modified = 0;

    MJ_Color antialias_buf[9];

    for (int y = 1; y < input.height() - 1; y++) {
        for (int x = 1; x < input.width() - 1; x++) {
            if (output(x-1,y-1).v[3] > 0.0f)
                continue;

            int need_antialias = 0;
            for (int k = 0; k < 8; k++) {
                if (fabs(input(x,y) - input(x + offset_x[k], y + offset_y[k])) >= threshold * threshold_weight[k]) {
                    need_antialias = 1;
                    break;
                }
            }

            if (!need_antialias) {
                if (input(x,y) < MJ_INFINITY)
                    output(x-1,y-1).v[3] = 1.0f;
                continue;
            }

            int is_infinity = 1;
            for (int k = 0; k < 8; k++) {
                double zx = (x - center_x + offset_x[k] * antialias_step) * pixel_width;
                double zy = (center_y - y - offset_y[k] * antialias_step) * pixel_width;
                double res = mj_calc_select(cx, cy, zx, zy, max_iter, julia_mode);
                if (res == MJ_INFINITY) {
                    antialias_buf[k] = palette.infinity_color(1);
                } else {
                    antialias_buf[k] = palette.color(res / period, 1);
                    is_infinity = 0;
                }
            }
            antialias_buf[8] = output(x-1,y-1);
            output(x-1,y-1) = mj_color_average(antialias_buf, 1, 9);
            if (input(x,y) == MJ_INFINITY && !is_infinity) {
                modified = 1;
                input(x,y) *= 0.5;
            }
        }
    }

    return modified;
}

#endif
