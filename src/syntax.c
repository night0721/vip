#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "vip.h"

int is_separator(int c) {
	return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void update_highlight(row *row)
{
	row->hl = realloc(row->hl, row->render_size);
	memset(row->hl, HL_NORMAL, row->render_size);

	int prev_sep = 1;
	int i = 0;
	while (i < row->render_size) {
		char c = row->render[i];
		unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;
		if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
				(c == '.' && prev_hl == HL_NUMBER)) {
			row->hl[i] = HL_NUMBER;
			i++;
			prev_sep = 0;
			continue;
		}
		prev_sep = is_separator(c);
		i++;
	}}

char *syntax_to_color(int hl, size_t *len)
{
	switch (hl) {
		case HL_NUMBER: 
			*len = COLOR_LEN;
			return strdup(PEACH_BG);

		case HL_MATCH:;
					  char *str = malloc(COLOR_LEN * 2 + 1);
					  snprintf(str, COLOR_LEN * 2 + 1, "%s%s", BLACK_BG, SKY_FG);
					  *len = COLOR_LEN * 2;
					  return str;

		case HL_RESET:;
					  char *res = malloc(COLOR_LEN * 2 + 1);
					  snprintf(res, COLOR_LEN * 2 + 1, "%s%s", WHITE_BG, BLACK_FG);
					  *len = COLOR_LEN * 2;
					  return res;

		default: 
					  *len = COLOR_LEN;
					  return strdup(WHITE_BG);
	}
}

