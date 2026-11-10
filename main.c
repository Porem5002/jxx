#include <stdio.h>

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

    jxx_free_value(&value);
}