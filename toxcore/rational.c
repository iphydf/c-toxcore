/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2024 The TokTok team.
 */

#include "rational.h"

#include "util.h"

static uint32_t gcd(uint32_t a, uint32_t b)
{
    uint32_t result = min_u32(a, b);
    while (result > 0) {
        if (a % result == 0 && b % result == 0) {
            break;
        }
        result--;
    }

    return result;
}

static Rational rat_zero(void)
{
    const Rational zero = {0, 1};
    return zero;
}

static Rational rat_normalise(Rational rat)
{
    if (rat.n == 0) {
        return rat_zero();
    }
    const uint32_t g = gcd(rat.n, rat.d);
    const Rational normalised = {rat.n / g, rat.d / g};
    return normalised;
}

Rational rat_from_nd(uint32_t n, uint32_t d)
{
    const Rational rat = {n, d};
    return rat_normalise(rat);
}

Rational rat_add(Rational lhs, Rational rhs)
{
    const Rational res = {lhs.n * rhs.d + rhs.n * lhs.d, lhs.d * rhs.d};
    return rat_normalise(res);
}

Rational rat_sub(Rational lhs, Rational rhs)
{
    if (lhs.n * rhs.d < rhs.n * lhs.d) {
        return rat_zero();
    }
    const Rational res = {lhs.n * rhs.d - rhs.n * lhs.d, lhs.d * rhs.d};
    return rat_normalise(res);
}

Rational rat_mul(Rational lhs, Rational rhs)
{
    const Rational res = {lhs.n * rhs.n, lhs.d * rhs.d};
    return rat_normalise(res);
}

Rational rat_div(Rational lhs, Rational rhs)
{
    const Rational res = {lhs.n * rhs.d, lhs.d * rhs.n};
    return rat_normalise(res);
}

Rational rat_mod(Rational lhs, Rational rhs)
{
    const Rational res = {lhs.n % rhs.n, lhs.d % rhs.d};
    return res;
}

bool rat_eq(Rational lhs, Rational rhs)
{
    return lhs.n == rhs.n && lhs.d == rhs.d;
}

bool rat_ne(Rational lhs, Rational rhs)
{
    return !rat_eq(lhs, rhs);
}

bool rat_gt(Rational lhs, Rational rhs)
{
    return lhs.n * rhs.d > rhs.n * lhs.d;
}

bool rat_ge(Rational lhs, Rational rhs)
{
    return lhs.n * rhs.d >= rhs.n * lhs.d;
}

bool rat_lt(Rational lhs, Rational rhs)
{
    return !rat_ge(lhs, rhs);
}

bool rat_le(Rational lhs, Rational rhs)
{
    return !rat_gt(lhs, rhs);
}
