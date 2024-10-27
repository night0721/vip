#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "editor.h"
#include "term.h"
#include "vip.h"
#include "bar.h"

int read_key()
{
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read");
		}
	}

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

void save_file()
{
	if (vip.filename == NULL) {
		vip.filename = prompt_editor("Save as: %s", NULL);
		if (vip.filename == NULL) {
			set_status_bar_message("Save aborted");
			return;
		}
		select_syntax_highlight();
	}
	int len;
	char *buf = rows_to_str(&len);
	int fd = open(vip.filename, O_RDWR | O_CREAT, 0644);
	if (fd != -1) {
		if (ftruncate(fd, len) != -1) {
			if (write(fd, buf, len) == len) {
				close(fd);
				free(buf);
				vip.dirty = 0;
				set_status_bar_message("\"%s\" %dL, %dB written", vip.filename, vip.rows, len);
				return;
			}
		}
		close(fd);
	}
	free(buf);
	set_status_bar_message("Error saving: %s", strerror(errno));
}

void process_key()
{
	int c = read_key();
	switch (c) {
		case '\r':
			insert_new_line();
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
		case DEL_KEY: /* PASSTHROUGH */
			if (vip.mode == INSERT) {
				if (c == DEL_KEY)
					move_cursor(ARROW_RIGHT);
				del_char();
			}
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
			if (vip.mode == INSERT) {
				move_cursor(c);
			}
			break;

		case CTRL_KEY('l'):
		case '\x1b':
			if (vip.mode == INSERT) {
				vip.mode = NORMAL;
				move_cursor(ARROW_LEFT);
			}
			break;

		case 'i': /* PASSTHROUGH */
			if (vip.mode == NORMAL) {
				vip.mode = INSERT;
				break;
			}

		case ':': /* PASSTHROUGH */
			if (vip.mode == NORMAL) {
				vip.mode = COMMAND;
				char *cmd = prompt_editor(":%s", NULL);
				if (cmd == NULL) {
					vip.mode = NORMAL;
					return;
				}
				switch (cmd[0]) {
					case 'q':
						if (cmd[1] == '!') {
							exit(0);
							break;
						} else {
							if (vip.dirty) {
								set_status_bar_message("No write since last change for buffer \"%s\"", vip.filename);
								vip.mode = NORMAL;
								return;
							}

							exit(0);
							break;

						}
					case 'w':
						save_file();
						vip.mode = NORMAL;
				}
			}
		case '/': /* PASSTHROUGH */
			if (vip.mode == NORMAL) {
				find_editor();
				break;
			}

		case 'k': /* PASSTHROUGH */
			if (vip.mode != INSERT) {
				move_cursor(ARROW_UP);
				break;
			}

		case 'j': /* PASSTHROUGH */
			if (vip.mode != INSERT) {
				move_cursor(ARROW_DOWN);
				break;
			}

		case 'l': /* PASSTHROUGH */
			if (vip.mode != INSERT) {
				move_cursor(ARROW_RIGHT);
				break;
			}

		case 'h': /* PASSTHROUGH */
			if (vip.mode != INSERT) {
				move_cursor(ARROW_LEFT);
				break;
			}

		case 'o': /* PASSTHROUGH */
			if (vip.mode == NORMAL) {
				shift_new_line();
				vip.mode = INSERT;
				break;
			}

		case '0': /* PASSTHROUGH */
			if (vip.mode == NORMAL) {
				vip.cx = 0;
				break;
			}
		
		case '$': /* PASSTHROUGH */
			if (vip.mode == NORMAL) {
				vip.cx = vip.row[vip.cy].size;
				break;
			}

		default:
			if (vip.mode == INSERT) {
				insert_char(c);
			}
			break;
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
			char *c = &vip.row[filerow].render[vip.coloff];
			unsigned char *hl = &vip.row[filerow].hl[vip.coloff];

			char *current_color = malloc(COLOR_LEN * 2);
			int current_color_len = 0;
			for (int j = 0; j < len; j++) {
				if (iscntrl(c[j])) {
					char sym = (c[j] <= 26) ? '@' + c[j] : '?';
					abAppend(ab, "\x1b[7m", 4);
					abAppend(ab, &sym, 1);
					abAppend(ab, "\x1b[m", 3);
					if (strncmp(current_color, WHITE_BG, COLOR_LEN)) {
						char *buf = malloc(COLOR_LEN * 2);
						memcpy(buf, current_color, current_color_len);
						abAppend(ab, buf, current_color_len);
						free(buf);
					}
				} else if (hl[j] == NORMAL) {
					if (strncmp(current_color, WHITE_BG, COLOR_LEN)) {
						memcpy(current_color, WHITE_BG, COLOR_LEN);
						current_color_len = COLOR_LEN;
						abAppend(ab, WHITE_BG, COLOR_LEN);
					}
					abAppend(ab, &c[j], 1);
				} else {
					size_t len;
					char *color = syntax_to_color(hl[j], &len);
					if (strncmp(current_color, color, len)) {
						memcpy(current_color, color, len);
						current_color_len = len;
						abAppend(ab, color, len);
					}
					free(color);
					abAppend(ab, &c[j], 1);
				}
			}
			free(current_color);
			abAppend(ab, WHITE_BG, COLOR_LEN);
		}

		abAppend(ab, "\x1b[K", 3);
		abAppend(ab, "\r\n", 2);
	}
}
