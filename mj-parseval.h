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

#ifndef MJ_PARSEVAL_H
#define MJ_PARSEVAL_H 1

#include <stdio.h>
#include <typeinfo>

template<typename T>
T mj_parseval(const char *str)
{
    T v;
    char tail;
    const char *fmt = NULL;

    if (typeid(T) == typeid(double))
        fmt = "%lf %c";
    if (typeid(T) == typeid(long double))
        fmt = "%Lf %c";
    if (typeid(T) == typeid(int))
        fmt = "%i %c";

    if (!fmt)
        throw "unimplemented mj_parseval";

    int ret = sscanf(str, fmt, &v, &tail);
    if (ret != 1)
        throw "invalid mj_parseval";

    return v;
}

template<typename T>
T mj_parseval(const char *str, T min, T max)
{
    T v = mj_parseval<T>(str);
    if (v >= min && v <= max)
        return v;
    throw "out of range mj_parseval";
}

template<typename T>
T mj_parseval(const char *str, const T* list, int n_list)
{
    T v = mj_parseval<T>(str);
    for (int n = 0; n < n_list; n++)
        if (v == list[n])
            return v;
    throw "invalid value mj_parseval";
}

template<typename T>
void mj_printval(FILE *fp, T v)
{
    const char *fmt = NULL;

    if (typeid(T) == typeid(double))
        fmt = "%.17e";
    if (typeid(T) == typeid(long double))
        fmt = "%.23Le";

    if (!fmt)
        throw "unimplemented mj_printval";

    fprintf(fp, fmt, v);
}

#endif
