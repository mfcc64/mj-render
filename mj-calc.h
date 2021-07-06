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
#include <complex.h>
#include "mj-f128.h"

#define MJ_JULIA_MODE_MANDELBROT 0
#define MJ_JULIA_MODE_JULIA_AT_C 1
#define MJ_JULIA_MODE_JULIA_AT_0 2
#define MJ_JULIA_MODE_MANDELBROT_JULIA 3

#define MJ_INFINITY (65536.0*65536.0*65536.0)

#ifndef MJ_MANDELBROT_POWER
#define MJ_MANDELBROT_POWER 2
#endif

template<typename T>
inline T mj_sqr(T v)
{
    return v * v;
}

template<typename T>
inline void mj_complex_mul(T &sx, T &sy, const T &ax, const T &ay, const T &bx, const T &by)
{
    sx = ax * bx - ay * by;
    sy = ax * by + bx * ay;
}

template<typename T>
inline void mj_complex_pow2(T &sx, T &sy, const T &zx, const T &zy, T *side_fsq = NULL)
{
    T zx2 = mj_sqr(zx);
    T zy2 = mj_sqr(zy);
    sx = zx2 - zy2;
    sy = zx * zy;
    sy = sy + sy;
    if (side_fsq)
        *side_fsq = zx2 + zy2;
}

/* C-style template */
#define MJ_DEF_COMPLEX_POW(n, m, odd)                                           \
template<typename T>                                                            \
inline void mj_complex_pow ## n (T &sx, T &sy, const T &zx, const T &zy,        \
                                 T *side_fsq = NULL)                            \
{                                                                               \
    T tx, ty;                                                                   \
    mj_complex_pow ## m (tx, ty, zx, zy, side_fsq);                             \
    if (odd) mj_complex_mul(sx, sy, tx, ty, zx, zy);                            \
    else mj_complex_pow2(sx, sy, tx, ty);                                       \
}

MJ_DEF_COMPLEX_POW(3, 2, 1)
MJ_DEF_COMPLEX_POW(4, 2, 0)
MJ_DEF_COMPLEX_POW(5, 4, 1)
MJ_DEF_COMPLEX_POW(6, 3, 0)
MJ_DEF_COMPLEX_POW(7, 6, 1)
MJ_DEF_COMPLEX_POW(8, 4, 0)
MJ_DEF_COMPLEX_POW(9, 8, 1)

#define MJ_TEMP_JOIN(a, b) a ## b
#define MJ_COMPLEX_POW(sx, sy, zx, zy, fsq, p) \
    MJ_TEMP_JOIN(mj_complex_pow, p) (sx, sy, zx, zy, fsq)

template<typename T>
double mj_calc(T cx, T cy, T zx, T zy, int max_iter)
{
    T fsq, sx, sy;
    static const double fsq_max = 1.001 * pow(2.0, 2.0 / (MJ_MANDELBROT_POWER - 1));

    for (int k = 0; k < max_iter; k++) {
        MJ_COMPLEX_POW(sx, sy, zx, zy, &fsq, MJ_MANDELBROT_POWER);

        if (fsq >= fsq_max) {
            double _cx = cx, _cy = cy, _zx = zx, _zy = zy;

            for (k-- ; k < max_iter + 1000; k++) {
                double _fsq, _sx, _sy;
                MJ_COMPLEX_POW(_sx, _sy, _zx, _zy, &_fsq, MJ_MANDELBROT_POWER);
                _zx = _sx + _cx;
                _zy = _sy + _cy;
                if (_fsq >= MJ_INFINITY)
                    return k - log2(log2(_fsq)) / log2(MJ_MANDELBROT_POWER);
            }

            return MJ_INFINITY;
        }

        zx = sx + cx;
        zy = sy + cy;
    }

    return MJ_INFINITY;
}

template<typename T>
double mj_calc_select(T cx, T cy, double _zx, double _zy, int max_iter, int julia_mode)
{
    //return is_julia ? mj_calc(cx, cy, zx, zy, max_iter) : mj_calc(cx + zx, cy + zy, T(0), T(0), max_iter);
    static const double fsq_max = 1.001 * pow(2.0, 2.0 / (MJ_MANDELBROT_POWER - 1));
    _Complex double tmp;
    double _cx = cx, _cy = cy;
    switch (julia_mode) {
    case MJ_JULIA_MODE_MANDELBROT:
        cx = cx + T(_zx);
        cy = cy + T(_zy);
        _cx += _zx;
        _cy += _zy;
        _zx = _zy = 0;
        break;
    case MJ_JULIA_MODE_JULIA_AT_0:
        break;
    case MJ_JULIA_MODE_MANDELBROT_JULIA:
        tmp = cpow(_zx + I * _zy, MJ_MANDELBROT_POWER);
        cx = cx + T(creal(tmp));
        cy = cy + T(cimag(tmp));
        _cx += creal(tmp);
        _cy += cimag(tmp);
        _zx = _zy = 0;
        break;
    case MJ_JULIA_MODE_JULIA_AT_C:
        tmp = cpow(_zx + I * _zy, 1.0 / MJ_MANDELBROT_POWER);
        _zx = creal(tmp);
        _zy = cimag(tmp);
        break;
    default:
        throw "invalid julia mode";
    }

    if (_zx * _zx + _zy * _zy >= fsq_max || _cx * _cx + _cy * _cy >= fsq_max)
        return mj_calc(_cx, _cy, _zx, _zy, max_iter);
    else
        return mj_calc(cx, cy, T(_zx), T(_zy), max_iter);
}

#endif
