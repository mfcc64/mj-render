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

#ifndef MJ_PNG_H
#define MJ_PNG_H 1

#include <stdint.h>
#include <typeinfo>
#include <png.h>
#include "mj-surface.h"
#include "mj-color.h"

template<typename T>
void mj_output_png(MJ_Surface<MJ_Color> const& surface, const char *filename)
{
    float multiplier = 0.0f;
    FILE *fp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    T *line = NULL;

    try {
        if (typeid(T) == typeid(uint8_t))
            multiplier = 255.0f;

        if (typeid(T) == typeid(uint16_t))
            multiplier = 65536.0f;

        if (multiplier == 0.0f)
            throw "invalid type of mj_output_png";

        fp = fopen(filename, "wb");
        if (!fp)
            throw "mj_output_png cannot open file";

        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr)
            throw "png error of mj_output_png";

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
            throw "png error of mj_output_png";

        if (setjmp(png_jmpbuf(png_ptr)))
            throw "png errot of mj_output_png";

        png_init_io(png_ptr, fp);
        png_set_filter(png_ptr, 0, PNG_ALL_FILTERS);
        png_set_IHDR(png_ptr, info_ptr, surface.width(), surface.height(), 8 * sizeof(T),
                     PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                     PNG_FILTER_TYPE_DEFAULT);
        png_set_gAMA(png_ptr, info_ptr, 0.45455);
        png_write_info(png_ptr, info_ptr);
        png_set_swap(png_ptr);
        line = new T[surface.width() * 3];

        for (int y = 0; y < surface.height(); y++) {
            for (int x = 0; x < surface.width(); x++) {
                line[3*x+0] = lrintf(multiplier * surface(x,y).v[0]);
                line[3*x+1] = lrintf(multiplier * surface(x,y).v[1]);
                line[3*x+2] = lrintf(multiplier * surface(x,y).v[2]);
            }
            png_write_row(png_ptr, (png_bytep) line);
        }

        png_write_end(png_ptr, NULL);
        delete[] line, line = NULL;
        png_destroy_info_struct(png_ptr, &info_ptr);
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp), fp = NULL;
    } catch (...) {
        if (info_ptr)
            png_destroy_info_struct(png_ptr, &info_ptr);
        if (png_ptr)
            png_destroy_write_struct(&png_ptr, NULL);
        if (fp)
            fclose(fp), fp = NULL;
        throw;
    }
}

#endif
