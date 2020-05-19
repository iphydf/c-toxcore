/*
 * C language compatibility macros for varying compiler support.
 */
#ifndef C_TOXCORE_TOXCORE_CCOMPAT_H
#define C_TOXCORE_TOXCORE_CCOMPAT_H

//!TOKSTYLE-

// Variable length arrays.
// VLA(type, name, size) allocates a variable length array with automatic
// storage duration. VLA_SIZE(name) evaluates to the runtime size of that array
// in bytes.
//
// If C99 VLAs are not available, an emulation using alloca (stack allocation
// "function") is used. Note the semantic difference: alloca'd memory does not
// get freed at the end of the declaration's scope. Do not use VLA() in loops or
// you may run out of stack space.
#if !defined(USE_ALLOCA) && !defined(_MSC_VER) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
// C99 VLAs.
#define VLA(type, name, size) type name[size]
#define SIZEOF_VLA sizeof
#else

// Emulation using alloca.
#ifdef _WIN32
#include <malloc.h>
#elif defined(__linux__)
#include <alloca.h>
#else
#include <stdlib.h>
#if !defined(alloca) && defined(__GNUC__)
#define alloca __builtin_alloca
#endif
#endif

#define VLA(type, name, size)                           \
  const size_t name##_size = (size) * sizeof(type);     \
  type *const name = (type *)alloca(name##_size)
#define SIZEOF_VLA(name) name##_size

#endif

#if !defined(__cplusplus) || __cplusplus < 201103L
#define nullptr NULL
#endif

#ifdef __GNUC__
#define GNU_PRINTF(f, a) __attribute__((__format__(__printf__, f, a)))
#else
#define GNU_PRINTF(f, a)
#endif

#include <stdint.h>

typedef const int8_t  int8;
typedef       int8_t  int8_mut;
typedef const int16_t int16;
typedef       int16_t int16_mut;
typedef const int32_t int32;
typedef       int32_t int32_mut;
typedef const int64_t int64;
typedef       int64_t int64_mut;

typedef const uint8_t  uint8;
typedef       uint8_t  uint8_mut;
typedef const uint16_t uint16;
typedef       uint16_t uint16_mut;
typedef const uint32_t uint32;
typedef       uint32_t uint32_mut;
typedef const uint64_t uint64;
typedef       uint64_t uint64_mut;

//!TOKSTYLE+

#endif // C_TOXCORE_TOXCORE_CCOMPAT_H
