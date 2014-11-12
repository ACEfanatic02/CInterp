#pragma once

#include <vector>
#include <cstdio>

enum {
    // Single-character tokens are represented by their ASCII literal.
    TOKEN_IDENT = 256,

    TOKEN_INCREMENT,
    TOKEN_DECREMENT,

    TOKEN_ADD_EQUALS,
    TOKEN_SUB_EQUALS,
    TOKEN_DIV_EQUALS,
    TOKEN_MUL_EQUALS,
    TOKEN_MOD_EQUALS,

    TOKEN_GREATER_EQUALS,
    TOKEN_LESS_EQUALS,
    TOKEN_EQUALS,
    TOKEN_NOT_EQUALS,
    
    TOKEN_LOGICAL_OR,
    TOKEN_LOGICAL_AND,

    TOKEN_BITWISE_OR_EQUALS,
    TOKEN_BITWISE_AND_EQUALS,
    TOKEN_BITWISE_NOT_EQUALS,

    TOKEN_POINTER_MEMBER_DEREF,

    TOKEN_INTEGER_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_CHARACTER_LITERAL,
};
static char * TOKEN_SYMBOL_TEXT[] = {
    "TOKEN_IDENT",

    "++",
    "--",

    "+=",
    "-=",
    "/=",
    "*=",
    "%=",

    ">=",
    "<=",
    "==",
    "!=",

    "||",
    "&&",

    "|=",
    "&=",
    "~=",

    "->",
};

struct Token {
    char * filename;
    int line_number;
    int column_number;
    int type;
    char * text;

    Token() :
        filename(NULL),
        line_number(-1),
        column_number(-1),
        type(-1),
        text(NULL)
    {}
};

class Lexer {
public:
    char * current_filename;
    int current_line;
    int current_column;
    std::vector<Token> tokens;

    bool lex_identifier(FILE * fp);
    bool lex_number(FILE * fp);
    bool lex_char_literal(FILE * fp);
    bool lex_string_literal(FILE * fp);
    bool lex_symbol(FILE * fp);

    char next_char(FILE * fp);
    char peek_char(FILE * fp);
    void discard_line(FILE * fp);

public:
	Lexer() :
		current_filename(NULL),
		current_line(1),
		current_column(1)
	{}

    bool lex_file(char * filename);
	void print_tokens();
};