#ifndef JXX_LIB_HEADER
#define JXX_LIB_HEADER

#include <stdlib.h>
#include <stdbool.h>

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

bool jxx_match_char(Jxx_Parser* parser, char c);
bool jxx_match_char_and_advance(Jxx_Parser* parser, char c);
bool jxx_match_string_and_advance(Jxx_Parser* parser, const char* cstr);
void jxx_skip_whitespace(Jxx_Parser* parser);

bool jxx_parse_null(Jxx_Parser* parser, Jxx_Value* out_value);
bool jxx_parse_true(Jxx_Parser* parser, Jxx_Value* out_value);
bool jxx_parse_false(Jxx_Parser* parser, Jxx_Value* out_value);
bool jxx_parse_number(Jxx_Parser* parser, Jxx_Value* out_value);
bool jxx_parse_string(Jxx_Parser* parser, Jxx_Value* out_value);
bool jxx_parse_array(Jxx_Parser* parser, Jxx_Value* out_value);
bool jxx_parse_object(Jxx_Parser* parser, Jxx_Value* out_value);
bool jxx_parse_value(Jxx_Parser* parser, Jxx_Value* out_value);

#endif
