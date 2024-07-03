#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "vip.h"
#include "bar.h"
#include "row.h"
#include "term.h"

extern editor vip;

void insert_char(int c)
{
	if (vip.cy == vip.rows) {
		insert_row(vip.rows, "", 0);
	}
	row_insert_char(&vip.row[vip.cy], vip.cx, c);
	vip.cx++;
}

void insert_new_line()
{
	if (vip.cx == 0) {
		insert_row(vip.cy, "", 0);
	} else {
		row *row = &vip.row[vip.cy];
		insert_row(vip.cy + 1, &row->chars[vip.cx], row->size - vip.cx);
		row = &vip.row[vip.cy];
		row->size = vip.cx;
		row->chars[row->size] = '\0';
		update_row(row);
	}
	vip.cy++;
	vip.cx = 0;
}

/*
 * 'o' in vim
 */
void shift_new_line()
{
	insert_row(vip.cy + 1, "", 0);
	vip.cy++;
	vip.cx = 0;
}

void del_char()
{
	if (vip.cy == vip.rows) return;
	if (vip.cx == 0 && vip.cy == 0) return;

	row *row = &vip.row[vip.cy];
	if (vip.cx > 0) {
		row_del_char(row, vip.cx - 1);
		vip.cx--;
	} else {
		vip.cx = vip.row[vip.cy - 1].size;
		row_append_str(&vip.row[vip.cy - 1], row->chars, row->size);
		del_row(vip.cy);
		vip.cy--;
	}
}

void open_editor(char *filename)
{
	free(vip.filename);
	vip.filename = strdup(filename);
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		die("fopen");
	}
	char *line = NULL;
	size_t linecap = 0;
	ssize_t len;
	while ((len = getline(&line, &linecap, fp)) != -1) {
		/* remove new line and carriage return at end of line */
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
			len--;
		}
		insert_row(vip.rows, line, len);
	}
	free(line);
	fclose(fp);
	/* reset dirtiness as nothing is modified yet */
	vip.dirty = 0;
}

char *prompt_editor(char *prompt, void (*callback)(char *, int))
{
	size_t bufsize = 128;
	char *buf = malloc(bufsize);
	size_t buflen = 0;
	buf[0] = '\0';
	while (1) {
		set_status_bar_message(prompt, buf);
		refresh_screen();
		int c = read_key();
		if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
			if (buflen != 0) {
				buf[--buflen] = '\0';
			}
		} else if (c == '\x1b') {
			set_status_bar_message("");
			if (callback) callback(buf, c);
			free(buf);
			return NULL;
		} else if (c == '\r') {
			if (buflen != 0) {
				set_status_bar_message("");
				if (callback) callback(buf, c);
				return buf;
			}
		} else if (!iscntrl(c) && c < 128) {
			if (buflen == bufsize - 1) {
				bufsize *= 2;
				buf = realloc(buf, bufsize);
			}
			buf[buflen++] = c;
			buf[buflen] = '\0';
		}

		if (callback) callback(buf, c);
	}
}

void find_callback(char *query, int key)
{
	static int last_match = -1;
	static int direction = 1;

	static int saved_hl_line;
	static char *saved_hl = NULL;
	if (saved_hl) {
		memcpy(vip.row[saved_hl_line].hl, saved_hl, vip.row[saved_hl_line].render_size);
		free(saved_hl);
		saved_hl = NULL;
	}

	if (key == '\r' || key == '\x1b') {
		last_match = -1;
		direction = 1;
		return;
	} else if (key == CTRL_KEY('n')) {
		direction = 1;
	} else if (key == CTRL_KEY('p')) {
		direction = -1;
	} else {
		last_match = -1;
		direction = 1;
	}

	if (last_match == -1) direction = 1;
	int current = last_match;
	for (int i = 0; i < vip.rows; i++) {
		current += direction;

		if (current == -1) current = vip.rows - 1;
		else if (current == vip.rows) current = 0;

		row *row = &vip.row[current];
		char *match = strstr(row->render, query);
		if (match) {
			last_match = current;
			vip.cy = current;
			vip.cx = row_rx_to_cx(row, match - row->render);
			vip.rowoff = vip.rows;

			saved_hl_line = current;
			saved_hl = malloc(row->render_size);
			memcpy(saved_hl, row->hl, row->render_size);

			memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
			memset(&row->hl[match - row->render + strlen(query)], HL_RESET, row->render_size - (match - row->render + strlen(query)));
			break;
		}
	}
}

void find_editor()
{
	int tmp_cx = vip.cx;
	int tmp_cy = vip.cy;
	int tmp_coloff = vip.coloff;
	int tmp_rowoff = vip.rowoff;
	char *query = prompt_editor("/%s", find_callback);
	if (query) {
		free(query);
	} else {
		vip.cx = tmp_cx;
		vip.cy = tmp_cy;
		vip.coloff = tmp_coloff;
		vip.rowoff = tmp_rowoff;
	}
}
