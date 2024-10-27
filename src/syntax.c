#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "vip.h"
#include "row.h"
#include "syntax.h"

char *c_keywords[] = { "switch", "if", "while", "for", "break", "continue", "return", "else", "struct", "union", "typedef", "static", "enum", "case", "sizeof", "int|", "long|", "double|", "float|", "char|", "unsigned|", "void|", NULL };
char *c_ext[] = { ".c", ".h", ".cpp", NULL };

language langs[] = {
	{
		"c",
		HL_NUMBERS | HL_STRINGS,
		"//",
		"/*",
		"*/",
		c_keywords,
		c_ext,
	},
};

#define LANGS_LEN (sizeof(langs) / sizeof(langs[0]))

int is_separator(int c)
{
	return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void update_highlight(row *row)
{
	row->hl = realloc(row->hl, row->render_size);
	memset(row->hl, NORMAL, row->render_size);

	if (vip.syntax == NULL) return;

	char **keywords = vip.syntax->keywords;

	char *scs = vip.syntax->singleline_comment_start;
	char *mcs = vip.syntax->multiline_comment_start;
	char *mce = vip.syntax->multiline_comment_end;

	int scs_len = scs ? strlen(scs) : 0;
	int mcs_len = mcs ? strlen(mcs) : 0;
	int mce_len = mce ? strlen(mce) : 0;

	int prev_sep = 1;
	int in_string = 0;
	int in_comment = (row->idx > 0 && vip.row[row->idx - 1].opened_comment);

	int i = 0;
	while (i < row->render_size) {
		char c = row->render[i];
		unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : NORMAL;

		if (scs_len && !in_string && !in_comment) {
			if (!strncmp(&row->render[i], scs, scs_len)) {
				memset(&row->hl[i], COMMENT, row->render_size - i);
				break;
			}
		}

		if (mcs_len && mce_len && !in_string) {
			if (in_comment) {
				row->hl[i] = MLCOMMENT;
				if (!strncmp(&row->render[i], mce, mce_len)) {
					memset(&row->hl[i], MLCOMMENT, mce_len);
					i += mce_len;
					in_comment = 0;
					prev_sep = 1;
					continue;
				} else {
					i++;
					continue;
				}
			} else if (!strncmp(&row->render[i], mcs, mcs_len)) {
				memset(&row->hl[i], MLCOMMENT, mcs_len);
				i += mcs_len;
				in_comment = 1;
				continue;
			}
		}

		if (vip.syntax->flags & HL_STRINGS) {
			if (in_string) {
				row->hl[i] = STRING;
				if (c == '\\' && i + 1 < row->render_size) {
					row->hl[i + 1] = STRING;
					i += 2;
					continue;
				}
				if (c == in_string) in_string = 0;
				i++;
				prev_sep = 1;
				continue;
			} else {
				if (c == '"') {
					in_string = c;
					row->hl[i] = STRING;
					i++;
					continue;
				}
			}
		}

		if (vip.syntax->flags & HL_NUMBERS) {
			if ((isdigit(c) && (prev_sep || prev_hl == NUMBER)) ||
					(c == '.' && prev_hl == NUMBER)) {
				row->hl[i] = NUMBER;
				i++;
				prev_sep = 0;
				continue;
			}
		}

		if (prev_sep) {
			int j;
			for (j = 0; keywords[j]; j++) {
				int klen = strlen(keywords[j]);
				int type_keyword = keywords[j][klen - 1] == '|';
				if (type_keyword) klen--;
				if (!strncmp(&row->render[i], keywords[j], klen) &&
						is_separator(row->render[i + klen])) {
					memset(&row->hl[i], type_keyword ? KEYWORD2 : KEYWORD1, klen);
					i += klen;
					break;
				}
			}
			if (keywords[j] != NULL) {
				prev_sep = 0;
				continue;
			}
		}

		prev_sep = is_separator(c);
		i++;
	}
	int changed = (row->opened_comment != in_comment);
	row->opened_comment = in_comment;
	if (changed && row->idx + 1 < vip.rows)
		update_highlight(&vip.row[row->idx + 1]);
}

char *syntax_to_color(int hl, size_t *len)
{
	switch (hl) {
		case NUMBER:
			*len = COLOR_LEN;
			return strdup(PEACH_BG);

		case STRING:
			*len = COLOR_LEN;
			return strdup(GREEN_BG);

		case COMMENT:
		case MLCOMMENT:
			*len = COLOR_LEN;
			return strdup(OVERLAY_0_BG);

		case KEYWORD1:
			*len = COLOR_LEN;
			return strdup(MAUVE_BG);

		case KEYWORD2:
			*len = COLOR_LEN;
			return strdup(YELLOW_BG);

		case MATCH:;
			char *str = malloc(COLOR_LEN * 2 + 1);
			snprintf(str, COLOR_LEN * 2 + 1, "%s%s", BLACK_BG, SKY_FG);
			*len = COLOR_LEN * 2;
			return str;

		case RESET:;
			char *res = malloc(COLOR_LEN * 2 + 1);
			snprintf(res, COLOR_LEN * 2 + 1, "%s%s", WHITE_BG, BLACK_FG);
			*len = COLOR_LEN * 2;
			return res;

		default: 
			*len = COLOR_LEN;
			return strdup(WHITE_BG);
	}
}

void select_syntax_highlight()
{
	vip.syntax = NULL;
	if (vip.filename == NULL) return;
	char *ext = strrchr(vip.filename, '.');
	for (unsigned int j = 0; j < LANGS_LEN; j++) {
		language *s = &langs[j];
		unsigned int i = 0;
		while (s->extensions[i]) {
			int is_ext = (s->extensions[i][0] == '.');
			if ((is_ext && ext && !strcmp(ext, s->extensions[i])) ||
					(!is_ext && strstr(vip.filename, s->extensions[i]))) {
				vip.syntax = s;

				for (int row = 0; row < vip.rows; row++) {
					update_highlight(&vip.row[row]);
				}
				return;
			}
			i++;
		}
	}
}

