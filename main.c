#include <stdio.h>
#include <string.h>

#include "jxx.h"

#define JXX_IF_NULL_PERFORM(VALUE)\
    if((VALUE)->kind == JXX_VALUE_KIND_NULL)

#define JXX_IF_TRUE_PERFORM(VALUE)\
    if((VALUE)->kind == JXX_VALUE_KIND_TRUE)

#define JXX_IF_FALSE_PERFORM(VALUE)\
    if((VALUE)->kind == JXX_VALUE_KIND_FALSE)

#define JXX_IF_NUMBER_PERFORM(NEW_SYMBOL, VALUE)\
    for(Jxx_Number* NEW_SYMBOL = &(VALUE)->number; NEW_SYMBOL != NULL; NEW_SYMBOL = NULL)\
        if((VALUE)->kind == JXX_VALUE_KIND_NUMBER)

#define JXX_IF_STRING_PERFORM(NEW_SYMBOL, VALUE)\
    for(Jxx_String *NEW_SYMBOL = &(VALUE)->string; NEW_SYMBOL != NULL; NEW_SYMBOL = NULL)\
        if((VALUE)->kind == JXX_VALUE_KIND_STRING)

#define JXX_IF_ARRAY_PERFORM(NEW_SYMBOL, VALUE)\
    for(Jxx_Array *NEW_SYMBOL = &(VALUE)->array; NEW_SYMBOL != NULL; NEW_SYMBOL = NULL)\
        if((VALUE)->kind == JXX_VALUE_KIND_ARRAY)

#define JXX_IF_OBJECT_PERFORM(NEW_SYMBOL, VALUE)\
    for(Jxx_Object *NEW_SYMBOL = &(VALUE)->object; NEW_SYMBOL != NULL; NEW_SYMBOL = NULL)\
        if((VALUE)->kind == JXX_VALUE_KIND_OBJECT)

void test_assert(bool cond)
{
    if(!cond)
    {
        fprintf(stderr, "TEST ASSERTION FAILED!");
        exit(1);
    }
}

bool jxx_string_equals_cstr(const Jxx_String* s1, const char* s2)
{
    if(strlen(s2) != s1->length) return false;
    return memcmp(s1->data, s2, s1->length) == 0;
}

void test_parse_null()
{
    Jxx_Parser parser = { .text_ptr = "null" };
    Jxx_Value value;

    test_assert(jxx_parse(&parser, &value));
    test_assert(value.kind == JXX_VALUE_KIND_NULL);
}

void test_parse_true()
{
    Jxx_Parser parser = { .text_ptr = "true" };
    Jxx_Value value;

    test_assert(jxx_parse(&parser, &value));
    test_assert(value.kind == JXX_VALUE_KIND_TRUE);
}

void test_parse_false()
{
    Jxx_Parser parser = { .text_ptr = "false" };
    Jxx_Value value;

    test_assert(jxx_parse(&parser, &value));
    test_assert(value.kind == JXX_VALUE_KIND_FALSE);
}

void test_parse_number()
{
    Jxx_Parser parser = { .text_ptr = "123.456" };
    Jxx_Value value;

    test_assert(jxx_parse(&parser, &value));
    test_assert(value.kind == JXX_VALUE_KIND_NUMBER);
    test_assert(value.number == 123.456);
}

void test_parse_string()
{
    Jxx_Parser parser = { .text_ptr = "\"MyName\"" };
    Jxx_Value value;

    test_assert(jxx_parse(&parser, &value));
    test_assert(value.kind == JXX_VALUE_KIND_STRING);
    test_assert(jxx_string_equals_cstr(&value.string, "MyName"));
}

void test_parse_string_escape_sequences()
{
    Jxx_Parser parser = { .text_ptr = "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"" };
    Jxx_Value value;

    test_assert(jxx_parse(&parser, &value));
    test_assert(value.kind == JXX_VALUE_KIND_STRING);
    test_assert(jxx_string_equals_cstr(&value.string, "\" \\ / \b \f \n \r \t"));
}

void test_parse_array_0_elem()
{
    Jxx_Parser parser = { .text_ptr = "[]" };
    Jxx_Value value;

    test_assert(jxx_parse(&parser, &value));
    test_assert(value.kind == JXX_VALUE_KIND_ARRAY);
    test_assert(value.array.element_count == 0);
}

void test_parse_array_1_elem()
{
    Jxx_Parser parser = { .text_ptr = "[ true ]" };
    Jxx_Value value;

    test_assert(jxx_parse(&parser, &value));
    test_assert(value.kind == JXX_VALUE_KIND_ARRAY);
    test_assert(value.array.element_count == 1);
    test_assert(value.array.elements[0].kind == JXX_VALUE_KIND_TRUE);
}

void test_parse_array_n_elem()
{
    Jxx_Parser parser = { .text_ptr = "[ 1, 2, 3, 4, null ]" };
    Jxx_Value value;

    test_assert(jxx_parse(&parser, &value));
    test_assert(value.kind == JXX_VALUE_KIND_ARRAY);
    test_assert(value.array.element_count == 5);

    test_assert(value.array.elements[0].kind == JXX_VALUE_KIND_NUMBER);
    test_assert(value.array.elements[0].number == 1);
    
    test_assert(value.array.elements[1].kind == JXX_VALUE_KIND_NUMBER);
    test_assert(value.array.elements[1].number == 2);
    
    test_assert(value.array.elements[2].kind == JXX_VALUE_KIND_NUMBER);
    test_assert(value.array.elements[2].number == 3);

    test_assert(value.array.elements[3].kind == JXX_VALUE_KIND_NUMBER);
    test_assert(value.array.elements[3].number == 4);

    test_assert(value.array.elements[4].kind == JXX_VALUE_KIND_NULL);
}

void test_parse_object_0_pairs()
{
    Jxx_Parser parser = { .text_ptr = "{}" };
    Jxx_Value value;

    test_assert(jxx_parse(&parser, &value));
    test_assert(value.kind == JXX_VALUE_KIND_OBJECT);
    test_assert(value.object.pair_count == 0);
}

void test_parse_object_n_pairs()
{
    Jxx_Parser parser = { .text_ptr = "{ \"A\": 10, \"bbb\": \"my_str123\", \"HelloMyNameIs_Done\": true }" };
    Jxx_Value value;

    test_assert(jxx_parse(&parser, &value));
    test_assert(value.kind == JXX_VALUE_KIND_OBJECT);
    test_assert(value.object.pair_count == 3);

    test_assert(jxx_string_equals_cstr(&value.object.pairs[0].name, "A"));
    test_assert(value.object.pairs[0].value.kind == JXX_VALUE_KIND_NUMBER);
    test_assert(value.object.pairs[0].value.number == 10);

    test_assert(jxx_string_equals_cstr(&value.object.pairs[1].name, "bbb"));
    test_assert(value.object.pairs[1].value.kind == JXX_VALUE_KIND_STRING);
    test_assert(jxx_string_equals_cstr(&value.object.pairs[1].value.string, "my_str123"));
    
    test_assert(jxx_string_equals_cstr(&value.object.pairs[2].name, "HelloMyNameIs_Done"));
    test_assert(value.object.pairs[2].value.kind == JXX_VALUE_KIND_TRUE);
}

int main()
{
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_string_escape_sequences();
    test_parse_array_0_elem();
    test_parse_array_1_elem();
    test_parse_array_n_elem();
    test_parse_object_0_pairs();
    test_parse_object_n_pairs();
}