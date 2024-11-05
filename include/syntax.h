#ifndef SYNTAX_H_
#define SYNTAX_H_

#include <stdio.h>

#include "row.h"

#define HL_NUMBERS (1 << 0)
#define HL_STRINGS (1 << 1)

typedef struct language {
	char *filetype;
	int flags;
	char *singleline_comment_start;
	char *multiline_comment_start;
	char *multiline_comment_end;
	char **keywords;
	char **extensions;
} language;

int is_separator(int c);
void update_highlight(row *row);
char *syntax_to_color(int hl, size_t *len);
void select_syntax_highlight(void);

#endif
