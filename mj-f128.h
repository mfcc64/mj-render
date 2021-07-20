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

#ifndef MJ_F128_H
#define MJ_F128_H 1

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <gmp.h>
#include "mj-parseval.h"

class MJ_F128 {
public:
    inline MJ_F128(int value = 0)
    {
        m_value[0] = 0;
        m_value[1] = int64_t(value) << 56;
    }

    inline MJ_F128(double value)
    {
        int is_negative = (value < 0.0) ? (value = -value, 1) : 0;
        value = round(value * 0x1.0p120);
        value *= 0x1.0p-64;
        m_value[1] = value;
        value = value - m_value[1];
        value *= 0x1.0p64;
        m_value[0] = value;
        if (is_negative)
            (*this) = -(*this);
    }

    /* round toward -INF */
    inline operator int() const
    {
        return int64_t(m_value[1]) >> 56;
    }

    inline operator double() const
    {
        int sign = 1;
        MJ_F128 v = (int64_t(m_value[1]) < 0) ? (sign = -1, -(*this)) : (*this);
        return sign * (v.m_value[1] * 0x1.0p-56 + v.m_value[0] * 0x1.0p-120);
    }

    MJ_F128(const char *str)
    {
        mpf_t v;
        char tail;
        mpf_init2(v, 192);
        if (gmp_sscanf(str, "%Ff %c", v, &tail) != 1)
            throw "invalid MJ_F128 string";

        int sign = mpf_sgn(v);
        mpf_abs(v, v);
        mpf_mul_2exp(v, v, 121);
        mpf_add_ui(v, v, 1);
        mpf_div_2exp(v, v, 1);
        if (mpf_cmp_d(v, 0x1.0p127) > 0)
            mpf_set_d(v, 0x1.0p127);

        mpz_t vi;
        mpz_init2(vi, 192);
        mpz_set_f(vi, v);

        m_value[0] = mpz_getlimbn(vi, 0);
        m_value[1] = mpz_getlimbn(vi, 1);

        (*this) = (sign < 0) ? -(*this) : (*this);

        mpz_clear(vi);
        mpf_clear(v);
    }

    void printval(FILE *fp) const
    {
        MJ_F128 a = (*this);
        int is_negative = (int64_t(m_value[1]) < 0) ? (a = -a, 1) : 0;

        mpz_t vi;
        mpz_init2(vi, 192);
        mp_limb_t *limb = mpz_limbs_write(vi, 2);
        limb[0] = a.m_value[0];
        limb[1] = a.m_value[1];
        mpz_limbs_finish(vi, 2);

        mpf_t v;
        mpf_init2(v, 192);
        mpf_set_z(v, vi);
        mpf_div_2exp(v, v, 120);
        if (is_negative)
            mpf_neg(v, v);
        gmp_fprintf(fp, "%.40Ff", v);

        mpf_clear(v);
        mpz_clear(vi);
    }

    friend MJ_F128 operator +(const MJ_F128& a, const MJ_F128& b);
    friend MJ_F128 operator -(const MJ_F128& a, const MJ_F128& b);
    friend MJ_F128 operator -(const MJ_F128& a);
    friend MJ_F128 operator *(const MJ_F128& a, const MJ_F128& b);
    friend MJ_F128 mj_sqr(const MJ_F128& a);
    friend bool operator >=(const MJ_F128& a, double b);

private:
    uint64_t m_value[2];

    static inline MJ_F128 s_mul_or_sqr(const MJ_F128& a, const MJ_F128& b, int is_sqr)
    {
        MJ_F128 r, low, tmp;
        uint64_t sign_a = int64_t(a.m_value[1]) >> 63;
        uint64_t sign_b = int64_t(b.m_value[1]) >> 63;

        asm("mulq %2 \n"
            :
            "=a" (r.m_value[0]),
            "=d" (r.m_value[1])
            :
            "rm" (a.m_value[1]),
            "0"  (b.m_value[1]));

        asm("mulq %2 \n"
            :
            "=a" (low.m_value[0]),
            "=d" (low.m_value[1])
            :
            "rm" (a.m_value[0]),
            "0"  (b.m_value[0]));

        asm("mulq %2 \n"
            :
            "=a" (tmp.m_value[0]),
            "=d" (tmp.m_value[1])
            :
            "rm" (a.m_value[1]),
            "0"  (b.m_value[0]));

        asm("addq %3, %0 \n"
            "adcq %4, %1 \n"
            "adcq $0, %2 \n"
            :
            "+&r" (low.m_value[1]),
            "+&r" (r.m_value[0]),
            "+&r" (r.m_value[1])
            :
            "rm" (tmp.m_value[0]),
            "rm" (tmp.m_value[1]));

        if (!is_sqr) {
            asm("mulq %2 \n"
                :
                "=a" (tmp.m_value[0]),
                "=d" (tmp.m_value[1])
                :
                "rm" (b.m_value[1]),
                "0"  (a.m_value[0]));
        }

        asm("addq %3, %0 \n"
            "adcq %4, %1 \n"
            "adcq $0, %2 \n"
            :
            "+&r" (low.m_value[1]),
            "+&r" (r.m_value[0]),
            "+&r" (r.m_value[1])
            :
            "rm" (tmp.m_value[0]),
            "rm" (tmp.m_value[1]));

        asm("subq %2, %0 \n"
            "sbbq %3, %1 \n"
            :
            "+&r" (r.m_value[0]),
            "+&r" (r.m_value[1])
            :
            "rm" (a.m_value[0] & sign_b),
            "rm" (a.m_value[1] & sign_b));

        asm("subq %2, %0 \n"
            "sbbq %3, %1 \n"
            :
            "+&r" (r.m_value[0]),
            "+&r" (r.m_value[1])
            :
            "rm" (b.m_value[0] & sign_a),
            "rm" (b.m_value[1] & sign_a));

        asm("shldq $8, %1, %2 \n"
            "shldq $8, %0, %1 \n"
            :
            "+&r" (low.m_value[1]),
            "+&r" (r.m_value[0]),
            "+&r" (r.m_value[1]));

        asm("shlq $9, %2 \n"
            "adcq $0, %0 \n"
            "adcq $0, %1 \n"
            :
            "+&r" (r.m_value[0]),
            "+&r" (r.m_value[1]),
            "+&r" (low.m_value[1]));

        return r;
    }
};

inline MJ_F128 operator +(const MJ_F128& a, const MJ_F128& b)
{
    MJ_F128 r;
    asm("addq %2, %0 \n"
        "adcq %3, %1 \n"
        :
        "=&r" (r.m_value[0]),
        "=&r" (r.m_value[1])
        :
        "rm"  (b.m_value[0]),
        "rm"  (b.m_value[1]),
        "0"   (a.m_value[0]),
        "1"   (a.m_value[1]));
    return r;
}

inline MJ_F128 operator -(const MJ_F128& a, const MJ_F128& b)
{
    MJ_F128 r;
    asm("subq %2, %0 \n"
        "sbbq %3, %1 \n"
        :
        "=&r" (r.m_value[0]),
        "=&r" (r.m_value[1])
        :
        "rm"  (b.m_value[0]),
        "rm"  (b.m_value[1]),
        "0"   (a.m_value[0]),
        "1"   (a.m_value[1]));
    return r;
}

inline MJ_F128 operator -(const MJ_F128& a)
{
    MJ_F128 r;
    asm("negq %0 \n"
        "sbbq %2, %1 \n"
        :
        "=&r" (r.m_value[0]),
        "=&r" (r.m_value[1])
        :
        "rm"  (a.m_value[1]),
        "0"   (a.m_value[0]),
        "1"   (uint64_t(0)));
    return r;
}

inline MJ_F128 operator *(const MJ_F128& a, const MJ_F128& b)
{
    return MJ_F128::s_mul_or_sqr(a, b, 0);
}

inline MJ_F128 mj_sqr(const MJ_F128& a)
{
    return MJ_F128::s_mul_or_sqr(a, a, 1);
}

inline bool operator >=(const MJ_F128& a, double b)
{
    return int64_t(a.m_value[1]) >= int64_t(b * (1LL << 56));
}

inline MJ_F128 mj_parseval(const char *str, MJ_F128 dummy)
{
    return str;
}

inline void mj_printval(FILE *fp, MJ_F128 v)
{
    v.printval(fp);
}

#endif
