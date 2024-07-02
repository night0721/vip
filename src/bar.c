#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "vip.h"

extern editor vip;

void draw_status_bar(struct abuf *ab)
{
	abAppend(ab, "\x1b[7m", 4);
	abAppend(ab, "\x1b[1m", 4);
	abAppend(ab, BLUE_BG, 19);

	char mode[9] = " NORMAL ";
	int mode_len = 8;
	abAppend(ab, mode, 8);
	abAppend(ab, "\x1b[22m", 5);

	char git_branch[80], git_diff[80], file[80], lines[80], coord[80];
	int gitb_len = snprintf(git_branch, sizeof(git_branch), " %s ", "master");
	int gitd_len = snprintf(git_diff, sizeof(git_diff), " %s ", "+1");
	int file_len = snprintf(file, sizeof(file), " %.20s %s",
			vip.filename ? vip.filename : "[No Name]", vip.dirty ? "[+]" : "");
	int lines_len = snprintf(lines, sizeof(lines), " %dL ", vip.rows);
	int coord_len = snprintf(coord, sizeof(coord), " %d:%d ", vip.cy + 1, vip.rx + 1);

	abAppend(ab, SURFACE_1_BG, 16); /* background */
	abAppend(ab, BLUE_FG, 19); /* text */
	abAppend(ab, git_branch, gitb_len);
	abAppend(ab, "|", 1);
	abAppend(ab, GREEN_FG, 19);
	abAppend(ab, git_diff, gitd_len);
	abAppend(ab, BLACK_BG, 13);
	abAppend(ab, WHITE_FG, 19);
	abAppend(ab, file, file_len);


	while (file_len < vip.screencols) {
		if (vip.screencols - mode_len - file_len - gitb_len - gitd_len - 1 == lines_len + coord_len) {
			abAppend(ab, SURFACE_1_BG, 16);
			abAppend(ab, BLUE_FG, 19);
			abAppend(ab, lines, lines_len);
			abAppend(ab, BLUE_BG, 19);
			abAppend(ab, BLACK_FG, 13);
			abAppend(ab, "\x1b[1m", 4);
			abAppend(ab, coord, coord_len);
			abAppend(ab, "\x1b[22m", 5);
			break;
		} else {
			abAppend(ab, " ", 1);
			file_len++;
		}
	}
	abAppend(ab, "\x1b[m", 3);
	abAppend(ab, "\r\n", 2);
}

void draw_message_bar(struct abuf *ab)
{
	abAppend(ab, "\x1b[K", 3);
	int msglen = strlen(vip.statusmsg);
	if (msglen > vip.screencols) msglen = vip.screencols;
	if (msglen && time(NULL) - vip.statusmsg_time < 5)
		abAppend(ab, vip.statusmsg, msglen);
}

void set_status_bar_message(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(vip.statusmsg, sizeof(vip.statusmsg), fmt, ap);
	va_end(ap);
	vip.statusmsg_time = time(NULL);
}
