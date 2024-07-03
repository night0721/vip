#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "vip.h"

void update_highlight(row *row)
{
	row->hl = realloc(row->hl, row->render_size);
	memset(row->hl, HL_NORMAL, row->render_size);
	for (int i = 0; i < row->render_size; i++) {
		if (isdigit(row->render[i])) {
			row->hl[i] = HL_NUMBER;
		}
	}
}

char *syntax_to_color(int hl, size_t *len)
{
	switch (hl) {
		case HL_NUMBER: 
			*len = COLOR_LEN;
			return strdup(PEACH_BG);

		case HL_MATCH:;
			char *str = malloc(COLOR_LEN * 2 + 1);
			snprintf(str, COLOR_LEN * 2 + 1, "%s%s", BLACK_BG, SKY_FG);
			str[COLOR_LEN * 2] = '\0';
			*len = COLOR_LEN * 2;
			return str;

		case HL_RESET:;
			char *res = malloc(COLOR_LEN * 2 + 1);
			snprintf(res, COLOR_LEN * 2 + 1, "%s%s", WHITE_BG, BLACK_FG);
			res[COLOR_LEN * 2] = '\0';
			*len = COLOR_LEN * 2;
			return res;

		default: 
			*len = COLOR_LEN;
			return strdup(WHITE_BG);
	}
}

