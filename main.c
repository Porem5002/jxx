#include <stdio.h>

#include "jxx.h"

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