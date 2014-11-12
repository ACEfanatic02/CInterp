#include "Lexer.h"
#include <cassert>
#include <cctype>

#define REPORT_ERROR(msg, ...) fprintf(stderr, "(%s:%d)" msg "\n", __FILE__, __LINE__, __VA_ARGS__)
#define REPORT_TOKEN_ERROR(msg, token, ...) fprintf(stderr, "(%s:%d)" msg "(%s:%d, %d)\n", __FILE__, __LINE__, __VA_ARGS__, token.filename, token.line_number, token.column_number)

static void make_token_text(Token * target, char * buffer, int length) {
    assert(length > 0);
    assert(buffer != NULL);

    if (target->text) {
        free(target->text);
    }

    target->text = (char *)calloc(1, length + 1);
    memcpy(target->text, buffer, length);
    target->text[length] = '\0';
}

static void set_token_symbol_text(Token * token) {
    assert(token->text == NULL);

    if (token->type > 256) {
        int index = token->type - 256;
        token->text = TOKEN_SYMBOL_TEXT[index];
    }
    else {
        // Single-character token.
        char * ptr = (char *)calloc(1, 2);
        ptr[0] = token->type;
        ptr[1] = '\0';
        token->text = ptr;
    }
}

char Lexer::next_char(FILE * fp) {
    assert(fp != NULL);

    char c = fgetc(fp);

	if (c == EOF) return '\0';
    if (c == '\n') {
        current_column = 1;
        current_line++;
    }
    else {
        current_column++;
    }
    return c;
}

char Lexer::peek_char(FILE * fp) {
    assert(fp != NULL);

	int prev_line = current_line;
	int prev_col = current_column;
	char c = next_char(fp);
    if (c != '\0') {
		assert(ungetc(c, fp) != EOF);
		current_line = prev_line;
		current_column = prev_col;
	}
    return c;
}

void Lexer::discard_line(FILE * fp) {
    assert(fp != NULL);

    int start_line = current_line;
    while (current_line == start_line) {
        if (!next_char(fp)) return;
    }
}

bool Lexer::lex_identifier(FILE * fp) {
    Token token;
    token.filename = current_filename;
	token.line_number = current_line;
	token.column_number = current_column;
    token.type = TOKEN_IDENT;
    char cur = peek_char(fp);
    char buf[256];
    int i = 0;
    while (cur && isalnum(cur)) {
        if (!(i < 256)) {
            REPORT_TOKEN_ERROR("Identifier exceeds limit of 256 characters.", token);
            return false;
        }
		next_char(fp); // eat current char

        buf[i] = cur;
        i++;
        cur = peek_char(fp);
    }

    make_token_text(&token, buf, i);
    tokens.push_back(token);
    return true;
}

bool Lexer::lex_number(FILE * fp) {
	Token token;
	token.filename = current_filename;
	token.line_number = current_line;
	token.column_number = current_column;

	char cur = peek_char(fp);
	char buf[256] = {0};
	int i = 0;
	bool is_hex = false;
	bool has_decimal_point = false;
	
	while (cur) {
		if (!(i < 256)) {
			REPORT_TOKEN_ERROR("Numeric literal too long.", token);
			return false;
		}

		if (!(isxdigit(cur) || cur == '.' || cur == 'x' || cur == 'X')) {
			break;
		}
		else {
			// actually eat char.
			next_char(fp);
		}

		if (cur == '.') {
			if (has_decimal_point) {
				REPORT_TOKEN_ERROR("Invalid floating point literal.", token);
				return false;
			}
			has_decimal_point = true;
		}

		if (cur == 'x' || cur == 'X') {
			if (has_decimal_point) {
				REPORT_TOKEN_ERROR("Hexidecimal floating point literals are not permitted.", token);
				return false;
			}
			else if (i != 1 || buf[0] != '0') {
				// hex prefix must be 0x or 0X
				REPORT_TOKEN_ERROR("Invalid hexidecimal literal.", token);
				return false;
			}

			is_hex = true;
		}

		if ((cur >= 'a' && cur <= 'f') ||
			(cur >= 'A' && cur <= 'F')) {
			if (has_decimal_point) {
				if (!(cur == 'f' || cur == 'F')) {
					REPORT_TOKEN_ERROR("Invalid floating point literal.", token);
					return false;
				}
			}
			else if (!is_hex) {
				REPORT_TOKEN_ERROR("Invalid hexidecimal literal.", token);
				return false;
			}
		}

		buf[i] = cur;
		i++;
		cur = peek_char(fp);
	}

	if (has_decimal_point) {
		token.type = TOKEN_FLOAT_LITERAL;
	}
	else {
		token.type = TOKEN_INTEGER_LITERAL;
	}
	make_token_text(&token, buf, i);
	tokens.push_back(token);
	return true;
}

bool Lexer::lex_char_literal(FILE * fp) {
    Token token;
    token.filename = current_filename;
	token.line_number = current_line;
	token.column_number = current_column;
    token.type = TOKEN_CHARACTER_LITERAL;
    char buf[4];
    int i;
    for (i = 0; i < 4; ++i) {
        buf[i] = next_char(fp);
        if (buf[i] == '\'' && i > 0) {
            break;
        }
    }
    if ((buf[i] != '\'') || 
        ((i == 3) && (buf[1] != '\\'))) {
        // Character literal longer than one (potentially escaped) character.
        REPORT_TOKEN_ERROR("Invalid character literal.", token);
        return false;
    }

    make_token_text(&token, buf, i + 1);
	tokens.push_back(token);
    return true;
}

bool Lexer::lex_string_literal(FILE * fp) {
    Token token;
    token.filename = current_filename;
	token.line_number = current_line;
	token.column_number = current_column;
    token.type = TOKEN_STRING_LITERAL;

    // Size limit is the same as used by MSVC.
    char buf[2048];
    int i = 0;
    char cur = next_char(fp);
    while (cur) {
        if (!(i < 2048)) {
            REPORT_TOKEN_ERROR("String literal too long, max 2048 characters.", token);
            free(buf);
            return false;
        }
        buf[i] = cur;

        if (i > 0 && cur == '\"') {
            // End quote.
            break;
        }
        if (cur == '\\') {
            // Escape character
            i++;
            buf[i] = next_char(fp);
        }

		i++;
        cur = next_char(fp);
    }

    make_token_text(&token, buf, i + 1);
	tokens.push_back(token);
    return true;
}

bool Lexer::lex_symbol(FILE * fp) {
    Token token;
    token.filename = current_filename;
	token.line_number = current_line;
	token.column_number = current_column;

    char cur = next_char(fp);
    switch (cur) {
    /* ARITHMATIC */
    case '+': {
        if (peek_char(fp) == '+') {
            token.type = TOKEN_INCREMENT;
            next_char(fp);
        }
        else if (peek_char(fp) == '=') {
            token.type = TOKEN_ADD_EQUALS;
            next_char(fp);
        }
        else {
            token.type = '+';
        }
    }
    break;
    case '-': {
        if (peek_char(fp) == '-') {
            token.type = TOKEN_DECREMENT;
            next_char(fp);
        }
        else if (peek_char(fp) == '=') {
            token.type == TOKEN_SUB_EQUALS;
            next_char(fp);
        }
        else if (peek_char(fp) == '>') {
            token.type = TOKEN_POINTER_MEMBER_DEREF;
            next_char(fp);
        }
        else {
            token.type = '-';
        }
    }
    break;
    case '/': {
        if (peek_char(fp) == '/') {
            // Comment.
            // @TODO: CLEANUP: this should really be handled elsewhere.
            discard_line(fp);
            return true;
        }
        else if (peek_char(fp) == '=') {
            token.type = TOKEN_DIV_EQUALS;
            next_char(fp);
        }
        else {
            token.type = '/';
        }
    }
    break;
    case '*': {
        if (peek_char(fp) == '=') {
            token.type = TOKEN_MUL_EQUALS;
            next_char(fp);
        }
        else {
            token.type = '*';
        }
    }
    break;
    case '%': {
        if (peek_char(fp) == '=') {
            token.type = TOKEN_MOD_EQUALS;
            next_char(fp);
        }
        else {
            token.type = '%';
        }
    }
    break;


    /* BRACKETS */
    case '(': 
    case ')':
    case '{':
    case '}':
    case '[':
    case ']': {
        token.type = cur;
    }
    break;
    

    /* COMPARISON */
    case '>': {
        if (peek_char(fp) == '=') {
            token.type = TOKEN_GREATER_EQUALS;
            next_char(fp);
        }
        else {
            token.type = '>';
        }
    }
    break;
    case '<': {
        if (peek_char(fp) == '=') {
            token.type = TOKEN_LESS_EQUALS;
            next_char(fp);
        }
        else {
            token.type = '<';
        }
    }
    break;
    case '=': {
        if (peek_char(fp) == '=') {
            token.type = TOKEN_EQUALS;
            next_char(fp);
        }
        else {
            token.type = '=';
        }
    }
    break;
    case '!': {
        if (peek_char(fp) == '=') {
            token.type = TOKEN_NOT_EQUALS;
            next_char(fp);
        }
        else {
            token.type = '!';
        }
    }
    break;

    
    /* BOOLEAN / BITWISE */
    case '|': {
        if (peek_char(fp) == '|') {
            token.type = TOKEN_LOGICAL_OR;
            next_char(fp);
        }
        else if (peek_char(fp) == '=') {
            token.type = TOKEN_BITWISE_OR_EQUALS;
            next_char(fp);
        }
        else {
            token.type = '|';
        }
    }
    break;
    case '&': {
        if (peek_char(fp) == '&') {
            token.type = TOKEN_LOGICAL_AND;
            next_char(fp);
        }
        else if (peek_char(fp) == '=') {
            token.type = TOKEN_BITWISE_AND_EQUALS;
            next_char(fp);
        }
        else {
            token.type = '&';
        }
    }
    break;
    case '~': {
        if (peek_char(fp) == '=') {
            token.type = TOKEN_NOT_EQUALS;
            next_char(fp);
        }
        else {
            token.type = '~';
        }
    }
    break;

    
    /* MISC */
    case '.': {
        token.type = '.';
    }
    break;
	case ';': {
		token.type = ';';
	}
	break;
    default:
        REPORT_TOKEN_ERROR("Lexer error: Unrecognized token.", token);
        break;
    }

    // Sanity check: type must be assigned.
    assert(token.type > 0);
	set_token_symbol_text(&token);
	tokens.push_back(token);
}

bool Lexer::lex_file(char * filename) {
    FILE * fp = fopen(filename, "r");
    if (fp == NULL) {
        REPORT_ERROR("Failed to open file [%s].", filename);
        return false;
    }

    current_filename = filename;

    char cur;
    while (true) {
        cur = peek_char(fp);
		while (cur && isspace(cur)) {
            next_char(fp);
			cur = peek_char(fp);
        }
        if (!cur) {
            break;
        }

        if (isalpha(cur)) {
            if (!lex_identifier(fp)) {    
                fclose(fp);
                return false;
            }
            continue;
        }
        else if (isdigit(cur)) {
            if (!lex_number(fp)) {
                fclose(fp);
                return false;
            }
            continue;
        }
        else if (cur == '\'') {
            if (!lex_char_literal(fp)) {
                fclose(fp);
                return false;
            }
            continue;
        }
        else if (cur == '\"') {
            if (!lex_string_literal(fp)) {
                fclose(fp);
                return false;
            }
            continue;
        }
		else if (cur == '\0') {
			assert(0);
		}
        else {
            if (!lex_symbol(fp)) {
                fclose(fp);
                return false;
            }
            continue;
        }
    }

    fclose(fp);
    return true;
}

void Lexer::print_tokens() {
	for (size_t i = 0; i < tokens.size(); ++i) {
		fprintf(stdout, "%s\n", tokens[i].text);
	}
}