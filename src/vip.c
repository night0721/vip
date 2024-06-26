#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "vip.h"
#include "term.h"
#include "bar.h"
#include "editor.h"

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

editor vip;

int read_key()
{
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read");
		}
	}
	if (c == 'k') return ARROW_UP;
	if (c == 'j') return ARROW_DOWN;
	if (c == 'l') return ARROW_RIGHT;
	if (c == 'h') return ARROW_LEFT;

	if (c == '\x1b') {
		char seq[3];

		if (read(STDIN_FILENO, &seq[0], 1) != 1)
			return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1)
			return '\x1b';

		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1)
					return '\x1b';
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			} else {
				switch (seq[1]) {
					case 'A': return ARROW_UP;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'F': return END_KEY;
					case 'H': return HOME_KEY;
				}
			}
		} else if (seq[0] == 'O') {
			switch (seq[1]) {
				case 'F': return END_KEY;
				case 'H': return HOME_KEY;
			}
		}
		return '\x1b';
	} else {
		return c;
	}
}

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

void append_row(char *s, size_t len)
{
	vip.row = realloc(vip.row, sizeof(row) * (vip.rows + 1));

	int at = vip.rows;
	vip.row[at].size = len;
	vip.row[at].chars = malloc(len + 1);
	memcpy(vip.row[at].chars, s, len);
	vip.row[at].chars[len] = '\0';

	vip.row[at].render_size = 0;
	vip.row[at].render = NULL;
	update_row(&vip.row[at]);

	vip.rows++;
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
		append_row(line, len);
	}
	free(line);
	fclose(fp);
}

void abAppend(struct abuf *ab, const char *s, int len)
{
	char *new = realloc(ab->b, ab->len + len);
	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab)
{
	free(ab->b);
}

void scroll()
{
	vip.rx = 0;
	if (vip.cy < vip.rows) {
		vip.rx = row_cx_to_rx(&vip.row[vip.cy], vip.cx);
	}
	if (vip.cy < vip.rowoff) {
		vip.rowoff = vip.cy;
	}
	if (vip.cy >= vip.rowoff + vip.screenrows) {
		vip.rowoff = vip.cy - vip.screenrows + 1;
	}
	if (vip.rx < vip.coloff) {
		vip.coloff = vip.rx;
	}
	if (vip.rx >= vip.coloff + vip.screencols) {
		vip.coloff = vip.rx - vip.screencols + 1;
	}
}

void draw_rows(struct abuf *ab)
{
	for (int y = 0; y < vip.screenrows; y++) {
		int filerow = y + vip.rowoff;
		if (filerow >= vip.rows) {
			if (vip.rows == 0 && y == vip.screenrows / 3) {
				char welcome[11];
				int welcomelen = snprintf(welcome, sizeof(welcome),
						"VIP v%s", VERSION);
				if (welcomelen > vip.screencols)
					welcomelen = vip.screencols;
				int padding = (vip.screencols - welcomelen) / 2;
				if (padding) {
					abAppend(ab, "~", 1);
					padding--;
				}
				while (padding--) abAppend(ab, " ", 1);
				abAppend(ab, welcome, welcomelen);
			} else {
				abAppend(ab, "~", 1);
			}
		} else {
			int len = vip.row[filerow].render_size - vip.coloff;
			if (len < 0) len = 0;
			if (len > vip.screencols) len = vip.screencols;
			abAppend(ab, &vip.row[filerow].render[vip.coloff], len);
		}

		abAppend(ab, "\x1b[K", 3);
		abAppend(ab, "\r\n", 2);
	}
}

void refresh_screen()
{
	scroll();

	struct abuf ab = ABUF_INIT;

	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[H", 3);

	draw_rows(&ab);
	draw_status_bar(&ab);
	draw_message_bar(&ab);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (vip.cy - vip.rowoff) + 1,
			(vip.rx - vip.coloff) + 1);
	abAppend(&ab, buf, strlen(buf));

	abAppend(&ab, "\x1b[?25h", 6);

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

void move_cursor(int key)
{
	row *row = (vip.cy >= vip.rows) ? NULL : &vip.row[vip.cy];
	switch (key) {
		case ARROW_LEFT:
			if (vip.cx != 0) {
				vip.cx--;
			}
			break;
		case ARROW_RIGHT:
			if (row && vip.cx < row->size) {
				vip.cx++;
			}
			break;
		case ARROW_UP:
			if (vip.cy != 0) {
				vip.cy--;
			}
			break;
		case ARROW_DOWN:
			if (vip.cy < vip.rows) {
				vip.cy++;
			}
			break;
	}
	row = (vip.cy >= vip.rows) ? NULL : &vip.row[vip.cy];
	int rowlen = row ? row->size : 0;
	if (vip.cx > rowlen) {
		vip.cx = rowlen;
	}
}

void process_key()
{
	int c = read_key();
	switch (c) {
		case '\r':
			/* TBC */
			break;

		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;

		case HOME_KEY:
			vip.cx = 0;
			break;

		case END_KEY:
			if (vip.cy < vip.rows) {
				vip.cx = vip.row[vip.cy].size;
			}
			break;

		case BACKSPACE:
		case CTRL_KEY('h'):
		case DEL_KEY:
			/* TBC */
			break;

		case PAGE_UP:
		case PAGE_DOWN:
			{
				if (c == PAGE_UP) {
					vip.cy = vip.rowoff;
				} else if (c == PAGE_DOWN) {
					vip.cy = vip.rowoff + vip.screenrows - 1;
					if (vip.cy > vip.rows) vip.cy = vip.rows;
				}
				int times = vip.screenrows;
				while (times--)
					move_cursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
			}
			break;

		case ARROW_UP:
		case ARROW_DOWN:
		case ARROW_LEFT:
		case ARROW_RIGHT:
			move_cursor(c);
			break;

		case CTRL_KEY('l'):
		case '\x1b':
			break;

		default:
			insert_char(c);
			break;
	}
}

void init_editor()
{
	vip.cx = 0;
	vip.cy = 0;
	vip.rx = 0;
	vip.rowoff = 0;
	vip.coloff = 0;
	vip.rows = 0;
	vip.row = NULL;
	vip.filename = NULL;
	vip.statusmsg[0] = '\0';
	vip.statusmsg_time = 0;

	if (get_window_size(&vip.screenrows, &vip.screencols) == -1) {
		die("get_window_size");
	}
	vip.screenrows -= 2;
}

int main(int argc, char **argv)
{
	setup_term();
	init_editor();
	if (argc >= 2) {
		open_editor(argv[1]);
	}

	set_status_bar_message("Ctrl-Q = Quit");

	while (1) {
		refresh_screen();
		process_key();
	}
	return 0;
}
