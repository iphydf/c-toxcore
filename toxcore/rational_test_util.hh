#ifndef C_TOXCORE_TOXCORE_RATIONAL_TEST_UTIL_H
#define C_TOXCORE_TOXCORE_RATIONAL_TEST_UTIL_H

#include <iosfwd>

#include "rational.h"

Rational operator+(Rational lhs, Rational rhs);
Rational operator-(Rational lhs, Rational rhs);
Rational operator*(Rational lhs, Rational rhs);
Rational operator/(Rational lhs, Rational rhs);
Rational operator%(Rational lhs, Rational rhs);
bool operator==(Rational lhs, Rational rhs);
bool operator!=(Rational lhs, Rational rhs);
bool operator<(Rational lhs, Rational rhs);
bool operator<=(Rational lhs, Rational rhs);
bool operator>(Rational lhs, Rational rhs);
bool operator>=(Rational lhs, Rational rhs);

std::ostream &operator<<(std::ostream &os, Rational r);

#endif  // C_TOXCORE_TOXCORE_RATIONAL_TEST_UTIL_H
