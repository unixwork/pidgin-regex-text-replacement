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

#define PURPLE_PLUGINS

#include "regex-text-replacement.h"
#include "ui.h"

#include <util.h> /* pidgin/util.h */

#include <errno.h>


static gboolean writing_chat_msg(PurpleAccount *account, const char *who,
                                 char **message, PurpleConversation *conv,
                                 PurpleMessageFlags flags);
static gboolean writing_im_msg(PurpleAccount *account, const char *who,
                               char **message, PurpleConversation *conv,
                               PurpleMessageFlags flags);
static void sending_im_msg(PurpleAccount *account, const char *receiver,
                           char **message);
static void sending_chat_msg(PurpleAccount *account, char **message, int id);


static TextReplacementRule *rules;
static size_t nrules;


static gboolean plugin_load(PurplePlugin *plugin) {
    char *file_path = rules_file_path();
    int err = load_rules(file_path, &rules, &nrules);
    free(file_path);
    if(err) {
        fprintf(stderr, "regex-text-replacement: load_rules failed\n");
        return TRUE;
    }
    
    void *conversation = purple_conversations_get_handle();
    // callbacks for handling writing to the conversation window locally
    purple_signal_connect(conversation, "writing-im-msg",
            plugin, PURPLE_CALLBACK(writing_im_msg), NULL);
    purple_signal_connect(conversation, "writing-chat-msg",
            plugin, PURPLE_CALLBACK(writing_chat_msg), NULL);
    // callback for handling outgoing messages
    purple_signal_connect_priority(conversation, "sending-im-msg",
            plugin, PURPLE_CALLBACK(sending_im_msg), NULL,
            PURPLE_SIGNAL_PRIORITY_DEFAULT);
    purple_signal_connect_priority(conversation, "sending-chat-msg",
            plugin, PURPLE_CALLBACK(sending_chat_msg), NULL,
            PURPLE_SIGNAL_PRIORITY_DEFAULT);
    return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin) {
    free_rules(rules, nrules);
    rules = NULL;
    nrules = 0;
    return TRUE;
}

static PidginPluginUiInfo ui_info =
{
    get_config_frame,
    0,

    /* padding */
    NULL,
    NULL,
    NULL,
    NULL
};

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,
    PIDGIN_PLUGIN_TYPE,
    0,
    NULL,
    PURPLE_PRIORITY_HIGHEST,

    "regex-text-replacement",
    "Regex Text Replacement",
    "0.9",

    "Replace text with regex rules",          
    "Replace text in outgoing messages with regex rules",          
    "Olaf Wintermann <olaf.wintermann@gmail.com>",                          
    "https://github.com/unixwork/pidgin-regex-text-replacement",     
    
    plugin_load,                   
    plugin_unload,                          
    NULL,                          
                                   
    &ui_info,                          
    NULL,                          
    NULL,                        
    NULL, 
    
    NULL,                          
    NULL,                          
    NULL,                          
    NULL                           
};
    
static void init_plugin(PurplePlugin *plugin)
{
    
}

PURPLE_INIT_PLUGIN(regex_text_replace, init_plugin, info)
        

static void writing_msg(char **message, PurpleMessageFlags flags) {
    // apply rules only on send
    if((flags & PURPLE_MESSAGE_SEND) == 0) {
        return;
    }
    apply_all_rules(message);
}

static gboolean writing_chat_msg(PurpleAccount *account, const char *who,
                                 char **message, PurpleConversation *conv,
                                 PurpleMessageFlags flags)
{
    writing_msg(message, flags);
    return FALSE;
}


static gboolean writing_im_msg(PurpleAccount *account, const char *who,
                               char **message, PurpleConversation *conv,
                               PurpleMessageFlags flags)
{
    writing_msg(message, flags);
    return FALSE;
}

static void sending_im_msg(PurpleAccount *account, const char *receiver,
                           char **message)
{
    apply_all_rules(message);
}

static void sending_chat_msg(PurpleAccount *account, char **message, int id) {
    apply_all_rules(message);
}

/* ------------------------------------------------------------------------- */

char *rules_file_path(void) {
    // get path to ~/.purple directory
    const char *user_dir = purple_user_dir();
    return g_build_filename(user_dir, REGEX_TEXT_REPLACEMENT_RULES_FILE, NULL);
}

int load_rules(const char *file, TextReplacementRule **rules, size_t *len) {
    *rules = NULL;
    *len = 0;
    
    FILE *in = fopen(file, "r");
    if(!in) {
        if(errno == ENOENT) {
            FILE *out = fopen(file, "w");
            fputs("?v1\n", out);
            fclose(out);
            return 0;
        }
        return 1;
    }
    
    size_t rules_alloc = 16;
    size_t rules_size = 0;
    TextReplacementRule *r = calloc(rules_alloc, sizeof(TextReplacementRule));
      
    char *line = NULL;
    size_t linelen = 0;
    
    // read format version
    size_t vlen = getline(&line, &linelen, in);
    if(vlen == 0) {
        free(line);
        fclose(in);
        return 0;
    }
    
    if(line[vlen-1] == '\n') {
        line[vlen-1] = 0;
    }
    if(strcmp(line, "?v1")) {
        fprintf(stderr, "Unknown file format version: %s\n", line);
        free(line);
        fclose(in);
        return 1;
    }
    
    // read rules
    while(getline(&line, &linelen, in) >= 0) {
        char *ln = line;
        size_t lnlen = strlen(ln);    
        
        // remove trailing newline
        if(lnlen > 0 && ln[lnlen-1] == '\n') {
            ln[lnlen-1] = '\0';
            lnlen--;
        }
        
        if(lnlen == 0) {
            continue;
        }
        
        // find first \t separator
        int separator = 0;
        for(int i=0;i<lnlen;i++) {
            if(ln[i] == '\t') {
                separator = i;
                break;
            }
        }
        
        // if a separator was found, we can add the rule
        if(separator > 0) {
            ln[separator] = '\0';
            
            char *pattern = strdup(ln);
            char *replacement = strdup(ln+separator+1);
            
            if(rules_size == rules_alloc) {
                rules_alloc *= 2;
                r = reallocarray(r, rules_alloc, sizeof(TextReplacementRule));
            }
            r[rules_size].pattern = pattern;
            r[rules_size].replacement = replacement;
            
            // compile the rule
            if(regcomp(&r[rules_size].regex, ln, REG_EXTENDED) == 0) {
                r[rules_size].compiled = TRUE;
            } else {
                fprintf(stderr, "Cannot compile pattern: %s\n", ln);
            }
            
            rules_size++;
        } else {
            fprintf(stderr, "Invalid text replacement rule: %s\n", ln);
        }
    }
    fclose(in);
    if(line) {
        free(line);
    }
    
    *rules = r;
    *len = rules_size;
    
    return 0;
}

TextReplacementRule* get_rules(size_t *numelm) {
    *numelm = nrules;
    return rules;
}

int rule_update_pattern(size_t index, char *new_pattern) {
    if(index >= nrules) {
        return 0;
    }
    TextReplacementRule *rule = &rules[index];
    free(rule->pattern);
    if(rule->compiled) {
        regfree(&rule->regex);
    }
    
    rule->pattern = strdup(new_pattern);
    if(strlen(new_pattern) > 0) {
        rule->compiled = regcomp(&rule->regex, new_pattern, REG_EXTENDED) == 0;
    } else {
        rule->compiled = 0;
    }
    
    return rule->compiled;
}

void rule_update_replacement(size_t index, char *new_replacement) {
    if(index >= nrules) {
        return;
    }
    TextReplacementRule *rule = &rules[index];
    free(rule->replacement);
    rule->replacement = strdup(new_replacement);
}

void rule_remove(size_t index) {
    if(index >= nrules) {
        fprintf(stderr, "rules array out of bounds: %d\n", (int)index);
        return;
    }
    TextReplacementRule *r = &rules[index];
    free(r->pattern);
    free(r->replacement);
    if(r->compiled) {
        regfree(&r->regex);
    }
    
    if(index+1 < nrules) {
        memmove(rules+index, rules+index+1, (nrules-index-1)*sizeof(TextReplacementRule));
    }
    nrules--;
}

void rule_move_up(size_t index) {
    if(index == 0 || index >= nrules) {
        return;
    }
    TextReplacementRule tmp = rules[index-1];
    rules[index-1] = rules[index];
    rules[index] = tmp;
}

void rule_move_down(size_t index) {
    if(index+1 >= nrules) {
        return;
    }
    TextReplacementRule tmp = rules[index+1];
    rules[index+1] = rules[index];
    rules[index] = tmp;
}

int save_rules(void) {
    char *path = rules_file_path();
    FILE *out = fopen(path, "w");
    free(path);
    if(!out) {
        return 1;
    }
    
    fputs("?v1\n", out);
    for(int i=0;i<nrules;i++) {
        TextReplacementRule *rule = &rules[i];
        if(rule->pattern && strlen(rule->pattern) > 0) {
            fprintf(out, "%s\t%s\n", rule->pattern, rule->replacement);
        }
    }
    
    fclose(out);
    return 0;
}

size_t add_empty_rule(void) {
    nrules++;
    rules = realloc(rules, nrules * sizeof(TextReplacementRule));
    memset(&rules[nrules-1], 0, sizeof(TextReplacementRule));
    return nrules;
}

void free_rules(TextReplacementRule *rules, size_t nelm) {
    for(size_t i=0;i<nelm;i++) {
        free(rules[i].pattern);
        free(rules[i].replacement);
        if(rules[i].compiled) {
            regfree(&rules[i].regex);
        }
    }
    free(rules);
}

char* str_unescape_and_replace(
        const char *in,
        const char *search,
        const char *replacement)
{
    size_t alloc = 1024;
    size_t pos = 0;
    char *newstr = malloc(alloc);
    
    size_t search_len = strlen(search);
    size_t replacement_len = strlen(replacement);
    
    int escaped = 0;
    int match = 0;
    char c;
    for(;(c = *in ) != '\0';in++) {
        int matchchar = 1;
        if(escaped) {
            switch(c) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '$': matchchar = 0; break; // don't match escaped $
            }
        } else if(!escaped && c == '\\') {
            escaped = 1;
            continue;
        }
        if(pos + match >= alloc) {
            alloc *= 2;
            newstr = realloc(newstr, alloc);
        }
        
        if(search_len > 0 && matchchar && c == search[match]) {
            match++;
            if(match == search_len) {
                if(pos + replacement_len + 1 >= alloc) {
                    alloc *= 2;
                    newstr = realloc(newstr, alloc);
                }
                memcpy(newstr + pos, replacement, replacement_len);
                pos += replacement_len;
                match = 0;
            }
        } else {
            if(match > 0) {
                // copy previously skipped characters
                memcpy(newstr + pos, in - match, match);
                pos += match;
            }
            match = 0;
            newstr[pos++] = c;
        }
        escaped = 0;
    }
    
    if(pos >= alloc) {
        alloc++;
        newstr = realloc(newstr, alloc);
    }
    newstr[pos] = '\0';
    
    return newstr;
}

char* apply_rule(char *msg_in, TextReplacementRule *rule) {
    size_t len = strlen(msg_in);
    char *in = msg_in;
    char *end = in+len;
    
    // find all occurences of the pattern
    size_t alloc = 0;
    size_t pos = 0;
    char *newstr = NULL;
    while(in < end) {
        regmatch_t matches[2];
        int ret = regexec(&rule->regex, in, 2, matches, 0);
        if(ret) {
            break;
        }
        
        // add anything before the match
        size_t cplen = pos + matches[0].rm_so;
        if(cplen >= alloc) {
            alloc += cplen + 1024;
            newstr = g_realloc(newstr, alloc);
        }
        if(cplen > 0) {
            memcpy(newstr+pos, in, cplen);
            pos += cplen;
        }
        
        // replace matches[0] with replacement
        // if a capture group exists, adjust the replacement
        char *rpl = rule->replacement;
        if(matches[1].rm_so >= 0) {
            size_t cg_len = matches[1].rm_eo - matches[1].rm_so;
            char *capture_group = malloc(cg_len + 1);
            memcpy(capture_group, in+matches[1].rm_so, cg_len);
            capture_group[cg_len] = 0;
            rpl = str_unescape_and_replace(rule->replacement, "$1", capture_group);
            free(capture_group);
        }
        size_t rpl_len = strlen(rpl);
        
        if(pos + rpl_len >= alloc) {
            alloc += rpl_len + 1024;
            newstr = g_realloc(newstr, alloc);
        }
        memcpy(newstr + pos, rpl, rpl_len);
        pos += rpl_len;
        if(rpl != rule->replacement) {
            free(rpl); // rpl was allocated by str_replace
        }
        
        in = in + matches[0].rm_eo;
    }
    
    // if no match was found, we can return the original msg ptr
    if(!newstr) {
        return msg_in;
    }
    
    // add remaining str
    size_t remaining = end - in;
    if(pos + remaining >= alloc) {
        alloc += remaining + 1;
        newstr = g_realloc(newstr, alloc);
    }
    if(remaining > 0) {
        memcpy(newstr+pos, in, remaining);
        pos += remaining;
    }
    
    if(pos >= alloc) {
        alloc++;
        newstr = g_realloc(newstr, alloc);
    }
    newstr[pos] = 0;
    
    g_free(msg_in);
    return newstr ? newstr : msg_in;
}

void apply_all_rules(char **msg) {
    char *msg_in = *msg;
    for(int i=0;i<nrules;i++) {
        if(rules[i].compiled) {
            msg_in = apply_rule(msg_in, &rules[i]);
        }
    }
    *msg = msg_in;
}
