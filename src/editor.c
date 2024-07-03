#include "vip.h"
#include "row.h"

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
