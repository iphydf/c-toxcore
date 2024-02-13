#include "rational_test_util.hh"

#include <ostream>

Rational operator+(Rational lhs, Rational rhs) { return rat_add(lhs, rhs); }
Rational operator-(Rational lhs, Rational rhs) { return rat_sub(lhs, rhs); }
Rational operator*(Rational lhs, Rational rhs) { return rat_mul(lhs, rhs); }
Rational operator/(Rational lhs, Rational rhs) { return rat_div(lhs, rhs); }
Rational operator%(Rational lhs, Rational rhs) { return rat_mod(lhs, rhs); }
bool operator==(Rational lhs, Rational rhs) { return rat_eq(lhs, rhs); }
bool operator!=(Rational lhs, Rational rhs) { return rat_ne(lhs, rhs); }
bool operator<(Rational lhs, Rational rhs) { return rat_lt(lhs, rhs); }
bool operator<=(Rational lhs, Rational rhs) { return rat_le(lhs, rhs); }
bool operator>(Rational lhs, Rational rhs) { return rat_gt(lhs, rhs); }
bool operator>=(Rational lhs, Rational rhs) { return rat_ge(lhs, rhs); }

std::ostream &operator<<(std::ostream &os, Rational r) {
  return os << r.n << "/" << r.d;
}
