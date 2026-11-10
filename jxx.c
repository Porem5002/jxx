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

    *out_value = (Jxx_Value){ .kind = JXX_VALUE_KIND_NULL };
    return true;
}

bool jxx_parse_true(Jxx_Parser* parser, Jxx_Value* out_value)
{
    memset(out_value, 0, sizeof(*out_value));

    if(!jxx_match_string_and_advance(parser, "true"))
        return false;

    *out_value = (Jxx_Value){ .kind = JXX_VALUE_KIND_TRUE };
    return true;
}

bool jxx_parse_false(Jxx_Parser* parser, Jxx_Value* out_value)
{
    memset(out_value, 0, sizeof(*out_value));

    if(!jxx_match_string_and_advance(parser, "false"))
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

    Jxx_String string = {0};
    
    while(true)
    {
        char c = *parser->text_ptr;
        parser->text_ptr++;

        // The JSON spec does not allow strings to contain raw control characters (values from 0x00 to 0x1F)
        if(c >= 0x00 && c <= 0x1F)
            goto failure;

        if(c == '\"')
            break;

        if(c == '\\')
        {
            switch (*parser->text_ptr)
            {
                case '"': c = '"'; break;
                case '\\': c = '\\'; break;
                case '/': c = '/'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                // TODO: Handle hex escape sequence (\uFFFF) 
                default: goto failure;
            }

            *parser->text_ptr++;
        }

        char* new_string_data = realloc(string.data, string.length + 1);
        if(new_string_data == NULL)
            goto failure;

        string.data = new_string_data;
        string.data[string.length++] = c;
    }

    *out_value = (Jxx_Value){ .kind = JXX_VALUE_KIND_STRING, .string = string };
    return true;
failure:
    *parser = parser_snapshot;
    jxx_free_string(&string);
    return false;
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

        Jxx_Value* new_elements_buffer = realloc(array.elements, (array.element_count + 1) * sizeof(Jxx_Value));
        if(new_elements_buffer == NULL)
        {
            jxx_free_value(&new_element);
            goto failure;
        }

        array.elements = new_elements_buffer;
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
        goto success;

    while(true)
    {
        Jxx_Value pair_name;
        Jxx_Value pair_value;

        jxx_skip_whitespace(parser);

        if(!jxx_parse_string(parser, &pair_name))
            goto failure;

        jxx_skip_whitespace(parser);

        if(!jxx_match_char_and_advance(parser, ':'))
        {
            jxx_free_value(&pair_name);
            goto failure;
        }

        if(!jxx_parse_value(parser, &pair_value))
        {
            jxx_free_value(&pair_name);
            goto failure;
        }

        Jxx_Pair* new_pairs_buffer = realloc(object.pairs, (object.pair_count + 1) * sizeof(Jxx_Pair));
        if(new_pairs_buffer == NULL)
        {
            jxx_free_value(&pair_name);
            jxx_free_value(&pair_value);
            goto failure;
        }

        object.pairs = new_pairs_buffer;
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
