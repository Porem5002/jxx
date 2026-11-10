#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#include <stdio.h>

#define FFC_IMPL
#include "ffc.h"

typedef struct Jxx_Value Jxx_Value;
typedef struct Jxx_Pair Jxx_Pair;

typedef double Jxx_Number;

typedef struct Jxx_String
{
    size_t length;
    char* data;
} Jxx_String;

typedef struct Jxx_Array
{
    size_t element_count;
    Jxx_Value* elements;
} Jxx_Array;

typedef struct Jxx_Object
{
    size_t pair_count;
    Jxx_Pair* pairs;
} Jxx_Object;

typedef enum Jxx_ValueKind
{
    JXX_VALUE_KIND_NULL,
    JXX_VALUE_KIND_TRUE,
    JXX_VALUE_KIND_FALSE,
    JXX_VALUE_KIND_NUMBER,
    JXX_VALUE_KIND_STRING,
    JXX_VALUE_KIND_ARRAY,
    JXX_VALUE_KIND_OBJECT,
} Jxx_ValueKind;

struct Jxx_Value
{
    Jxx_ValueKind kind;

    union
    {
        Jxx_Number number;
        Jxx_String string;
        Jxx_Array array;
        Jxx_Object object;
    };
};

typedef struct Jxx_Pair
{
    Jxx_String name;
    Jxx_Value value;
} Jxx_Pair;

typedef struct Jxx_Parser
{
    const char* text_ptr;
} Jxx_Parser;

bool jxx_match_char(Jxx_Parser* parser, char c)
{
    return *parser->text_ptr == c;
}

bool jxx_match_char_and_advance(Jxx_Parser* parser, char c)
{
    if(jxx_match_char(parser, c))
    {
        parser->text_ptr++;
        return true;
    }

    return false;
}

bool jxx_match_string_and_advance(Jxx_Parser* parser, const char* cstr)
{
    Jxx_Parser parser_snapshot = *parser;

    while(*cstr != '\0')
    {
        if(!jxx_match_char_and_advance(parser, *cstr))
        {
            *parser = parser_snapshot;
            return false;
        }

        cstr++;
    }

    return true;
}

void jxx_skip_whitespace(Jxx_Parser* parser)
{
    while(isspace(*parser->text_ptr))
        parser->text_ptr++;
}

bool jxx_parse_null(Jxx_Parser* parser, Jxx_Value* out_value)
{
    memset(out_value, 0, sizeof(*out_value));

    if(!jxx_match_string_and_advance(parser, "null"))
        return false;
    
    if(!isspace(*parser->text_ptr))
        return false;

    *out_value = (Jxx_Value){ .kind = JXX_VALUE_KIND_NULL };
    return true;
}

bool jxx_parse_true(Jxx_Parser* parser, Jxx_Value* out_value)
{
    memset(out_value, 0, sizeof(*out_value));

    if(!jxx_match_string_and_advance(parser, "true"))
        return false;
    
    if(!isspace(*parser->text_ptr))
        return false;

    *out_value = (Jxx_Value){ .kind = JXX_VALUE_KIND_TRUE };
    return true;
}

bool jxx_parse_false(Jxx_Parser* parser, Jxx_Value* out_value)
{
    memset(out_value, 0, sizeof(*out_value));

    if(!jxx_match_string_and_advance(parser, "false"))
        return false;
    
    if(!isspace(*parser->text_ptr))
        return false;

    *out_value = (Jxx_Value){ .kind = JXX_VALUE_KIND_FALSE };
    return true;
}

bool jxx_parse_number(Jxx_Parser* parser, Jxx_Value* out_value)
{
    memset(out_value, 0, sizeof(*out_value));

    double number_value;

    ffc_result result = ffc_parse_double(strlen(parser->text_ptr), parser->text_ptr, &number_value);
    if(result.outcome != FFC_OUTCOME_OK)
        return false;

    parser->text_ptr = result.ptr;
    *out_value = (Jxx_Value){ .kind = JXX_VALUE_KIND_NUMBER, .number = number_value };
    return true;
}

bool jxx_parse_string(Jxx_Parser* parser, Jxx_Value* out_value)
{
    memset(out_value, 0, sizeof(*out_value));

    if(!jxx_match_char_and_advance(parser, '\"'))
        return false;

    Jxx_Parser parser_snapshot = *parser;

    size_t string_length = 0;
    char* string_data = NULL;
    
    while(true)
    {
        char c = *parser->text_ptr;
        parser->text_ptr++;

        if(c == '\0' || isspace(c))
        {
            *parser = parser_snapshot;
            free(string_data);
            return false;
        }

        if(c == '\"')
            break;

        // TODO: Handle escape characters

        // TODO: Handle allocation failure
        string_data = realloc(string_data, string_length + 1);
        string_data[string_length++] = c;
    }

    *out_value = (Jxx_Value){
        .kind = JXX_VALUE_KIND_STRING,
        .string = {  .length = string_length, .data = string_data }
    };
    return true;
}

bool jxx_parse_value(Jxx_Parser* parser, Jxx_Value* out_value);

bool jxx_parse_array(Jxx_Parser* parser, Jxx_Value* out_value)
{
    Jxx_Parser parser_snapshot = *parser;

    if(!jxx_match_char_and_advance(parser, '['))
        return false;

    size_t element_count = 0;
    Jxx_Value* elements = NULL;

    jxx_skip_whitespace(parser);

    if(jxx_match_char_and_advance(parser, ']'))
        goto success;

    while(true)
    {
        Jxx_Value new_element;

        if(!jxx_parse_value(parser, &new_element))
            goto failure;

        // TODO: Handle allocation failure
        elements = realloc(elements, (element_count + 1) * sizeof(Jxx_Value));
        elements[element_count++] = new_element;

        if(jxx_match_char_and_advance(parser, ']'))
            break;

        if(!jxx_match_char_and_advance(parser, ','))
            goto failure;
    }

success:
    *out_value = (Jxx_Value){
        .kind = JXX_VALUE_KIND_ARRAY,
        .array = { .element_count = element_count, .elements = elements }
    };
    return true;
failure:
    *parser = parser_snapshot;
    // TODO: Clean up existing elements
    free(elements);
    return false;
}

bool jxx_parse_object(Jxx_Parser* parser, Jxx_Value* out_value)
{
    Jxx_Parser parser_snapshot = *parser;

    if(!jxx_match_char_and_advance(parser, '{'))
        return false;

    size_t pair_count = 0;
    Jxx_Pair* pairs = NULL;

    jxx_skip_whitespace(parser);
    
    if(jxx_match_char_and_advance(parser, '}'))
        goto failure;

    while(true)
    {
        Jxx_Value pair_name;
        Jxx_Value pair_value;

        jxx_skip_whitespace(parser);

        if(!jxx_parse_string(parser, &pair_name))
            goto failure;

        jxx_skip_whitespace(parser);

        if(!jxx_match_char_and_advance(parser, ':'))
            goto failure;

        if(!jxx_parse_value(parser, &pair_value))
            goto failure;

        // TODO: Handle allocation failure
        pairs = realloc(pairs, (pair_count + 1) * sizeof(Jxx_Pair));
        pairs[pair_count++] = (Jxx_Pair){ .name = pair_name.string, .value = pair_value };

        if(jxx_match_char_and_advance(parser, '}'))
            break;

        if(!jxx_match_char_and_advance(parser, ','))
            goto failure;
    }

success:
    *out_value = (Jxx_Value){
        .kind = JXX_VALUE_KIND_OBJECT,
        .object = { .pair_count = pair_count, .pairs = pairs }
    };
    return true;
failure:
    *parser = parser_snapshot;
    free(pairs);
    return false;
}

bool jxx_parse_value(Jxx_Parser* parser, Jxx_Value* out_value)
{
    Jxx_Parser parser_snapshot = *parser;

    jxx_skip_whitespace(parser);

    if(jxx_parse_null(parser, out_value) ||
       jxx_parse_true(parser, out_value) ||
       jxx_parse_false(parser, out_value) ||
       jxx_parse_number(parser, out_value) ||
       jxx_parse_string(parser, out_value) ||
       jxx_parse_array(parser, out_value) ||
       jxx_parse_object(parser, out_value))
    {
        jxx_skip_whitespace(parser);
        return true;
    }

    *parser = parser_snapshot;
    return false;
}

int main()
{
    Jxx_Parser parser = { .text_ptr = "{ \"hello\": 23.0, \"B\": 100.0 }" };
    Jxx_Value value;

    bool success = jxx_parse_value(&parser, &value);
    
    if(success)
        printf("SUCCESS!\n");
    else
        printf("FAILURE!\n");

    printf("\"%.*s\": %f\n", value.object.pairs[0].name.length, value.object.pairs[0].name.data, value.object.pairs[0].value.number);
    printf("\"%.*s\": %f\n", value.object.pairs[1].name.length, value.object.pairs[1].name.data, value.object.pairs[1].value.number);
}