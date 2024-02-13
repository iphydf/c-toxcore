/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2024 The TokTok team.
 */

#ifndef C_TOXCORE_TOXCORE_RATIONAL_H
#define C_TOXCORE_TOXCORE_RATIONAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attributes.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief A 32 bit rational number. */
typedef struct Rational {
    /** @brief Numerator. */
    uint32_t n;
    /** @brief Denominator. */
    uint32_t d;
} Rational;

Rational rat_from_nd(uint32_t n, uint32_t d);
Rational rat_add(Rational lhs, Rational rhs);
Rational rat_sub(Rational lhs, Rational rhs);
Rational rat_mul(Rational lhs, Rational rhs);
Rational rat_div(Rational lhs, Rational rhs);
Rational rat_mod(Rational lhs, Rational rhs);
bool rat_eq(Rational lhs, Rational rhs);
bool rat_ne(Rational lhs, Rational rhs);
bool rat_lt(Rational lhs, Rational rhs);
bool rat_le(Rational lhs, Rational rhs);
bool rat_gt(Rational lhs, Rational rhs);
bool rat_ge(Rational lhs, Rational rhs);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_RATIONAL_H */
