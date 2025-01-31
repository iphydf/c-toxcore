/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2025 The TokTok team.
 * Copyright © 2013 Tox project.
 */

/**
 * Logger abstraction backed by callbacks for writing.
 */
#ifndef C_TOXCORE_TOXCORE_LOGGER_H
#define C_TOXCORE_TOXCORE_LOGGER_H

#include <stdbool.h>
#include <stdint.h>

#include "attributes.h"
#include "mem.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MIN_LOGGER_LEVEL
#define MIN_LOGGER_LEVEL LOGGER_LEVEL_INFO
#endif /* MIN_LOGGER_LEVEL */

// NOTE: Don't forget to update build system files after modifying the enum.
typedef enum Logger_Level {
    LOGGER_LEVEL_TRACE,
    LOGGER_LEVEL_DEBUG,
    LOGGER_LEVEL_INFO,
    LOGGER_LEVEL_WARNING,
    LOGGER_LEVEL_ERROR,
} Logger_Level;

typedef struct Logger Logger;

typedef void logger_cb(void *_Nullable context, Logger_Level level, const char *_Nonnull file, uint32_t line,
                       const char *_Nonnull func, const char *_Nonnull message, void *_Nullable userdata);

/**
 * Creates a new logger with logging disabled (callback is NULL) by default.
 */
Logger *_Nullable logger_new(const Memory *_Nonnull mem);

/**
 * Frees all resources associated with the logger.
 */
void logger_kill(Logger *_Nullable log);
/**
 * Sets the logger callback. Disables logging if set to NULL.
 * The context parameter is passed to the callback as first argument.
 */
void logger_callback_log(Logger *_Nonnull log, logger_cb *_Nullable function, void *_Nullable context, void *_Nullable userdata);
/** @brief Main write function. If logging is disabled, this does nothing.
 *
 * If the logger is NULL and `NDEBUG` is not defined, this writes to stderr.
 * This behaviour should not be used in production code, but can be useful for
 * temporarily debugging a function that does not have a logger available. It's
 * essentially `fprintf(stderr, ...)`, but with source location.
 *
 * If `NDEBUG` is defined, the NULL logger does nothing.
 */
GNU_PRINTF(6, 7)
void logger_write(const Logger *_Nullable log, Logger_Level level, const char *_Nonnull file, uint32_t line, const char *_Nonnull func, const char *_Nonnull format, ...);

/* @brief Terminate the program with a signal. */
void logger_abort(void);

#define LOGGER_WRITE(log, level, ...)                                            \
    do {                                                                         \
        if (level >= MIN_LOGGER_LEVEL) {                                         \
            logger_write(log, level, __FILE__, __LINE__, __func__, __VA_ARGS__); \
        }                                                                        \
    } while (0)

/* To log with an logger */
#define LOGGER_TRACE(log, ...)   LOGGER_WRITE(log, LOGGER_LEVEL_TRACE, __VA_ARGS__)
#define LOGGER_DEBUG(log, ...)   LOGGER_WRITE(log, LOGGER_LEVEL_DEBUG, __VA_ARGS__)
#define LOGGER_INFO(log, ...)    LOGGER_WRITE(log, LOGGER_LEVEL_INFO, __VA_ARGS__)
#define LOGGER_WARNING(log, ...) LOGGER_WRITE(log, LOGGER_LEVEL_WARNING, __VA_ARGS__)
#define LOGGER_ERROR(log, ...)   LOGGER_WRITE(log, LOGGER_LEVEL_ERROR, __VA_ARGS__)

#define LOGGER_FATAL(log, ...)          \
    do {                                \
        LOGGER_ERROR(log, __VA_ARGS__); \
        logger_abort();                 \
    } while (0)

#define LOGGER_ASSERT(log, cond, ...)              \
    do {                                           \
        if (!(cond)) {                             \
            LOGGER_ERROR(log, "Assertion failed"); \
            LOGGER_FATAL(log, __VA_ARGS__);        \
        }                                          \
    } while (0)

typedef struct Logger_Fmt Logger_Fmt;

typedef void logger_fmt_cb(Logger_Fmt *fmt, const void *object);

// User-Defined
typedef struct Logger_Arg_UD {
    const void *object;
    logger_fmt_cb *fmt_callback;
} Logger_Arg_UD;

typedef struct Logger_Arg_D {
    uint64_t value;
    int8_t width;
} Logger_Arg_D;

typedef struct Logger_Arg_S {
    const char *value;
    int8_t width;
} Logger_Arg_S;

typedef struct Logger_Arg_U {
    uint64_t value;
    int8_t width;
    bool hex;
} Logger_Arg_U;

typedef union Logger_Arg_Data {
    Logger_Arg_UD ud;
    char c;
    Logger_Arg_D d;
    double f;
    Logger_Arg_S s;
    Logger_Arg_U u;
} Logger_Arg_Data;

typedef enum Logger_Arg_Type {
    LOGGER_ARG_TYPE_USER_DEFINED,
    LOGGER_ARG_TYPE_CHAR,
    LOGGER_ARG_TYPE_INT64,
    LOGGER_ARG_TYPE_UINT64,
    LOGGER_ARG_TYPE_FLOAT,
    LOGGER_ARG_TYPE_STRING,
} Logger_Arg_Type;

typedef struct Logger_Arg {
    Logger_Arg_Type type;
    Logger_Arg_Data data;
} Logger_Arg;

typedef Logger_Arg logger_arg_ud_cb(const void *object, logger_fmt_cb *fmt_callback);
logger_arg_ud_cb logger_arg_ud;
typedef Logger_Arg logger_arg_c_cb(char value);
logger_arg_c_cb logger_arg_c;
typedef Logger_Arg logger_arg_d_cb(int64_t value);
logger_arg_d_cb logger_arg_d;
typedef Logger_Arg logger_arg_f_cb(double value);
logger_arg_f_cb logger_arg_f;
typedef Logger_Arg logger_arg_s_cb(const char *value);
logger_arg_s_cb logger_arg_s;
typedef Logger_Arg logger_arg_s__cb(const char *value, int8_t width);
logger_arg_s__cb logger_arg_s_;
typedef Logger_Arg logger_arg_u_cb(uint64_t value);
logger_arg_u_cb logger_arg_u;
typedef Logger_Arg logger_arg_x_cb(uint64_t value);
logger_arg_x_cb logger_arg_x;
typedef Logger_Arg logger_arg_x0_cb(uint64_t value, int8_t width);
logger_arg_x0_cb logger_arg_x0;

non_null()
void logger_fmt_args(Logger_Fmt *fmt, const char *format, const Logger_Arg *args, uint8_t args_size);

#define LOGGER_FMT(fmt, format, ...)                                        \
    do {                                                                    \
        MAYBE_UNUSED logger_arg_ud_cb *ud = logger_arg_ud;                  \
        MAYBE_UNUSED logger_arg_c_cb *c = logger_arg_c;                     \
        MAYBE_UNUSED logger_arg_d_cb *d = logger_arg_d;                     \
        MAYBE_UNUSED logger_arg_f_cb *f = logger_arg_f;                     \
        MAYBE_UNUSED logger_arg_s_cb *s = logger_arg_s;                     \
        MAYBE_UNUSED logger_arg_s__cb *s = logger_arg_s_;                   \
        MAYBE_UNUSED logger_arg_u_cb *u = logger_arg_u;                     \
        MAYBE_UNUSED logger_arg_x_cb *x = logger_arg_x;                     \
        MAYBE_UNUSED logger_arg_x0_cb *x0 = logger_arg_x0;                  \
        const Logger_Arg args[] = {__VA_ARGS__};                            \
        logger_fmt_args(fmt, format, args, sizeof(args) / sizeof(args[0])); \
    } while (0)

non_null()
void logger_write_args(
    const Logger *log, Logger_Level level, const char *file, uint32_t line, const char *func,
    const char *format, const Logger_Arg *args, uint8_t args_size);

#define LOGGER_WRITE_A(log, level, format, ...)                                       \
    do {                                                                              \
        if (level >= MIN_LOGGER_LEVEL) {                                              \
            MAYBE_UNUSED logger_arg_ud_cb *ud = logger_arg_ud;                        \
            MAYBE_UNUSED logger_arg_c_cb *c = logger_arg_c;                           \
            MAYBE_UNUSED logger_arg_d_cb *d = logger_arg_d;                           \
            MAYBE_UNUSED logger_arg_f_cb *f = logger_arg_f;                           \
            MAYBE_UNUSED logger_arg_s_cb *s = logger_arg_s;                           \
            MAYBE_UNUSED logger_arg_s__cb *s_ = logger_arg_s_;                        \
            MAYBE_UNUSED logger_arg_u_cb *u = logger_arg_u;                           \
            MAYBE_UNUSED logger_arg_x_cb *x = logger_arg_x;                           \
            MAYBE_UNUSED logger_arg_x0_cb *x0 = logger_arg_x0;                        \
            const Logger_Arg args[] = {__VA_ARGS__};                                  \
            logger_write_args(log, level, __FILE__, __LINE__, __func__, format, args, \
                sizeof(args) / sizeof(args[0]));                                      \
        }                                                                             \
    } while (0)

#define LOGGER_TRACE_A(log, ...)   LOGGER_WRITE_A(log, LOGGER_LEVEL_TRACE, __VA_ARGS__)
#define LOGGER_DEBUG_A(log, ...)   LOGGER_WRITE_A(log, LOGGER_LEVEL_DEBUG, __VA_ARGS__)
#define LOGGER_INFO_A(log, ...)    LOGGER_WRITE_A(log, LOGGER_LEVEL_INFO, __VA_ARGS__)
#define LOGGER_WARNING_A(log, ...) LOGGER_WRITE_A(log, LOGGER_LEVEL_WARNING, __VA_ARGS__)
#define LOGGER_ERROR_A(log, ...)   LOGGER_WRITE_A(log, LOGGER_LEVEL_ERROR, __VA_ARGS__)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_LOGGER_H */
