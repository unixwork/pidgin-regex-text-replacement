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
#include "ui.h"

int main(int argc, char **argv) {
    CxTestSuite *suite = cx_test_suite_new("regex-text-replacement");
    cx_test_register(suite, test_load_rules);
    cx_test_register(suite, test_str_unescape_and_replace);
    cx_test_register(suite, test_apply_rule);
    cx_test_run_stdout(suite);
    cx_test_suite_free(suite);
}

CX_TEST(test_load_rules) {
    FILE *testfile = fopen("testfile", "w");
    fputs("?v1\n", testfile);
    fputs("abc\t123\n", testfile);
    fputs("#nocomment\t#replacement\n", testfile);
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
        
        CX_TEST_ASSERT(!strcmp(rules[1].pattern, "#nocomment"));
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

CX_TEST(test_str_unescape_and_replace) {
    CX_TEST_DO {
        char *s = str_unescape_and_replace("hello $1 world $1 !", "$1", ":)");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "hello :) world :) !"));
        free(s);
        
        s = str_unescape_and_replace("{}", "{}", "replace_all");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "replace_all"));
        free(s);
        
        s = str_unescape_and_replace("no replacement", "123", "abc");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "no replacement"));
        free(s);
        
        s = str_unescape_and_replace("empty {placeholder} replacement", "{placeholder}", "");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "empty  replacement"));
        free(s);
        
        s = str_unescape_and_replace("axb", "x", LARGE_STR);
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "a" LARGE_STR "b"));
        free(s);
        
        s = str_unescape_and_replace("test abc abc abcdef end", "abcdef", "-");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "test abc abc - end"));
        free(s);
        
        s = str_unescape_and_replace("test \\$1 end", "$1", "fail");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "test $1 end"));
        free(s);
        
        s = str_unescape_and_replace("don't escape: \\$1 escape: $1", "$1", "escaped");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "don't escape: $1 escape: escaped"));
        free(s);
        
        s = str_unescape_and_replace("\\n", "", "");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "\n"));
        free(s);
        
        s = str_unescape_and_replace("\\r\\n\\t", "", "");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "\r\n\t"));
        free(s);
        
        s = str_unescape_and_replace("replace\\nnewline", "\n", "<br>");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "replace<br>newline"));
        free(s);
        
        s = str_unescape_and_replace("test1match\\nthistest2", "match\nthis", "<br>");
        CX_TEST_ASSERT(s != NULL);
        CX_TEST_ASSERT(!strcmp(s, "test1<br>test2"));
        free(s);
    }
}

CX_TEST(test_apply_rule) {
    TextReplacementRule rule0;
    rule0.pattern = "X([0-9]*)";
    rule0.replacement = "id=$1";
    regcomp(&rule0.regex, rule0.pattern, REG_EXTENDED);
    
    CX_TEST_DO {
        char *in = g_strdup("hello X123 test end");
        char *result = apply_rule(in, &rule0);
        CX_TEST_ASSERT(result != NULL);
        CX_TEST_ASSERT(!strcmp(result, "hello id=123 test end"));
        g_free(result);
        
        in = g_strdup("no pattern");
        result = apply_rule(in, &rule0);
        CX_TEST_ASSERT(result != NULL);
        CX_TEST_ASSERT(in == result);
        CX_TEST_ASSERT(!strcmp(result, "no pattern"));
        g_free(result);
        
        in = g_strdup("double X123 pattern X123");
        result = apply_rule(in, &rule0);
        CX_TEST_ASSERT(result != NULL);
        CX_TEST_ASSERT(!strcmp(result, "double id=123 pattern id=123"));
        g_free(result);
        
        in = g_strdup("multiple pattern X123 in X123 this X123 text X123 X123 X123 X123 X123 X123 end");
        result = apply_rule(in, &rule0);
        CX_TEST_ASSERT(result != NULL);
        CX_TEST_ASSERT(!strcmp(result, "multiple pattern id=123 in id=123 this id=123 text id=123 id=123 id=123 id=123 id=123 id=123 end"));
        g_free(result);
        
        in = g_strdup("different cg values X1 test X23 TEST X345 test X4567 TEST X56789 end");
        result = apply_rule(in, &rule0);
        CX_TEST_ASSERT(result != NULL);
        CX_TEST_ASSERT(!strcmp(result, "different cg values id=1 test id=23 TEST id=345 test id=4567 TEST id=56789 end"));
        g_free(result);
    }
    
    regfree(&rule0.regex);
}
