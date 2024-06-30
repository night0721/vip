#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "vip.h"

extern editor vip;

void draw_status_bar(struct abuf *ab)
{
	abAppend(ab, "\x1b[7m", 4);
	char status[80], rstatus[80];
	int len = snprintf(status, sizeof(status), "NORMAL | %.20s",
			vip.filename ? vip.filename : "[No Name]");
	int rlen = snprintf(rstatus, sizeof(rstatus), "%dL | %d:%d", vip.rows,
			vip.cy + 1, vip.rx + 1);
	if (len > vip.screencols) len = vip.screencols;
	abAppend(ab, status, len);
	while (len < vip.screencols) {
		if (vip.screencols - len == rlen) {
			abAppend(ab, rstatus, rlen);
			break;
		} else {
			abAppend(ab, " ", 1);
			len++;
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
