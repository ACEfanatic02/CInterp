#include "Lexer.h"
#include <cassert>

int main(int argc, char ** argv) {
	if (argc < 2) {
		fprintf(stderr, "You must provide a source file.\n");
		return 1;
	}

	Lexer * lexer = new Lexer;
/*	FILE * fp = fopen(argv[1], "r");
	while (!feof(fp)) {
		char c = lexer->peek_char(fp);
		assert(lexer->next_char(fp) == c);
	}
*/
	if (!lexer->lex_file(argv[1])) {
		fprintf(stderr, "Lexing file [%s] failed\n", argv[1]);
		return 1;
	}

	lexer->print_tokens();
	
	return 0;
}