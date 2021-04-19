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

#ifndef MJ_CALC_H
#define MJ_CALC_H 1

#include <math.h>
#include "mj-f128.h"

#define MJ_INFINITY (65536.0*65536.0*65536.0)

template<typename T>
inline T mj_sqr(T v)
{
    return v * v;
}

template<typename T>
double mj_calc(T cx, T cy, T zx, T zy, int max_iter)
{
    for (int k = 0; k < max_iter; k++) {
        T zx2 = mj_sqr(zx);
        T zy2 = mj_sqr(zy);

        if ((zx2 + zy2) >= 4.004) {
            double _cx = cx, _cy = cy, _zx = zx, _zy = zy;

            for (k-- ; k < max_iter + 1000; k++) {
                double _zx2 = _zx * _zx;
                double _zy2 = _zy * _zy;
                double _zxy = _zx * _zy;

                _zx = _zx2 - _zy2 + _cx;
                _zy = _zxy + _zxy + _cy;

                double fsq = _zx2 + _zy2;
                if (fsq >= MJ_INFINITY)
                    return k - log2(log2(fsq));
            }

            return MJ_INFINITY;
        }

        T zxy = zx * zy;
        zx = zx2 - zy2 + cx;
        zy = zxy + zxy + cy;
    }

    return MJ_INFINITY;
}

template<typename T>
double mj_calc_select(T cx, T cy, T zx, T zy, int max_iter, int is_julia)
{
    return is_julia ? mj_calc(cx, cy, zx, zy, max_iter) : mj_calc(cx + zx, cy + zy, T(0), T(0), max_iter);
}

#endif
