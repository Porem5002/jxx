#include <ctype.h>
#include <string.h>

#include "jxx.h"

#define FFC_IMPL
#include "ffc.h"

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

bool jxx_parse_array(Jxx_Parser* parser, Jxx_Value* out_value)
{
    Jxx_Parser parser_snapshot = *parser;

    if(!jxx_match_char_and_advance(parser, '['))
        return false;

    Jxx_Array array = {0};

    jxx_skip_whitespace(parser);

    if(jxx_match_char_and_advance(parser, ']'))
        goto success;

    while(true)
    {
        Jxx_Value new_element;

        if(!jxx_parse_value(parser, &new_element))
            goto failure;

        // TODO: Handle allocation failure
        array.elements = realloc(array.elements, (array.element_count + 1) * sizeof(Jxx_Value));
        array.elements[array.element_count++] = new_element;

        if(jxx_match_char_and_advance(parser, ']'))
            break;

        if(!jxx_match_char_and_advance(parser, ','))
            goto failure;
    }

success:
    *out_value = (Jxx_Value){ .kind = JXX_VALUE_KIND_ARRAY, .array = array };
    return true;
failure:
    *parser = parser_snapshot;
    jxx_free_array(&array);
    return false;
}

bool jxx_parse_object(Jxx_Parser* parser, Jxx_Value* out_value)
{
    Jxx_Parser parser_snapshot = *parser;

    if(!jxx_match_char_and_advance(parser, '{'))
        return false;

    Jxx_Object object = {0};

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
        object.pairs = realloc(object.pairs, (object.pair_count + 1) * sizeof(Jxx_Pair));
        object.pairs[object.pair_count++] = (Jxx_Pair){ .name = pair_name.string, .value = pair_value };

        if(jxx_match_char_and_advance(parser, '}'))
            break;

        if(!jxx_match_char_and_advance(parser, ','))
            goto failure;
    }

success:
    *out_value = (Jxx_Value){ .kind = JXX_VALUE_KIND_OBJECT, .object = object };
    return true;
failure:
    *parser = parser_snapshot;
    jxx_free_object(&object);
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

void jxx_free_string(Jxx_String* string)
{
    free(string->data);
    memset(string, 0, sizeof(*string));
}

void jxx_free_array(Jxx_Array* array)
{
    for(size_t i = 0; i < array->element_count; i++)
        jxx_free_value(&array->elements[i]);

    free(array->elements);
    memset(array, 0, sizeof(*array));
}

void jxx_free_object(Jxx_Object* object)
{
    for(size_t i = 0; i < object->pair_count; i++)
    {
        jxx_free_string(&object->pairs[i].name);
        jxx_free_value(&object->pairs[i].value);
    }

    free(object->pairs);
    memset(object, 0, sizeof(*object));
}

void jxx_free_value(Jxx_Value* value)
{
    switch(value->kind)
    {
        case JXX_VALUE_KIND_STRING:
            jxx_free_string(&value->string);
            break;
        case JXX_VALUE_KIND_ARRAY:
            jxx_free_array(&value->array);
            break;
        case JXX_VALUE_KIND_OBJECT:
            jxx_free_object(&value->object);
            break;
        default:
            break;
    }

    value->kind = JXX_VALUE_KIND_NULL;
}
