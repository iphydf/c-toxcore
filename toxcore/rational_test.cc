#include "rational.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "rational_test_util.hh"

namespace {

using ::testing::Le;
using ::testing::Lt;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::Not;

TEST(Rational, EquivalentNumbersComparesEqual)
{
    const Rational r1 = rat_from_nd(10, 4);
    const Rational r2 = rat_from_nd(5, 2);

    EXPECT_EQ(r1, r2);
}

TEST(Rational, DifferentNumbersComparesNotEqual)
{
    const Rational r1 = rat_from_nd(10, 4);
    const Rational r2 = rat_from_nd(5, 3);

    EXPECT_NE(r1, r2);
}

TEST(Rational, LessThan)
{
    const Rational r1 = rat_from_nd(10, 7);
    const Rational r2 = rat_from_nd(5, 2);

    EXPECT_THAT(r1, Lt(r2));
    EXPECT_THAT(r2, Not(Lt(r1)));
}

TEST(Rational, LessThanOrEqual)
{
    const Rational r1 = rat_from_nd(10, 7);
    const Rational r2 = rat_from_nd(5, 2);

    EXPECT_THAT(r1, Le(r2));
    EXPECT_THAT(r2, Not(Le(r1)));
    EXPECT_THAT(r1, Le(r1));
}

TEST(Rational, GreaterThan)
{
    const Rational r1 = rat_from_nd(10, 7);
    const Rational r2 = rat_from_nd(5, 2);

    EXPECT_THAT(r2, Gt(r1));
    EXPECT_THAT(r1, Not(Gt(r2)));
}

TEST(Rational, GreaterThanOrEqual)
{
    const Rational r1 = rat_from_nd(10, 7);
    const Rational r2 = rat_from_nd(5, 2);

    EXPECT_THAT(r2, Ge(r1));
    EXPECT_THAT(r1, Not(Ge(r2)));
    EXPECT_THAT(r1, Ge(r1));
}

TEST(Rational, Add)
{
    const Rational r1 = rat_from_nd(10, 7);
    const Rational r2 = rat_from_nd(5, 2);
    const Rational expected = rat_from_nd(55, 14);

    EXPECT_EQ(r1 + r2, expected);
}

TEST(Rational, Subtract)
{
    const Rational r1 = rat_from_nd(5, 2);
    const Rational r2 = rat_from_nd(10, 7);
    const Rational expected = rat_from_nd(15, 14);

    EXPECT_EQ(r1 - r2, expected);
}

TEST(Rational, Multiply)
{
    const Rational r1 = rat_from_nd(5, 2);
    const Rational r2 = rat_from_nd(10, 7);
    const Rational expected = rat_from_nd(25, 7);

    EXPECT_EQ(r1 * r2, expected);
}

TEST(Rational, MultiplyByZero)
{
    const Rational r1 = rat_from_nd(5, 2);
    const Rational r2 = rat_from_nd(0, 1);
    const Rational expected = rat_from_nd(0, 1);

    EXPECT_EQ(r1 * r2, expected);
}

TEST(Rational, MultiplyLargeNumbers)
{
    const Rational r1 = rat_from_nd(70429133, 7043123);
    const Rational r2 = rat_from_nd(70429823, 7044061);
    const Rational expected = rat_from_nd(2101475507, 1020806407);

    EXPECT_EQ(r1 * r2, expected);
}

TEST(Rational, Divide)
{
    const Rational r1 = rat_from_nd(5, 2);
    const Rational r2 = rat_from_nd(10, 7);
    const Rational expected = rat_from_nd(35, 20);

    EXPECT_EQ(r1 / r2, expected);
}

TEST(Rational, Modulo)
{
    const Rational r1 = rat_from_nd(5, 2);
    const Rational r2 = rat_from_nd(10, 7);
    const Rational expected = rat_from_nd(5, 2);

    EXPECT_EQ(r1 % r2, expected);
}

}  // namespace
