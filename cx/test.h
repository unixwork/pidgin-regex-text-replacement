/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
 *
 * Copyright 2017 Mike Becker, Olaf Wintermann All rights reserved.
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
 * @file: test.h
 * 
 * UCX Test Framework.
 * 
 * Usage of this test framework:
 *
 * **** IN HEADER FILE: ****
 *
 * <code>
 * CX_TEST(function_name);
 * CX_TEST_SUBROUTINE(subroutine_name, paramlist); // optional
 * </code>
 *
 * **** IN SOURCE FILE: ****
 * <code>
 * CX_TEST_SUBROUTINE(subroutine_name, paramlist) {
 *   // tests with CX_TEST_ASSERT()
 * }
 * 
 * CX_TEST(function_name) {
 *   // memory allocation and other stuff here
 *   #CX_TEST_DO {
 *     // tests with CX_TEST_ASSERT() and/or
 *     // calls with CX_TEST_CALL_SUBROUTINE() here
 *   }
 *   // cleanup of memory here
 * }
 * </code>
 * 
 * @attention Do not call own functions within a test, that use
 * CX_TEST_ASSERT() macros and are not defined by using CX_TEST_SUBROUTINE().
 *
 * @author Mike Becker
 * @author Olaf Wintermann
 *
 */

#ifndef UCX_TEST_H
#define	UCX_TEST_H

#include "common.h"

#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef __FUNCTION__
/**
 * Alias for the <code>__func__</code> preprocessor macro.
 * Some compilers use <code>__func__</code> and others use __FUNCTION__.
 * We use __FUNCTION__ so we define it for those compilers which use
 * <code>__func__</code>.
 */
#define __FUNCTION__ __func__
#endif

#if !defined(__clang__) && __GNUC__ > 3
#pragma GCC diagnostic ignored "-Wclobbered"
#endif

/** Type for the CxTestSuite. */
typedef struct CxTestSuite CxTestSuite;

/** Pointer to a test function. */
typedef void(*CxTest)(CxTestSuite *, void *, cx_write_func);

/** Type for the internal list of test cases. */
typedef struct CxTestSet CxTestSet;

/** Structure for the internal list of test cases. */
struct CxTestSet {
    
    /** Test case. */
    CxTest test;
    
    /** Pointer to the next list element. */
    CxTestSet *next;
};

/**
 * A test suite containing multiple test cases.
 */
struct CxTestSuite {
    
    /** The number of successful tests after the suite has been run. */
    unsigned int success;
    
    /** The number of failed tests after the suite has been run. */
    unsigned int failure;

    /** The optional name of this test suite. */
    const char *name;
    
    /**
     * Internal list of test cases.
     * Use cx_test_register() to add tests to this list.
     */
    CxTestSet *tests;
};

/**
 * Creates a new test suite.
 * @param name optional name of the suite
 * @return a new test suite
 */
cx_attr_nonnull
cx_attr_nodiscard
cx_attr_cstr_arg(1)
cx_attr_malloc
static inline CxTestSuite* cx_test_suite_new(const char *name) {
    CxTestSuite* suite = (CxTestSuite*) malloc(sizeof(CxTestSuite));
    if (suite != NULL) {
        suite->name = name;
        suite->success = 0;
        suite->failure = 0;
        suite->tests = NULL;
    }

    return suite;
}

/**
 * Deallocates a test suite.
 *
 * @param suite the test suite to free
 */
static inline void cx_test_suite_free(CxTestSuite* suite) {
    if (suite == NULL) return;
    CxTestSet *l = suite->tests;
    while (l != NULL) {
        CxTestSet *e = l;
        l = l->next;
        free(e);
    }
    free(suite);
}

/**
 * Registers a test function with the specified test suite.
 * 
 * @param suite the suite, the test function shall be added to
 * @param test the test function to register
 * @retval zero success
 * @retval non-zero failure
 */
cx_attr_nonnull
static inline int cx_test_register(CxTestSuite* suite, CxTest test) {
    CxTestSet *t = (CxTestSet*) malloc(sizeof(CxTestSet));
    if (t) {
        t->test = test;
        t->next = NULL;
        if (suite->tests == NULL) {
            suite->tests = t;
        } else {
            CxTestSet *last = suite->tests;
            while (last->next) {
                last = last->next;
            }
            last->next = t;
        }
        return 0;
    } else {
        return 1;
    }
}

/**
 * Runs a test suite and writes the test log to the specified stream.
 * @param suite the test suite to run
 * @param out_target the target buffer or file to write the output to
 * @param out_writer the write function writing to @p out_target
 */
cx_attr_nonnull
static inline void cx_test_run(CxTestSuite *suite,
                               void *out_target, cx_write_func out_writer) {
    if (suite->name == NULL) {
        out_writer("*** Test Suite ***\n", 1, 19, out_target);
    } else {
        out_writer("*** Test Suite : ", 1, 17, out_target);
        out_writer(suite->name, 1, strlen(suite->name), out_target);
        out_writer(" ***\n", 1, 5, out_target);
    }
    suite->success = 0;
    suite->failure = 0;
    for (CxTestSet *elem = suite->tests; elem; elem = elem->next) {
        elem->test(suite, out_target, out_writer);
    }
    out_writer("\nAll test completed.\n", 1, 21, out_target);
    char total[80];
    int len = snprintf(
            total, 80,
            "  Total:   %u\n  Success: %u\n  Failure: %u\n\n",
            suite->success + suite->failure, suite->success, suite->failure
    );
    out_writer(total, 1, len, out_target);
}

/**
 * Runs a test suite and writes the test log to the specified FILE stream.
 * @param suite (@c CxTestSuite*) the test suite to run
 * @param file (@c FILE*) the target file to write the output to
 */
#define cx_test_run_f(suite, file) cx_test_run(suite, (void*)file, (cx_write_func)fwrite)

/**
 * Runs a test suite and writes the test log to stdout.
 * @param suite (@c CxTestSuite*) the test suite to run
 */
#define cx_test_run_stdout(suite) cx_test_run_f(suite, stdout)

/**
 * Macro for a #CxTest function header.
 * 
 * Use this macro to declare and/or define a #CxTest function.
 * 
 * @param name the name of the test function
 */
#define CX_TEST(name) void name(CxTestSuite* _suite_,void *_output_, cx_write_func _writefnc_)

/**
 * Defines the scope of a test.
 *
 * @code
 * CX_TEST(my_test_name) {
 *     // setup code
 *     CX_TEST_DO {
 *         // your tests go here
 *     }
 *     // tear down code
 * }
 * @endcode
 *
 * @attention Any CX_TEST_ASSERT() calls must be performed in scope of
 * #CX_TEST_DO.
 */
#define CX_TEST_DO _writefnc_("Running ", 1, 8, _output_);\
        _writefnc_(__FUNCTION__, 1, strlen(__FUNCTION__), _output_);\
        _writefnc_("... ", 1, 4, _output_);\
        jmp_buf _env_;\
        for (unsigned int _cx_test_loop_ = 0 ;\
             _cx_test_loop_ == 0 && !setjmp(_env_);\
             _writefnc_("success.\n", 1, 9, _output_),\
             _suite_->success++, _cx_test_loop_++)

/**
 * Checks a test assertion.
 * If the assertion is correct, the test carries on. If the assertion is not
 * correct, the specified message (terminated by a dot and a line break) is
 * written to the test suites output stream.
 * @param condition (@c bool) the condition to check
 * @param message (@c char*) the message that shall be printed out on failure
 */
#define CX_TEST_ASSERTM(condition,message) if (!(condition)) { \
        const char *_assert_msg_ = message; \
        _writefnc_(_assert_msg_, 1, strlen(_assert_msg_), _output_); \
        _writefnc_(".\n", 1, 2, _output_); \
        _suite_->failure++; \
        longjmp(_env_, 1);\
    } (void) 0

/**
 * Checks a test assertion.
 * If the assertion is correct, the test carries on. If the assertion is not
 * correct, the specified message (terminated by a dot and a line break) is
 * written to the test suites output stream.
 * @param condition (@c bool) the condition to check
 */
#define CX_TEST_ASSERT(condition) CX_TEST_ASSERTM(condition, #condition " failed")

/**
 * Macro for a test subroutine function header.
 * 
 * Use this to declare and/or define a subroutine that can be called by using
 * CX_TEST_CALL_SUBROUTINE().
 * 
 * @param name the name of the subroutine
 * @param ... the parameter list
 * 
 * @see CX_TEST_CALL_SUBROUTINE()
 */
#define CX_TEST_SUBROUTINE(name,...) void name(CxTestSuite* _suite_,\
        void *_output_, cx_write_func _writefnc_, jmp_buf _env_, __VA_ARGS__)

/**
 * Macro for calling a test subroutine.
 * 
 * Subroutines declared with CX_TEST_SUBROUTINE() can be called by using this
 * macro.
 * 
 * @remark You may <b>only</b> call subroutines within a #CX_TEST_DO block.
 * 
 * @param name the name of the subroutine
 * @param ... the argument list
 * 
 * @see CX_TEST_SUBROUTINE()
 */
#define CX_TEST_CALL_SUBROUTINE(name,...) \
        name(_suite_,_output_,_writefnc_,_env_,__VA_ARGS__)

#ifdef	__cplusplus
}
#endif

#endif	/* UCX_TEST_H */

