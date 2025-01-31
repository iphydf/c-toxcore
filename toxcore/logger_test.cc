/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2025 The TokTok team.
 */
#include "logger.h"

#include <gtest/gtest.h>

#include "mem.h"

namespace {

struct Person {
    const char *name;
    uint64_t age;
    double height;
};

static void person_fmt(Logger_Fmt *fmt, const void *object)
{
    const Person *person = (const Person *)object;
    LOGGER_FMT(fmt, "Person{.name=%1, age=%2, height=%3}", s(person->name), u(person->age),
        f(person->height));
}

struct LoggerFmt : ::testing::Test {
protected:
    Logger *log_ = logger_new(os_memory());
    std::string output_;

    LoggerFmt()
    {
        logger_callback_log(
            log_,
            [](void *context, Logger_Level level, const char *file, uint32_t line, const char *func,
                const char *message, void *userdata) { *(std::string *)userdata = message; },
            nullptr, &output_);
    }

    ~LoggerFmt() { logger_kill(log_); }
};

TEST_F(LoggerFmt, FormatsArgumentsCorrectly)
{
    LOGGER_INFO_A(log_, "Hello %1, General %2 I'm %3 years old. I'm %4 c%5 tall", s("world"),
        s("Kenobi"), u(42), f(180.5), c('m'));

    EXPECT_EQ("Hello world, General Kenobi I'm 42 years old. I'm 180.500000 cm tall", output_);
}

TEST_F(LoggerFmt, FormatsUserDefinedTypesCorrectly)
{
    const Person me{"Obi-Wan", 72, 185.41};

    LOGGER_INFO_A(log_, "Person data: %1", ud(&me, person_fmt));

    EXPECT_EQ("Person data: Person{.name=Obi-Wan, age=72, height=185.410000}", output_);
}

TEST_F(LoggerFmt, AllowsOutOfOrderArgs)
{
    LOGGER_INFO_A(log_, "%2 %1", s("world"), s("hello"));

    EXPECT_EQ("hello world", output_);
}

TEST_F(LoggerFmt, AllowsRepeatedArgs)
{
    LOGGER_INFO_A(log_, "%1 %1", s("hello"));

    EXPECT_EQ("hello hello", output_);
}

TEST_F(LoggerFmt, GracefullyHandlesOutOfRange)
{
    LOGGER_INFO_A(log_, "%1 %0 %999 %1152921504606846975", s("hello"));

    EXPECT_EQ("hello %(missing:0) %(missing:999) %(missing:1152921504606846975)", output_);
}

TEST_F(LoggerFmt, SupportsHexPrinting)
{
    LOGGER_INFO_A(log_, "0x%1 0x%2 0x%3", x(0xffff), x(0xffffffffffffffff), x(0x1234));

    EXPECT_EQ("0xffff 0xffffffffffffffff 0x1234", output_);
}

TEST_F(LoggerFmt, SupportsHexWidth)
{
    LOGGER_INFO_A(log_, "0x%1", x0(0x12, 4));

    EXPECT_EQ("0x0012", output_);
}

TEST_F(LoggerFmt, SupportsStringWidth)
{
    LOGGER_INFO_A(log_, "%1", s("hello"));

    EXPECT_EQ("hello", output_);
}

}  // namespace
