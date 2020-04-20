#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum
{
    JSMN_UNDEFINED = 0,
    JSMN_OBJECT = 1,
    JSMN_ARRAY = 2,
    JSMN_STRING = 3,
    JSMN_PRIMITIVE = 4
} jsmntype_t;

enum jsmnerr
{
    /* Not enough tokens were provided */
    JSMN_ERROR_NOMEM = -1,
    /* Invalid character inside JSON string */
    JSMN_ERROR_INVAL = -2,
    /* The string is not a full JSON packet, more bytes expected */
    JSMN_ERROR_PART = -3
};

/**
 * JSON token description.
 * type		type (object, array, string etc.)
 * start	start position in JSON data string
 * end		end position in JSON data string
 */
typedef struct
{
    jsmntype_t type;
    int start;
    int end;
    int size;
#ifdef JSMN_PARENT_LINKS
    int parent;
#endif

#if defined(__cplusplus)
    int json_len() const
    {
        return end - start;
    }

    void get(char *dest, int dest_len, char const *json_text) const
    {
        int cp = json_len();
        if(cp >= dest_len) {
            cp = dest_len - 1;
        }
        memcpy(dest, json_text + start, cp);
        dest[cp] = 0;
    }

    bool get_flt(float &result, char const *json_text) const
    {
        if(type == JSMN_PRIMITIVE) {
            result = (float)atof(json_text + start);
            return true;
        }
        return false;
    }

    bool get_bool(bool &result, char const *json_text) const
    {
        if(type == JSMN_PRIMITIVE) {
            result = json_len() == 4 && strncmp(json_text + start, "true", 4) == 0;
            if(!result && (json_len() != 5 || strncmp(json_text + start, "false", 5) != 0)) {
                return false;
            }
            return true;
        }
        return false;
    }

    bool get_int(int &result, char const *json_text) const
    {
        if(type == JSMN_PRIMITIVE) {
            result = atoi(json_text + start);
            return true;
        }
        return false;
    }

    bool get_uint32(unsigned int &result, char const *json_text) const
    {
        if(type == JSMN_PRIMITIVE) {
            char *end;
            result = strtoul(json_text + start, &end, 10);
            return true;
        }
        return false;
    }

    template <typename T, size_t N> bool eq(T (&str)[N], char const *json_text) const
    {
        return type == JSMN_STRING && json_len() == (N - 1) && strncmp(json_text + start, str, N - 1) == 0;
    }

    template <typename T, size_t N> void get_str(T (&dest)[N], char const *json_text) const
    {
        get(dest, N, json_text);
    }

#endif
} jsmntok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
typedef struct
{
    unsigned int pos;     /* offset in the JSON string */
    unsigned int toknext; /* next token to allocate */
    int toksuper;         /* superior token node, e.g parent object or array */
} jsmn_parser;

/**
 * Create JSON parser over an array of tokens
 */
void jsmn_init(jsmn_parser *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
 * a single JSON object.
 */
int jsmn_parse(jsmn_parser *parser, const char *js, size_t json_len, jsmntok_t *tokens, unsigned int num_tokens);
