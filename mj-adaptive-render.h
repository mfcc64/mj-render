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

#ifndef MJ_ADAPTIVE_RENDER_H
#define MJ_ADAPTIVE_RENDER_H 1

#include "mj-surface.h"
#include "mj-calc.h"

template<typename T>
void mj_recursive_render(const MJ_Surface<double>& surface, T cx, T cy,
                         double center_x, double center_y, double pixel_width,
                         int left_x, int right_x, int top_y, int bottom_y,
                         int max_iter, int julia_mode)
{
    int width = right_x - left_x + 1;
    int height = bottom_y - top_y + 1;
    if (width <= 2 || height <= 2)
        return;

    int all_infinity = 1;

    for (int x = left_x; all_infinity && x <= right_x; x++)
        if (surface(x, top_y) < MJ_INFINITY || surface(x, bottom_y) < MJ_INFINITY)
            all_infinity = 0;

    for (int y = top_y + 1; all_infinity && y <= bottom_y - 1; y++)
        if (surface(left_x, y) < MJ_INFINITY || surface(right_x, y) < MJ_INFINITY)
            all_infinity = 0;

    if (all_infinity) {
        for (int y = top_y + 1; y <= bottom_y - 1; y++)
            for (int x = left_x + 1; x <= right_x - 1; x++)
                surface(x,y) = MJ_INFINITY;
        return;
    }

    if (width < height) {
        int middle_y = (top_y + bottom_y) / 2;
        double zy = (center_y - middle_y) * pixel_width;
        for (int x = left_x + 1; x <= right_x - 1; x++) {
            double zx = (x - center_x) * pixel_width;
            surface(x, middle_y) = mj_calc_select(cx, cy, zx, zy, max_iter, julia_mode);
        }

        mj_recursive_render(surface, cx, cy, center_x, center_y, pixel_width,
                            left_x, right_x, top_y, middle_y, max_iter, julia_mode);
        mj_recursive_render(surface, cx, cy, center_x, center_y, pixel_width,
                            left_x, right_x, middle_y, bottom_y, max_iter, julia_mode);
    } else {
        int middle_x = (left_x + right_x) / 2;
        double zx = (middle_x - center_x) * pixel_width;
        for (int y = top_y + 1; y <= bottom_y - 1; y++) {
            double zy = (center_y - y) * pixel_width;
            surface(middle_x, y) = mj_calc_select(cx, cy, zx, zy, max_iter, julia_mode);
        }

        mj_recursive_render(surface, cx, cy, center_x, center_y, pixel_width,
                            left_x, middle_x, top_y, bottom_y, max_iter, julia_mode);
        mj_recursive_render(surface, cx, cy, center_x, center_y, pixel_width,
                            middle_x, right_x, top_y, bottom_y, max_iter, julia_mode);
    }
}

template<typename T>
void mj_adaptive_render(const MJ_Surface<double>& surface, T cx, T cy,
                        double center_x, double center_y, double pixel_width,
                        int max_iter, int julia_mode)
{
    for (int x = 0; x < surface.width(); x++) {
        double zx = (x - center_x) * pixel_width;
        double zy = center_y * pixel_width;
        surface(x,0) = mj_calc_select(cx, cy, zx, zy, max_iter, julia_mode);

        int y = surface.height() - 1;
        zy = (center_y - y) * pixel_width;
        surface(x,y) = mj_calc_select(cx, cy, zx, zy, max_iter, julia_mode);
    }

    for (int y = 1; y < surface.height() - 1; y++) {
        double zx = -center_x * pixel_width;
        double zy = (center_y - y) * pixel_width;
        surface(0,y) = mj_calc_select(cx, cy, zx, zy, max_iter, julia_mode);

        int x = surface.width() - 1;
        zx = (x - center_x) * pixel_width;
        surface(x,y) = mj_calc_select(cx, cy, zx, zy, max_iter, julia_mode);
    }

    mj_recursive_render(surface, cx, cy, center_x, center_y, pixel_width,
                        0, surface.width() - 1, 0, surface.height() - 1,
                        max_iter, julia_mode);
}

#endif
