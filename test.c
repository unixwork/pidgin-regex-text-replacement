/*
 * pidgin-regex-text-replacement
 *
 * Copyright (C) 2025 Olaf Wintermann
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "test.h"

int main(int argc, char **argv) {
    CxTestSuite *suite = cx_test_suite_new("regex-text-replacement");
    cx_test_register(suite, test_load_rules);
    cx_test_register(suite, test_str_replace);
    cx_test_register(suite, test_apply_rule);
    cx_test_run_stdout(suite);
    cx_test_suite_free(suite);
}

CX_TEST(test_load_rules) {
    FILE *testfile = fopen("testfile", "w");
    fputs("?v1\n", testfile);
    fputs("# comment\n", testfile);
    fputs("abc\t123\n", testfile);
    fputs("\\#escaped\t#replacement\n", testfile);
    fputs("pattern\treplacement", testfile);
    fclose(testfile);
    
    CX_TEST_DO {
        TextReplacementRule *rules;
        size_t nrules;
        int ret = load_rules("testfile", &rules, &nrules);
        CX_TEST_ASSERT(ret == 0);
        CX_TEST_ASSERT(rules);
        CX_TEST_ASSERT(nrules == 3);
        
        CX_TEST_ASSERT(!strcmp(rules[0].pattern, "abc"));
        CX_TEST_ASSERT(!strcmp(rules[0].replacement, "123"));
        
        CX_TEST_ASSERT(!strcmp(rules[1].pattern, "#escaped"));
        CX_TEST_ASSERT(!strcmp(rules[1].replacement, "#replacement"));
        
        CX_TEST_ASSERT(!strcmp(rules[2].pattern, "pattern"));
        CX_TEST_ASSERT(!strcmp(rules[2].replacement, "replacement"));
        
        free_rules(rules, nrules);
    }
    
    unlink("testfile");
}

#define LARGE_STR "aaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbb" \
                  "ccccccccccccccccccccccddddddddddddddddddddddddd" \
                  "eeeeeeeeeeeeeeeeeeeeeefffffffffffffffffffffffff" \
                  "gggggggggggggggggggggghhhhhhhhhhhhhhhhhhhhhhhhh"

CX_TEST(test_str_replace) {
    CX_TEST_DO {
        char *s = str_replace("hello $1 world $1 !", "$1", ":)");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "hello :) world :) !"));
        free(s);
        
        s = str_replace("{}", "{}", "replace_all");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "replace_all"));
        free(s);
        
        s = str_replace("no replacement", "123", "abc");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "no replacement"));
        free(s);
        
        s = str_replace("empty {placeholder} replacement", "{placeholder}", "");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "empty  replacement"));
        free(s);
        
        s = str_replace("axb", "x", LARGE_STR);
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "a" LARGE_STR "b"));
        free(s);
    }
}

CX_TEST(test_apply_rule) {
    TextReplacementRule rule0;
    rule0.pattern = "abc([0-9]*)";
    rule0.replacement = "id=$1";
    regcomp(&rule0.regex, rule0.pattern, REG_EXTENDED);
    
    CX_TEST_DO {
        char *in = g_strdup("hello abc123 test end");
        char *result = apply_rule(in, &rule0);
        
        CX_TEST_ASSERT(result != NULL);
        CX_TEST_ASSERT(!strcmp(result, "hello id=123 test end"));
    }
}
