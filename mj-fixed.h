/*
 * Copyright (C) 2021 Muhammad Faiz
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

#ifndef MJ_FIXED_H
#define MJ_FIXED_H 1

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <gmp.h>
#include "mj-parseval.h"

template<int BITS>
class MJ_Fixed {
public:
    inline MJ_Fixed(int value = 0)
    {
        for (int k = 0; k < LIMBS - 1; k++)
            m_value[k] = 0;
        m_value[LIMBS - 1] = value;
    }

    inline MJ_Fixed(double value)
    {
        int is_negative = (value < 0.0) ? (value = -value, 1) : 0;
        for (int k = LIMBS - 1; k > 0; k--) {
            m_value[k] = value;
            value -= m_value[k];
            value *= 0x1.0p64;
        }
        m_value[0] = round(value);
        if (is_negative)
            (*this) = -(*this);
    }

    /* round toward -INF */
    inline operator int() const
    {
        return int64_t(m_value[LIMBS - 1]);
    }

    inline operator double() const
    {
        int sign = 1;
        MJ_Fixed v = (int64_t(m_value[LIMBS - 1]) < 0) ? (sign = -1, -(*this)) : (*this);
        double result = 0.0;
        double mul = 1.0;
        for (int k = LIMBS - 1; k >= 0; k--) {
            result += v.m_value[k] * mul;
            mul *= 0x1.0p-64;
        }
        return sign * result;;
    }

    MJ_Fixed(const char *str)
    {
        mpf_t v;
        char tail;
        mpf_init2(v, BITS + 64);

        if (gmp_sscanf(str, "%Ff %c", v, &tail) != 1)
            throw "invalid MJ_F128 string";

        int sign = mpf_sgn(v);
        mpf_abs(v, v);
        if (mpf_cmp_d(v, 256.0) > 0)
            mpf_set_d(v, 256.0);

        mpf_mul_2exp(v, v, BITS - 64 + 1);
        mpf_add_ui(v, v, 1);
        mpf_div_2exp(v, v, 1);

        mpz_t vi;
        mpz_init2(vi, BITS + 64);
        mpz_set_f(vi, v);

        for (int k = 0; k < LIMBS; k++)
            m_value[k] = mpz_getlimbn(vi, k);

        (*this) = (sign < 0) ? -(*this) : (*this);

        mpz_clear(vi);
        mpf_clear(v);
    }

    void printval(FILE *fp) const
    {
        MJ_Fixed a = *this;
        int is_negative = (int64_t(a.m_value[LIMBS - 1]) < 0) ? (a = -a, 1) : 0;

        mpz_t vi;
        mpz_init2(vi, BITS + 64);
        mp_limb_t *limb = mpz_limbs_write(vi, LIMBS);
        for (int k = 0; k < LIMBS; k++)
            limb[k] = a.m_value[k];
        mpz_limbs_finish(vi, LIMBS);

        mpf_t v;
        mpf_init2(v, BITS + 64);
        mpf_set_z(v, vi);
        mpf_div_2exp(v, v, BITS - 64);
        if (is_negative)
            mpf_neg(v, v);

        char fmt[64];
        sprintf(fmt, "%%.%dFf", int((BITS - 64) / 3));
        gmp_fprintf(fp, fmt, v);

        mpf_clear(v);
        mpz_clear(vi);
    }

    template<int BITS2> friend MJ_Fixed<BITS2> operator +(const MJ_Fixed<BITS2> &a, const MJ_Fixed<BITS2> &b);
    template<int BITS2> friend MJ_Fixed<BITS2> operator -(const MJ_Fixed<BITS2> &a, const MJ_Fixed<BITS2> &b);
    template<int BITS2> friend MJ_Fixed<BITS2> operator -(const MJ_Fixed<BITS2> &a);
    template<int BITS2> friend MJ_Fixed<BITS2> operator *(const MJ_Fixed<BITS2> &a, const MJ_Fixed<BITS2> &b);
    template<int BITS2> friend MJ_Fixed<BITS2> mj_sqr(const MJ_Fixed<BITS2> &a);
    template<int BITS2> friend bool operator >=(const MJ_Fixed<BITS2> &a, double b);

private:
    static const int BYTES = BITS / 8;
    static const int LIMBS = BITS / 64;

    uint64_t m_value[LIMBS];
};

template<int BITS>
inline MJ_Fixed<BITS> operator +(const MJ_Fixed<BITS> &a, const MJ_Fixed<BITS> &b)
{
    MJ_Fixed<BITS> r;
    mpn_add_n(r.m_value, a.m_value, b.m_value, MJ_Fixed<BITS>::LIMBS);
    return r;
}

template<int BITS>
inline MJ_Fixed<BITS> operator -(const MJ_Fixed<BITS> &a, const MJ_Fixed<BITS> &b)
{
    MJ_Fixed<BITS> r;
    mpn_sub_n(r.m_value, a.m_value, b.m_value, MJ_Fixed<BITS>::LIMBS);
    return r;
}

template<int BITS>
inline MJ_Fixed<BITS> operator -(const MJ_Fixed<BITS> &a)
{
    MJ_Fixed<BITS> r;
    mpn_neg(r.m_value, a.m_value, MJ_Fixed<BITS>::LIMBS);
    return r;
}

template<int BITS>
inline MJ_Fixed<BITS> operator *(const MJ_Fixed<BITS> &a, const MJ_Fixed<BITS> &b)
{
    static const int LIMBS = MJ_Fixed<BITS>::LIMBS;
    MJ_Fixed<BITS> r;
    uint64_t buf[LIMBS * 2];
    int sign = 1;
    if (int64_t(a.m_value[LIMBS - 1]) >= 0) {
        if (int64_t(b.m_value[LIMBS - 1]) >= 0) {
            mpn_mul_n(buf, a.m_value, b.m_value, LIMBS);
        } else {
            MJ_Fixed<BITS> tmp_b = -b;
            sign *= -1;
            mpn_mul_n(buf, a.m_value, tmp_b.m_value, LIMBS);
        }
    } else {
        MJ_Fixed<BITS> tmp_a = -a;
        sign *= -1;
        if (int64_t(b.m_value[LIMBS - 1]) >= 0) {
            mpn_mul_n(buf, tmp_a.m_value, b.m_value, LIMBS);
        } else {
            MJ_Fixed<BITS> tmp_b = -b;
            sign *= -1;
            mpn_mul_n(buf, tmp_a.m_value, tmp_b.m_value, LIMBS);
        }
    }

    if (int64_t(buf[LIMBS - 2]) < 0)
        mpn_add_1(buf + LIMBS - 1, buf + LIMBS - 1, LIMBS, 1);

    if (sign >= 0) {
        for (int k = 0; k < LIMBS; k++)
            r.m_value[k] = buf[k + LIMBS - 1];
    } else {
        mpn_neg(r.m_value, buf + LIMBS - 1, LIMBS);
    }

    return r;
}

template<int BITS>
inline MJ_Fixed<BITS> mj_sqr(const MJ_Fixed<BITS> &a)
{
    static const int LIMBS = MJ_Fixed<BITS>::LIMBS;
    MJ_Fixed<BITS> r;
    uint64_t buf[LIMBS * 2];

    if (int64_t(a.m_value[LIMBS -1]) >= 0) {
        mpn_sqr(buf, a.m_value, LIMBS);
    } else {
        MJ_Fixed<BITS> tmp_a = -a;
        mpn_sqr(buf, tmp_a.m_value, LIMBS);
    }

    if (int64_t(buf[LIMBS - 2]) < 0) {
        mpn_add_1(r.m_value, buf + LIMBS - 1, LIMBS, 1);
    } else {
        for (int k = 0; k < LIMBS; k++)
            r.m_value[k] = buf[k + LIMBS - 1];
    }

    return r;
}

template<int BITS>
inline bool operator >=(const MJ_Fixed<BITS> &a, double b)
{
    return double(a) >= b;
}

template<int BITS>
inline MJ_Fixed<BITS> mj_parseval(const char *str, MJ_Fixed<BITS> dummy)
{
    return str;
}

template<int BITS>
inline void mj_printval(FILE *fp, MJ_Fixed<BITS> v)
{
    v.printval(fp);
}

#endif
