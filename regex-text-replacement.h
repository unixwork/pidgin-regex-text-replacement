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

#ifndef RTR_H
#define RTR_H

#include <stdlib.h>
#include <sys/types.h>

#include <regex.h>

/* libpurple includes */
#include <notify.h>
#include <plugin.h>
#include <version.h>
#include <conversation.h>

/* pidgin includes */
#include <pidgin.h>
#include <gtkconv.h>

#define REGEX_TEXT_REPLACEMENT_RULES_FILE "regex-text-replacement.rules"

#ifdef DEBUG
#define DEBUG_PRINTF(...) printf( __VA_ARGS__ )
#else
#define DEBUG_PRINTF(...)
#endif

typedef struct TextReplacementRule {
    /*
     * regex pattern
     */
    char *pattern;
    
    /*
     * replacement string
     * If the string contains "$1", it is replaced with the first capture group
     * Escaping rules:
     * \$: "$"
     * \t: <tab>
     * \n: <newline>
     */
    char *replacement;
    
    /*
     * compiled regex
     */
    regex_t regex;
    
    /*
     * regex compiled successfully
     */
    int compiled;
} TextReplacementRule;

/*
 * returns path to ~/.purple/regex-text-replacement.rules
 * 
 * Must be freed with g_free
 */
char *rules_file_path(void);

/*
 * Loads text replacement rules from a rules definition file
 * 
 * Format:
 * # comment
 * <pattern>\t<replacement>
 * 
 */
int load_rules(const char *file, TextReplacementRule **rules, size_t *len);

/*
 * Frees a TextReplacementRule array, including all pattern and replacement
 * strings and the compiled regex pattern
 */
void free_rules(TextReplacementRule *rules, size_t nelm);

/*
 * returns the array of loaded text replacement rules
 */
TextReplacementRule* get_rules(size_t *numelm);

/*
 * replace the rule's pattern
 * returns 1 if the pattern was compiled successfully
 * returns 0 if the pattern couldn't be compiled
 */
int rule_update_pattern(size_t index, char *new_pattern);

/*
 * replace the rule's text replacement at the specified index
 */
void rule_update_replacement(size_t index, char *new_replacement);

/*
 * Remove a rule at the specified index
 */
void rule_remove(size_t index);

void rule_move_up(size_t index);
void rule_move_down(size_t index);

/*
 * adds a new empty rule to the rules array
 * returns the new size of the array
 */
size_t add_empty_rule(void);

/*
 * save loaded rules to ~/.purple/regex-text-replacement.rules 
 */
int save_rules(void);

/*
 * Replace all occurrences of str in in with replacement
 * 
 * Also unescapes: \t \n \$ \\
 */
char* str_unescape_and_replace(
        const char *in,
        const char *str,
        const char *replacement);

/*
 * Applies the text replacement rule to msg_in
 * If the rule pattern matches, msg_in is freed with (g_free) and a new string
 * is returned, allocated with g_malloc
 * If no match is found, the original string ptr is returned
 */
char* apply_rule(char *msg_in, TextReplacementRule *rule);

/*
 * apply all (compiled) rules to msg
 */
void apply_all_rules(char **msg);

#endif /* RTR_H */
