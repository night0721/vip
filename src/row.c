#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vip.h"

extern editor vip;

int row_cx_to_rx(row *row, int cx)
{
	int rx = 0;
	for (int j = 0; j < cx; j++) {
		if (row->chars[j] == '\t') {
			rx += (TAB_SIZE - 1) - (rx % TAB_SIZE);
		}
		rx++;
	}
	return rx;
}

void update_row(row *row)
{
	int tabs = 0;
	for (int j = 0; j < row->size; j++)
		if (row->chars[j] == '\t') tabs++;
	free(row->render);
	row->render = malloc(row->size + tabs * (TAB_SIZE - 1) + 1);
	int idx = 0;
	for (int j = 0; j < row->size; j++) {
		if (row->chars[j] == '\t') {
			row->render[idx++] = ' ';
			while (idx % TAB_SIZE != 0) {
				row->render[idx++] = ' ';
			}
		}
		else {
			row->render[idx++] = row->chars[j];
		}
	}
	row->render[idx] = '\0';
	row->render_size = idx;
}

void insert_row(int at, char *s, size_t len)
{
	if (at < 0 || at > vip.rows) return;

	vip.row = realloc(vip.row, sizeof(row) * (vip.rows + 1));
	memmove(&vip.row[at + 1], &vip.row[at], sizeof(row) * (vip.rows - at));

	vip.row[at].size = len;
	vip.row[at].chars = malloc(len + 1);
	memcpy(vip.row[at].chars, s, len);
	vip.row[at].chars[len] = '\0';

	vip.row[at].render_size = 0;
	vip.row[at].render = NULL;
	update_row(&vip.row[at]);

	vip.rows++;
	vip.dirty++;
}

void free_row(row *row)
{
	free(row->render);
	free(row->chars);
}

void del_row(int at)
{
	if (at < 0 || at >= vip.rows) return;
	free_row(&vip.row[at]);
	memmove(&vip.row[at], &vip.row[at + 1], sizeof(row) * (vip.rows - at - 1));
	vip.rows--;
	vip.dirty++;
}

void row_insert_char(row *row, int at, int c)
{
	if (at < 0 || at > row->size) {
		at = row->size;
	}
	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
	row->size++;
	row->chars[at] = c;
	update_row(row);
	vip.dirty++;
}

void row_append_str(row *row, char *s, size_t len)
{
	row->chars = realloc(row->chars, row->size + len + 1);
	memcpy(&row->chars[row->size], s, len);
	row->size += len;
	row->chars[row->size] = '\0';
	update_row(row);
	vip.dirty++;
}

void row_del_char(row *row, int at)
{
	if (at < 0 || at >= row->size) return;
	memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
	row->size--;
	update_row(row);
	vip.dirty++;
}

char *rows_to_str(int *buflen)
{
	int total_len = 0;
	for (int j = 0; j < vip.rows; j++) {
		total_len += vip.row[j].size + 1;
	}
	*buflen = total_len;
	char *buf = malloc(total_len);
	char *p = buf;
	for (int j = 0; j < vip.rows; j++) {
		memcpy(p, vip.row[j].chars, vip.row[j].size);
		p += vip.row[j].size;
		*p = '\n';
		p++;
	}
	return buf;
}
