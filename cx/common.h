/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
 *
 * Copyright 2021 Mike Becker, Olaf Wintermann All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file common.h
 *
 * @brief Common definitions and feature checks.
 *
 * @author Mike Becker
 * @author Olaf Wintermann
 * @copyright 2-Clause BSD License
 *
 * @mainpage UAP Common Extensions
 * Library with common and useful functions, macros and data structures.
 * <p>
 * Latest available source:<br>
 * <a href="https://sourceforge.net/projects/ucx/files/">https://sourceforge.net/projects/ucx/files/</a>
 * </p>
 *
 * <p>
 * Repositories:<br>
 * <a href="https://sourceforge.net/p/ucx/code">https://sourceforge.net/p/ucx/code</a>
 * -&nbsp;or&nbsp;-
 * <a href="https://develop.uap-core.de/hg/ucx">https://develop.uap-core.de/hg/ucx</a>
 * </p>
 *
 * <h2>LICENCE</h2>
 *
 * Copyright 2021 Mike Becker, Olaf Wintermann All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UCX_COMMON_H
#define UCX_COMMON_H

/** Major UCX version as integer constant. */
#define UCX_VERSION_MAJOR   3

/** Minor UCX version as integer constant. */
#define UCX_VERSION_MINOR   1

/** Version constant which ensures to increase monotonically. */
#define UCX_VERSION (((UCX_VERSION_MAJOR)<<16)|UCX_VERSION_MINOR)

// ---------------------------------------------------------------------------
//       Common includes
// ---------------------------------------------------------------------------

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

// ---------------------------------------------------------------------------
//       Architecture Detection
// ---------------------------------------------------------------------------

#ifndef INTPTR_MAX
#error Missing INTPTR_MAX definition
#endif
#if INTPTR_MAX == INT64_MAX
/**
 * The address width in bits on this platform.
 */
#define CX_WORDSIZE 64
#elif INTPTR_MAX == INT32_MAX
/**
 * The address width in bits on this platform.
 */
#define CX_WORDSIZE 32
#else
#error Unknown pointer size or missing size macros!
#endif

// ---------------------------------------------------------------------------
//       Missing Defines
// ---------------------------------------------------------------------------

#ifndef SSIZE_MAX // not defined in glibc since C23 and MSVC
#if CX_WORDSIZE == 64
/**
 * The maximum representable value in ssize_t.
 */
#define SSIZE_MAX 0x7fffffffffffffffll
#else
#define SSIZE_MAX 0x7fffffffl
#endif
#endif


// ---------------------------------------------------------------------------
//       Attribute definitions
// ---------------------------------------------------------------------------

#ifndef __GNUC__
/**
 * Removes GNU C attributes where they are not supported.
 */
#define __attribute__(x)
#endif

/**
 * All pointer arguments must be non-NULL.
 */
#define cx_attr_nonnull __attribute__((__nonnull__))

/**
 * The specified pointer arguments must be non-NULL.
 */
#define cx_attr_nonnull_arg(...) __attribute__((__nonnull__(__VA_ARGS__)))

/**
 * The returned value is guaranteed to be non-NULL.
 */
#define cx_attr_returns_nonnull __attribute__((__returns_nonnull__))

/**
 * The attributed function always returns freshly allocated memory.
 */
#define cx_attr_malloc __attribute__((__malloc__))

#ifndef __clang__
/**
 * The pointer returned by the attributed function is supposed to be freed
 * by @p freefunc.
 *
 * @param freefunc the function that shall be used to free the memory
 * @param freefunc_arg the index of the pointer argument in @p freefunc
 */
#define cx_attr_dealloc(freefunc, freefunc_arg) \
    __attribute__((__malloc__(freefunc, freefunc_arg)))
#else
/**
 * Not supported in clang.
 */
#define cx_attr_dealloc(...)
#endif // __clang__

/**
 * Shortcut to specify #cxFree() as deallocator.
 */
#define cx_attr_dealloc_ucx cx_attr_dealloc(cxFree, 2)

/**
 * Specifies the parameters from which the allocation size is calculated.
 */
#define cx_attr_allocsize(...) __attribute__((__alloc_size__(__VA_ARGS__)))


#ifdef __clang__
/**
 * No support for @c null_terminated_string_arg in clang or GCC below 14.
 */
#define cx_attr_cstr_arg(idx)
/**
 * No support for access attribute in clang.
 */
#define cx_attr_access(mode, ...)
#else
#if __GNUC__ < 10
/**
 * No support for access attribute in GCC < 10.
 */
#define cx_attr_access(mode, ...)
#else
/**
 * Helper macro to define access macros.
 */
#define cx_attr_access(mode, ...) __attribute__((__access__(mode, __VA_ARGS__)))
#endif // __GNUC__ < 10
#if __GNUC__ < 14
/**
 * No support for @c null_terminated_string_arg in clang or GCC below 14.
 */
#define cx_attr_cstr_arg(idx)
#else
/**
 * The specified argument is expected to be a zero-terminated string.
 *
 * @param idx the index of the argument
 */
#define cx_attr_cstr_arg(idx) \
    __attribute__((__null_terminated_string_arg__(idx)))
#endif // __GNUC__ < 14
#endif // __clang__


/**
 * Specifies that the function will only read through the given pointer.
 *
 * Takes one or two arguments: the index of the pointer and (optionally) the
 * index of another argument specifying the maximum number of accessed bytes.
 */
#define cx_attr_access_r(...) cx_attr_access(__read_only__, __VA_ARGS__)

/**
 * Specifies that the function will read and write through the given pointer.
 *
 * Takes one or two arguments: the index of the pointer and (optionally) the
 * index of another argument specifying the maximum number of accessed bytes.
 */
#define cx_attr_access_rw(...) cx_attr_access(__read_write__, __VA_ARGS__)

/**
 * Specifies that the function will only write through the given pointer.
 *
 * Takes one or two arguments: the index of the pointer and (optionally) the
 * index of another argument specifying the maximum number of accessed bytes.
 */
#define cx_attr_access_w(...) cx_attr_access(__write_only__, __VA_ARGS__)

#if __STDC_VERSION__ >= 202300L

/**
 * Do not warn about unused variable.
 */
#define cx_attr_unused [[maybe_unused]]

/**
 * Warn about discarded return value.
 */
#define cx_attr_nodiscard [[nodiscard]]

#else // no C23

/**
 * Do not warn about unused variable.
 */
#define cx_attr_unused __attribute__((__unused__))

/**
 * Warn about discarded return value.
 */
#define cx_attr_nodiscard __attribute__((__warn_unused_result__))

#endif // __STDC_VERSION__

// ---------------------------------------------------------------------------
//       Useful function pointers
// ---------------------------------------------------------------------------

/**
 * Function pointer compatible with fwrite-like functions.
 */
typedef size_t (*cx_write_func)(
        const void *,
        size_t,
        size_t,
        void *
);

/**
 * Function pointer compatible with fread-like functions.
 */
typedef size_t (*cx_read_func)(
        void *,
        size_t,
        size_t,
        void *
);

// ---------------------------------------------------------------------------
//       Utility macros
// ---------------------------------------------------------------------------

/**
 * Determines the number of members in a static C array.
 *
 * @attention never use this to determine the size of a dynamically allocated
 * array.
 *
 * @param arr the array identifier
 * @return the number of elements
 */
#define cx_nmemb(arr) (sizeof(arr)/sizeof((arr)[0]))

// ---------------------------------------------------------------------------
//       szmul implementation
// ---------------------------------------------------------------------------

#if (__GNUC__ >= 5 || defined(__clang__)) && !defined(CX_NO_SZMUL_BUILTIN)
#define CX_SZMUL_BUILTIN
#define cx_szmul(a, b, result) __builtin_mul_overflow(a, b, result)
#else // no GNUC or clang bultin
/**
 * Performs a multiplication of size_t values and checks for overflow.
  *
 * @param a (@c size_t) first operand
 * @param b (@c size_t) second operand
 * @param result (@c size_t*) a pointer to a variable, where the result should
 * be stored
 * @retval zero success
 * @retval non-zero the multiplication would overflow
 */
#define cx_szmul(a, b, result) cx_szmul_impl(a, b, result)

/**
 * Implementation of cx_szmul() when no compiler builtin is available.
 *
 * Do not use in application code.
 *
 * @param a first operand
 * @param b second operand
 * @param result a pointer to a variable, where the result should
 * be stored
 * @retval zero success
 * @retval non-zero the multiplication would overflow
 */
#if __cplusplus
extern "C"
#endif
int cx_szmul_impl(size_t a, size_t b, size_t *result);
#endif // cx_szmul


// ---------------------------------------------------------------------------
//       Fixes for MSVC incompatibilities
// ---------------------------------------------------------------------------

#ifdef _MSC_VER
// fix missing ssize_t definition
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;

// fix missing _Thread_local support
#define _Thread_local __declspec(thread)
#endif // _MSC_VER

#endif // UCX_COMMON_H
