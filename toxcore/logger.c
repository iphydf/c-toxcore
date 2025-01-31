/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2025 The TokTok team.
 * Copyright © 2013-2015 Tox project.
 */

/**
 * Text logging abstraction.
 */
#include "logger.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>  // IWYU pragma: keep
#include <string.h>

#include "ccompat.h"
#include "mem.h"

#define LOGGER_INIT_SIZE 4

struct Logger {
    const Memory *mem;

    logger_cb *callback;
    void *context;
    void *userdata;
};

/*
 * Public Functions
 */

Logger *logger_new(const Memory *mem)
{
    Logger *log = (Logger *)mem_alloc(mem, sizeof(Logger));

    if (log == nullptr) {
        return nullptr;
    }

    log->mem = mem;

    return log;
}

void logger_kill(Logger *log)
{
    if (log == nullptr) {
        return;
    }

    mem_delete(log->mem, log);
}

void logger_callback_log(Logger *log, logger_cb *function, void *context, void *userdata)
{
    assert(log != nullptr);
    log->callback = function;
    log->context = context;
    log->userdata = userdata;
}

void logger_write(const Logger *log, Logger_Level level, const char *file, uint32_t line,
    const char *func, const char *format, ...)
{
    if (log == nullptr) {
        return;
    }

    if (log->callback == nullptr) {
        return;
    }

    // Only pass the file name, not the entire file path, for privacy reasons.
    // The full path may contain PII of the person compiling toxcore (their
    // username and directory layout).
    const char *filename = strrchr(file, '/');
    file = filename != nullptr ? filename + 1 : file;
#if defined(_WIN32) || defined(__CYGWIN__)
    // On Windows, the path separator *may* be a backslash, so we look for that
    // one too.
    const char *windows_filename = strrchr(file, '\\');
    file = windows_filename != nullptr ? windows_filename + 1 : file;
#endif /* WIN32 */

    // Format message
    char msg[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);

    log->callback(log->context, level, file, line, func, msg, log->userdata);
}

void logger_abort(void) { abort(); }

static Logger_Arg_UD logger_arg_make_ud(const void *object, logger_fmt_cb *fmt_callback)
{
    const Logger_Arg_UD ud = {object, fmt_callback};
    return ud;
}

static Logger_Arg_D logger_arg_make_d(uint64_t value, int8_t width)
{
    const Logger_Arg_D d = {value, width};
    return d;
}

static Logger_Arg_S logger_arg_make_s(const char *value, int8_t width)
{
    const Logger_Arg_S s = {value, width};
    return s;
}

static Logger_Arg_U logger_arg_make_u(uint64_t value, int8_t width, bool hex)
{
    const Logger_Arg_U u = {value, width, hex};
    return u;
}

Logger_Arg logger_arg_ud(const void *object, logger_fmt_cb *fmt_callback)
{
    Logger_Arg arg;
    arg.type = LOGGER_ARG_TYPE_USER_DEFINED;
    arg.data.ud = logger_arg_make_ud(object, fmt_callback);
    return arg;
}

Logger_Arg logger_arg_c(char value)
{
    Logger_Arg arg;
    arg.type = LOGGER_ARG_TYPE_CHAR;
    arg.data.c = value;
    return arg;
}

Logger_Arg logger_arg_d(int64_t value)
{
    Logger_Arg arg;
    arg.type = LOGGER_ARG_TYPE_INT64;
    arg.data.d.value = value;
    return arg;
}

Logger_Arg logger_arg_f(double value)
{
    Logger_Arg arg;
    arg.type = LOGGER_ARG_TYPE_FLOAT;
    arg.data.f = value;
    return arg;
}

Logger_Arg logger_arg_s(const char *value)
{
    Logger_Arg arg;
    arg.type = LOGGER_ARG_TYPE_STRING;
    arg.data.s = logger_arg_make_s(value, 0);
    return arg;
}

Logger_Arg logger_arg_s_(const char *value, int8_t width)
{
    Logger_Arg arg;
    arg.type = LOGGER_ARG_TYPE_STRING;
    arg.data.s = logger_arg_make_s(value, width);
    return arg;
}

Logger_Arg logger_arg_u(uint64_t value)
{
    Logger_Arg arg;
    arg.type = LOGGER_ARG_TYPE_UINT64;
    arg.data.u = logger_arg_make_u(value, 0, false);
    return arg;
}

Logger_Arg logger_arg_x(uint64_t value)
{
    Logger_Arg arg;
    arg.type = LOGGER_ARG_TYPE_UINT64;
    arg.data.u = logger_arg_make_u(value, 0, true);
    return arg;
}

Logger_Arg logger_arg_x0(uint64_t value, int8_t width)
{
    Logger_Arg arg;
    arg.type = LOGGER_ARG_TYPE_UINT64;
    arg.data.u = logger_arg_make_u(value, width, true);
    return arg;
}

struct Logger_Fmt {
    const Memory *mem;
    const char *format;
    char *buffer;
    uint32_t buffer_size;
    uint32_t buffer_pos;
    bool error;
};

static bool logger_fmt_init(Logger_Fmt *fmt, const Memory *mem, const char *format)
{
    fmt->mem = mem;
    fmt->format = format;
    fmt->buffer = (char *)mem_balloc(mem, LOGGER_INIT_SIZE);
    fmt->buffer_size = LOGGER_INIT_SIZE;
    fmt->buffer_pos = 0;
    fmt->error = fmt->buffer == nullptr;

    return !fmt->error;
}

static void logger_fmt_deinit(Logger_Fmt *fmt)
{
    mem_delete(fmt->mem, fmt->buffer);
}

static bool logger_fmt_ensure_size(Logger_Fmt *fmt, uint32_t size)
{
    if (fmt->buffer_pos + size >= fmt->buffer_size) {
        fmt->buffer_size *= 2;
        char *new_buffer = (char *)mem_brealloc(fmt->mem, fmt->buffer, fmt->buffer_size);
        if (new_buffer == nullptr) {
            fmt->error = true;
            return false;
        }
        fmt->buffer = new_buffer;
    }

    return true;
}

static void logger_fmt_write(Logger_Fmt *fmt, const char *str, uint32_t size)
{
    if (logger_fmt_ensure_size(fmt, size)) {
        memcpy(fmt->buffer + fmt->buffer_pos, str, size);
        fmt->buffer_pos += size;
    }
}

static void logger_fmt_write_ud(Logger_Fmt *fmt, const Logger_Arg_UD *ud)
{
    ud->fmt_callback(fmt, ud->object);
}

static void logger_fmt_write_c(Logger_Fmt *fmt, char c)
{
    logger_fmt_write(fmt, &c, 1);
}

static int logger_fmt_format_d(Logger_Fmt *fmt, const Logger_Arg_D *d, char *buffer, uint32_t buffer_size)
{
    return snprintf(buffer, buffer_size, "%0*" PRId64, d->width, d->value);
}

static void logger_fmt_write_d(Logger_Fmt *fmt, const Logger_Arg_D *d)
{
    char buffer[32];
    const int size = logger_fmt_format_d(fmt, d, buffer, sizeof(buffer));
    if (size > 0) {
        logger_fmt_write(fmt, buffer, size);
    }
}

static int logger_fmt_format_u(Logger_Fmt *fmt, const Logger_Arg_U *u, char *buffer, uint32_t buffer_size)
{
    if (u->hex) {
        return snprintf(buffer, buffer_size, "%0*" PRIx64, u->width, u->value);
    }

    return snprintf(buffer, buffer_size, "%0*" PRIu64, u->width, u->value);
}

static void logger_fmt_write_u(Logger_Fmt *fmt, const Logger_Arg_U *u)
{
    char buffer[32];
    const int size = logger_fmt_format_u(fmt, u, buffer, sizeof(buffer));
    if (size > 0) {
        logger_fmt_write(fmt, buffer, size);
    }
}

static void logger_fmt_write_f(Logger_Fmt *fmt, double f)
{
    char buffer[32];
    const int size = snprintf(buffer, sizeof(buffer), "%f", f);
    if (size > 0) {
        logger_fmt_write(fmt, buffer, size);
    }
}

static void logger_fmt_write_s(Logger_Fmt *fmt, const Logger_Arg_S *s)
{
    logger_fmt_write(fmt, s->value, strlen(s->value));
}

static void logger_fmt_arg(Logger_Fmt *fmt, const Logger_Arg *arg)
{
    switch (arg->type) {
        case LOGGER_ARG_TYPE_USER_DEFINED: {
            logger_fmt_write_ud(fmt, &arg->data.ud);
            break;
        }
        case LOGGER_ARG_TYPE_CHAR: {
            logger_fmt_write_c(fmt, arg->data.c);
            break;
        }
        case LOGGER_ARG_TYPE_INT64: {
            logger_fmt_write_d(fmt, &arg->data.d);
            break;
        }
        case LOGGER_ARG_TYPE_UINT64: {
            logger_fmt_write_u(fmt, &arg->data.u);
            break;
        }
        case LOGGER_ARG_TYPE_FLOAT: {
            logger_fmt_write_f(fmt, arg->data.f);
            break;
        }
        case LOGGER_ARG_TYPE_STRING: {
            logger_fmt_write_s(fmt, &arg->data.s);
            break;
        }
    }
}

void logger_fmt_args(Logger_Fmt *fmt, const char *format, const Logger_Arg *args, uint8_t args_size)
{
    const uint32_t format_size = strlen(format);
    for (uint32_t i = 0; i < format_size; i++) {
        if (format[i] != '%') {
            logger_fmt_write(fmt, &format[i], 1);
            continue;
        }

        if (format[i + 1] == '%') {
            logger_fmt_write(fmt, &format[i], 1);
            continue;
        }
        char *endptr;
        const unsigned long index = strtoul(&format[i + 1], &endptr, 10);
        if (endptr != &format[i + 1]) {
            if (index > 0 && index <= args_size) {
                logger_fmt_arg(fmt, &args[index - 1]);
            } else {
                const Logger_Arg missing_arg = logger_arg_s("%(missing:");
                assert(missing_arg.type == LOGGER_ARG_TYPE_STRING);
                logger_fmt_write_s(fmt, &missing_arg.data.s);
                const Logger_Arg index_arg = logger_arg_u(index);
                assert(index_arg.type == LOGGER_ARG_TYPE_UINT64);
                logger_fmt_write_u(fmt, &index_arg.data.u);
                logger_fmt_write_c(fmt, ')');
            }
            i = endptr - format - 1;
        } else {
            logger_fmt_write(fmt, &format[i], 1);
        }
    }
}

void logger_write_args(const Logger *log, Logger_Level level, const char *file, uint32_t line,
    const char *func, const char *format, const Logger_Arg *args, uint8_t args_size)
{
    Logger_Fmt fmt;
    if (!logger_fmt_init(&fmt, log->mem, format)) {
        logger_write(log, level, file, line, func, "Allocation failure in logger; original format: \"%s\"", format);
        return;
    }
    logger_fmt_args(&fmt, format, args, args_size);
    fmt.buffer[fmt.buffer_pos] = '\0';
    logger_write(log, level, file, line, func, "%s", fmt.buffer);
    logger_fmt_deinit(&fmt);
}
