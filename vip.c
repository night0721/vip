#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/limits.h>

#include "config.h"

int cat_mode = 0;
struct termios newt, oldt;
int rows, cols;
editor_t editor[9];
int editors = 0;
editor_t *cur_editor;

void draw_status_bar(void);
void refresh_screen(void);
void move_xy(int key);
void insert_char(int c);
void insert_new_line(void);
void shift_new_line(void);
void del_char(void);
void init_editor(char *filename);
char *prompt_editor(char *prompt, void (*callback)(char *, int));
void find_callback(char *query, int key);
void find_editor(void);
int readch(void);
void save_file(void);
void draw_rows(void);
void update_row(row_t *row);
void insert_row(int at, char *s, size_t len);
void del_row(int at);
char *export_buffer(int *buflen);
void replace_home(char *path);
int is_symbol(int c);
int is_separator(int c);
void update_highlight(row_t *row);
void select_syntax(void);
void die(const char *s);
void cleanup(void);
void wpprintw(const char *fmt, ...);
void move_cursor(int row, int col);
int get_window_size(int *row, int *col);
void bprintf(const char *fmt, ...);

/*
 * Draw status bar
 * Like neovim's lualine
 */
void draw_status_bar(void)
{
	move_cursor(rows - 1, 1);
	/* Reverse and bold */
	bprintf("\033[7m\033[1m");

	int mode_len;
	switch (cur_editor->mode) {
		case NORMAL:
			bprintf("%s NORMAL ", BLUE_BG);
			mode_len = 8;
			break;
		case INSERT:
			bprintf("%s INSERT ", GREEN_BG);
			mode_len = 8;
			break;
		case COMMAND:
			bprintf("%s COMMAND ", PEACH_BG);
			mode_len = 9;
			break;
		case VISUAL:
			bprintf("%s VISUAL ", MAUVE_BG);
			mode_len = 8;
			break;
	}

	/* Reset */
	bprintf("\033[22m");

	char git[80], file[80], info[80], lines[80], coord[80];
	int git_len = snprintf(git, sizeof(git), " %s | %s ", "master", "+1");
	int file_len = snprintf(file, sizeof(file), " %s %s",
			strlen(cur_editor->filename) > 0 ? cur_editor->filename : "[No Name]", cur_editor->dirty ? "[+]" : "");
	int info_len = snprintf(info, sizeof(info), " %s ", cur_editor->syntax ? cur_editor->syntax->filetype : "");
	int lines_len;
	if (cur_editor->y == 0 || cur_editor->rows == cur_editor->y + 1) {
		lines_len = snprintf(lines, sizeof(lines), " %s ", cur_editor->y == 0 ? "Top" : "Bot");
	} else {
		lines_len = snprintf(lines, sizeof(lines), " %d%% ", ((cur_editor->y + 1) * 100 / cur_editor->rows));
	}
	int coord_len = snprintf(coord, sizeof(coord), " %d:%d ", cur_editor->y + 1, cur_editor->x + 1);

	bprintf(SURFACE_1_BG); /* background */
	/* text */
	switch (cur_editor->mode) {
		case NORMAL:
			bprintf("%s", BLUE_FG);
			break;
		case INSERT:
			bprintf("%s", GREEN_FG);
			break;
		case COMMAND:
			bprintf("%s", PEACH_FG);
			break;
		case VISUAL:
			bprintf("%s", MAUVE_FG);
			break;
	}
	bprintf("%s%s%s%s", git, BLACK_BG, WHITE_FG, file);

	while (file_len < cols) {
		if (cols - mode_len - git_len - file_len == info_len + lines_len + coord_len) {
			bprintf("%s%s", info, SURFACE_1_BG);
			if (cur_editor->mode == NORMAL) {
				bprintf(BLUE_FG);
			} else if (cur_editor->mode == INSERT) {
				bprintf(GREEN_FG);
			} else if (cur_editor->mode == COMMAND) {
				bprintf(PEACH_FG);
			} else if (cur_editor->mode == VISUAL) {
				bprintf(MAUVE_FG);
			}
			bprintf("%s", lines);
			if (cur_editor->mode == NORMAL) {
				bprintf(BLUE_BG);
			} else if (cur_editor->mode == INSERT) {
				bprintf(GREEN_BG);
			} else if (cur_editor->mode == COMMAND) {
				bprintf(PEACH_BG);
			} else if (cur_editor->mode == VISUAL) {
				bprintf(MAUVE_BG);
			}
			bprintf("%s\033[1m%s\033[22m", BLACK_FG, coord);
			break;
		} else {
			bprintf(" ");
			file_len++;
		}
	}
	bprintf("\033[m");
}

void refresh_screen(void)
{
	/* Scroll */
	cur_editor->rx = 0;
	if (cur_editor->y < cur_editor->rows) {
		for (int i = 0; i < cur_editor->x; i++) {
			if (cur_editor->row[cur_editor->y].chars[i] == '\t') {
				cur_editor->rx += (TAB_SIZE - 1) - (cur_editor->rx % TAB_SIZE);
			}
			cur_editor->rx++;
		}
	}
	if (cur_editor->y < cur_editor->rowoff) {
		cur_editor->rowoff = cur_editor->y;
	}
	if (cur_editor->y >= cur_editor->rowoff + rows - 3) {
		cur_editor->rowoff = cur_editor->y - rows + 3;
	}
	if (cur_editor->rx < cur_editor->coloff) {
		cur_editor->coloff = cur_editor->rx;
	}

	if (!cat_mode) {
		if (cur_editor->rx >= cur_editor->coloff + cols) {
			cur_editor->coloff = cur_editor->rx - cols + 1;
		}
		bprintf("\033H\033[2 q");
	}

	draw_rows();
	if (!cat_mode) {
		draw_status_bar();
		move_cursor((cur_editor->y - cur_editor->rowoff) + 1,
				(cur_editor->rx - cur_editor->coloff) + 1);
	}
}

void move_xy(int key)
{
	row_t *row = cur_editor->y >= cur_editor->rows ? NULL : &cur_editor->row[cur_editor->y];
	switch (key) {
		case ARROW_LEFT:
			if (cur_editor->x != 0) {
				cur_editor->x--;
			}
			break;
		case ARROW_RIGHT:
			if (row && cur_editor->x < row->size) {
				cur_editor->x++;
			}
			break;
		case ARROW_UP:
			if (cur_editor->y != 0) {
				cur_editor->y--;
			}
			break;
		case ARROW_DOWN:
			if (cur_editor->y < cur_editor->rows) {
				cur_editor->y++;
			}
			break;
	}
	/* Shifted row */
	row = cur_editor->y >= cur_editor->rows ? NULL : &cur_editor->row[cur_editor->y];
	int rowlen = row ? row->size : 0;
	if (cur_editor->x > rowlen) {
		cur_editor->x = rowlen;
	}
}

void insert_char(int c)
{
	row_t *row = &cur_editor->row[cur_editor->y];
	if (cur_editor->y == cur_editor->rows) {
		insert_row(cur_editor->rows, "", 0);
	}
	if (cur_editor->x < 0 || cur_editor->x > row->size) {
		cur_editor->x = row->size;
	}
	/* Insert char in row */
	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[cur_editor->x + 1], &row->chars[cur_editor->x], row->size - cur_editor->x + 1);
	row->size++;
	row->chars[cur_editor->x] = c;
	update_row(row);
	cur_editor->dirty++;
	cur_editor->x++;
}

void insert_new_line(void)
{
	if (cur_editor->x == 0) {
		insert_row(cur_editor->y, "", 0);
	} else {
		row_t *row = &cur_editor->row[cur_editor->y];
		insert_row(cur_editor->y + 1, &row->chars[cur_editor->x], row->size - cur_editor->x);
		row = &cur_editor->row[cur_editor->y];
		row->size = cur_editor->x;
		row->chars[row->size] = '\0';
		update_row(row);
	}
	cur_editor->y++;
	cur_editor->x = 0;
}

/*
 * 'O' in vim
 */
void shift_previous_line(void)
{
	insert_row(cur_editor->y, "", 0);
	cur_editor->x = 0;
}

/*
 * 'o' in vim
 */
void shift_new_line(void)
{
	insert_row(cur_editor->y + 1, "", 0);
	cur_editor->y++;
	cur_editor->x = 0;
}

void del_char(void)
{
	if (cur_editor->y == cur_editor->rows) return;
	if (cur_editor->x == 0 && cur_editor->y == 0) return;

	row_t *row = &cur_editor->row[cur_editor->y];
	if (cur_editor->x > 0) {
		/* Delete char in row */
		if (cur_editor->x - 1 < 0 || cur_editor->x - 1 >= row->size) return;
		memmove(&row->chars[cur_editor->x - 1], &row->chars[cur_editor->x], row->size - cur_editor->x + 1);
		row->size--;
		update_row(row);
		cur_editor->dirty++;
		cur_editor->x--;
	} else {
		row_t *prev_row = &cur_editor->row[cur_editor->y - 1];
		cur_editor->x = prev_row->size;
		/* Append string to previous row */
		prev_row->chars = realloc(prev_row->chars, prev_row->size + row->size + 1);
		memcpy(&prev_row->chars[prev_row->size], row->chars, row->size);
		prev_row->size += row->size;
		prev_row->chars[prev_row->size] = '\0';
		update_row(prev_row);
		cur_editor->dirty++;

		del_row(cur_editor->y);
		cur_editor->y--;
	}
}

void init_editor(char *filename)
{
	if (editors > 8) {
		wpprintw("%sOnly 9 tabs are allowed", ERROR);
		readch();
		wpprintw("");
		return;
	}
	cur_editor = &editor[editors++];
	cur_editor->x = 0;
	cur_editor->y = 0;
	cur_editor->rx = 0;
	cur_editor->rowoff = 0;
	cur_editor->coloff = 0;
	cur_editor->rows = 0;
	cur_editor->row = NULL;
	cur_editor->dirty = 0;
	cur_editor->mode = NORMAL;
	cur_editor->syntax = NULL;
	char cwd[PATH_MAX];
	getcwd(cwd, PATH_MAX);
	strcpy(cur_editor->cwd, cwd);

	if (filename) {
		strcpy(cur_editor->filename, basename(filename));

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
			insert_row(cur_editor->rows, line, len);
		}

		select_syntax();

		free(line);
		fclose(fp);
		/* Reset dirtiness as nothing is modified yet */
		cur_editor->dirty = 0;
	} else {
		strcpy(cur_editor->filename, "");
	}
}

char *prompt_editor(char *prompt, void (*callback)(char *, int))
{
	size_t bufsize = 128;
	char *buf = malloc(bufsize);
	size_t buflen = 0;
	int prompt_len = strlen(prompt);
	buf[0] = '\0';
	while (1) {
		wpprintw(prompt, buf);
		move_cursor(rows, prompt_len - 1 + strlen(buf));
		int c = readch();
		if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
			if (buflen != 0) {
				buf[--buflen] = '\0';
			}
		} else if (c == '\033') {
			wpprintw("");
			cur_editor->mode = NORMAL;
			if (callback) callback(buf, c);
			free(buf);
			return NULL;
		} else if (c == '\r') {
			if (buflen != 0) {
				wpprintw("");
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
		memcpy(cur_editor->row[saved_hl_line].hl, saved_hl, cur_editor->row[saved_hl_line].render_size);
		free(saved_hl);
		saved_hl = NULL;
	}

	if (key == '\r' || key == '\033') {
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
	for (int i = 0; i < cur_editor->rows; i++) {
		current += direction;

		if (current == -1) current = cur_editor->rows - 1;
		else if (current == cur_editor->rows) current = 0;

		row_t *row = &cur_editor->row[current];
		char *match = strstr(row->render, query);
		if (match) {
			last_match = current;
			cur_editor->y = current;
			int cur_rx = 0;
			int x;
			for (x = 0; x < row->size; x++) {
				if (row->chars[x] == '\t') {
					cur_rx += (TAB_SIZE - 1) - (cur_rx % TAB_SIZE);
				}
				cur_rx++;
				if (cur_rx > match - row->render) {
					cur_editor->x = x;
					break;
				}
			}
			cur_editor->rowoff = cur_editor->rows;

			saved_hl_line = current;
			saved_hl = malloc(row->render_size);
			memcpy(saved_hl, row->hl, row->render_size);

			memset(&row->hl[match - row->render], MATCH, strlen(query));
			memset(&row->hl[match - row->render + strlen(query)], RESET, row->render_size - (match - row->render + strlen(query)));
			break;
		}
	}
}

void find_editor(void)
{
	int tmp_x = cur_editor->x;
	int tmp_y = cur_editor->y;
	int tmp_coloff = cur_editor->coloff;
	int tmp_rowoff = cur_editor->rowoff;
	char *query = prompt_editor("/%s", find_callback);
	if (query) {
		free(query);
	} else {
		cur_editor->x = tmp_x;
		cur_editor->y = tmp_y;
		cur_editor->coloff = tmp_coloff;
		cur_editor->rowoff = tmp_rowoff;
	}
}

int readch(void)
{
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read");
		}
	}

	if (c == '\033') {
		char seq[3];

		if (read(STDIN_FILENO, &seq[0], 1) != 1)
			return '\033';
		if (read(STDIN_FILENO, &seq[1], 1) != 1)
			return '\033';

		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1)
					return '\033';
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
		return '\033';
	} else {
		return c;
	}
}

void save_file(void)
{
	if (strlen(cur_editor->filename) == 0) {
		char *new_filename = prompt_editor("Save as: %s", NULL);
		if (new_filename == NULL) {
			wpprintw("Save aborted");
			return;
		}
		strcpy(cur_editor->filename, new_filename);
		select_syntax();
	}
	int fd = open(cur_editor->filename, O_RDWR | O_CREAT, 0644);
	if (fd != -1) {
		int len;
		char *buf = export_buffer(&len);
		if (ftruncate(fd, len) != -1) {
			if (write(fd, buf, len) == len) {
				close(fd);
				free(buf);
				cur_editor->dirty = 0;
				wpprintw("\"%s\" %dL, %dB written", cur_editor->filename, cur_editor->rows, len);
				return;
			}
			free(buf);
		}
		close(fd);
	}
	wpprintw("Error saving: %s", strerror(errno));
}

void draw_rows(void)
{
	for (int y = 0; y < cur_editor->rows; y++) {
		if (!cat_mode && y > rows - 2) return;
		if (!cat_mode) move_cursor(y + 1, 1);
		int filerow = y + cur_editor->rowoff;
		if (filerow >= cur_editor->rows) {
			if (cur_editor->rows == 0 && y == rows / 2) {
				char welcome[11];
				snprintf(welcome, sizeof(welcome), "VIP v%s", VERSION);
				/* Length of welcome message must be 10 */
				int padding = (cols - 10) / 2;
				while (padding--)
					bprintf(" ");
				bprintf(welcome);
			}
		} else {
			int len = cur_editor->row[filerow].render_size - cur_editor->coloff;
			if (len < 0) len = 0;
			if (!cat_mode && len > cols) len = cols;
			char *c = &cur_editor->row[filerow].render[cur_editor->coloff];
			unsigned char *hl = &cur_editor->row[filerow].hl[cur_editor->coloff];

			for (int j = 0; j < len; j++) {
				if (iscntrl(c[j])) {
					bprintf("%s%c\033[m", OVERLAY_2_BG, '@' + c[j]);
				} else if (hl[j] == NORMAL) {
					bprintf("%s%c", WHITE_BG, c[j]);
				} else {
					switch (hl[j]) {
						case NUMBER:
							bprintf(PEACH_BG);
							break;

						case STRING:
							bprintf(GREEN_BG);
							break;

						case CHAR:
							bprintf(TEAL_BG);
							break;

						case ESCAPE:
							bprintf(PINK_BG);
							break;

						case SYMBOL:
							bprintf(SKY_BG);
							break;

						case TERMINATOR:
							bprintf(OVERLAY_2_BG);
							break;

						case COMMENT:
						case MLCOMMENT:
							bprintf("\033[3m");
							bprintf(OVERLAY_2_BG);
							break;

						case KW:
							bprintf(MAUVE_BG);
							break;

						case KW_TYPE:
							bprintf(YELLOW_BG);
							break;

						case KW_FN:
							bprintf(BLUE_BG);
							break;

						case KW_BRACKET:
							bprintf(RED_BG);
							break;

						case MATCH:
							bprintf(BLACK_BG);
							bprintf(SKY_FG);
							break;

						case RESET:
							bprintf(BLACK_BG);
							bprintf(SKY_FG);
							break;

						default: 
							bprintf(WHITE_BG);
					}
					bprintf("%c", c[j]);
					if (hl[j] == COMMENT || hl[j] == MLCOMMENT) {
						bprintf("\033[23m");
					}
				}
			}
			bprintf(WHITE_BG);
		}

		bprintf("\033[K\n");
	}
}

void update_row(row_t *row)
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
	update_highlight(row);
}

void insert_row(int at, char *s, size_t len)
{
	if (at < 0 || at > cur_editor->rows) return;

	cur_editor->row = realloc(cur_editor->row, sizeof(row_t) * (cur_editor->rows + 1));
	memmove(&cur_editor->row[at + 1], &cur_editor->row[at], sizeof(row_t) * (cur_editor->rows - at));
	for (int j = at + 1; j <= cur_editor->rows; j++) {
		cur_editor->row[j].idx++;
	}

	cur_editor->row[at].idx = at;
	cur_editor->row[at].size = len;
	cur_editor->row[at].chars = malloc(len + 1);
	memcpy(cur_editor->row[at].chars, s, len);
	cur_editor->row[at].chars[len] = '\0';
	cur_editor->row[at].hl = NULL;
	cur_editor->row[at].opened_comment = 0;
	cur_editor->row[at].render_size = 0;
	cur_editor->row[at].render = NULL;
	update_row(&cur_editor->row[at]);

	cur_editor->rows++;
	cur_editor->dirty++;
}

void del_row(int at)
{
	if (at < 0 || at >= cur_editor->rows) return;

	/* Free row contents */
	free(cur_editor->row[at].render);
	free(cur_editor->row[at].chars);
	free(cur_editor->row[at].hl);

	memmove(&cur_editor->row[at], &cur_editor->row[at + 1], sizeof(row_t) * (cur_editor->rows - at - 1));
	for (int j = at; j < cur_editor->rows - 1; j++) {
		cur_editor->row[j].idx--;
	}
	cur_editor->rows--;
	cur_editor->dirty++;
}

char *export_buffer(int *buflen)
{
	int total_len = 0;
	for (int j = 0; j < cur_editor->rows; j++) {
		total_len += cur_editor->row[j].size + 1;
	}
	*buflen = total_len;
	char *buf = malloc(total_len);
	char *p = buf;
	for (int j = 0; j < cur_editor->rows; j++) {
		memcpy(p, cur_editor->row[j].chars, cur_editor->row[j].size);
		p += cur_editor->row[j].size;
		*p = '\n';
		p++;
	}
	return buf;
}

void replace_home(char *path)
{
	char *home = getenv("HOME");
	if (home == NULL) {
		wpprintw("$HOME not defined");
		return;
	}
	/* replace ~ with home */
	snprintf(path, strlen(path) + strlen(home), "%s%s", home, path + 1);
	return;
}

int is_separator(int c)
{
	return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

int is_symbol(int c)
{
	return strchr("+-/*=~%><:?&|.", c) != NULL;
}

void update_highlight(row_t *row)
{
	row->hl = realloc(row->hl, row->render_size);
	memset(row->hl, NORMAL, row->render_size);

	if (cur_editor->syntax == NULL) return;

	char **keywords = cur_editor->syntax->keywords;

	char *scs = cur_editor->syntax->singleline_comment_start;
	char *mcs = cur_editor->syntax->multiline_comment_start;
	char *mce = cur_editor->syntax->multiline_comment_end;

	int scs_len = scs ? strlen(scs) : 0;
	int mcs_len = mcs ? strlen(mcs) : 0;
	int mce_len = mce ? strlen(mce) : 0;

	int prev_sep = 1;
	int in_string = 0;
	int in_char = 0;
	int in_include = 0;
	int in_escape = 0;
	int in_comment = row->idx > 0 && cur_editor->row[row->idx - 1].opened_comment;

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

		if (in_escape) {
			if ((c > 47 && c < 58) || c == 'n' || c == 't' || c == 'r') {
				row->hl[i] = ESCAPE;
				i++;
				prev_sep = 0;
				continue;
			} else {
				in_escape = 0;
			}
		} else {
			if (c == '\\') {
				in_escape = 1;
				row->hl[i] = ESCAPE;
				i++;
				continue;
			}
		}

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

		if (in_char) {
			row->hl[i] = CHAR;
			if (c == '\\' && i + 1 < row->render_size) {
				row->hl[i + 1] = CHAR;
				i += 2;
				continue;
			}
			if (c == in_char) in_char = 0;
			i++;
			prev_sep = 1;
			continue;
		} else {
			if (c == '\'') {
				in_char = c;
				row->hl[i] = CHAR;
				i++;
				continue;
			}
		}

		if (in_include) {
			row->hl[i] = STRING;
			if (c == '>') in_include = 0;
			i++;
			prev_sep = 1;
			continue;
		} else {
			if (c == '<' && (row->render[i-1] == 'e' || (row->render[i-1] == ' ' &&
							row->render[i-2] == 'e'))) {
				in_include = 1;
				row->hl[i] = STRING;
				i++;
				continue;
			}
		}	

		if ((isdigit(c) && (prev_sep || prev_hl == NUMBER)) ||
				(c == '.' && prev_hl == NUMBER) ||
				(c >= 'A' && c <= 'Z') ||
				(c == '_' && prev_hl == NUMBER)
		   ) {
			row->hl[i] = NUMBER;
			i++;
			prev_sep = 0;
			continue;
		}

		if (c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}') {
			row->hl[i] = KW_BRACKET;
			prev_sep = 1;
			i++;
			continue;
		}

		if (is_symbol(c)) {
			row->hl[i] = SYMBOL;
			prev_sep = 1;
			i++;
			continue;
		}

		if (c == ';' || c == ',') {
			row->hl[i] = TERMINATOR;
			prev_sep = 1;
			i++;
			continue;
		}

		if (prev_sep) {
			int j;
			for (j = 0; keywords[j]; j++) {
				int klen = strlen(keywords[j]);
				int type_keyword = keywords[j][klen - 1] == '|';
				if (type_keyword) klen--;

				if (!strncmp(&row->render[i], keywords[j], klen) &&
						is_separator(row->render[i + klen])) {
					memset(&row->hl[i], type_keyword ? KW_TYPE : KW, klen);
					i += klen;
					break;
				}
			}
			/* Matched a keyword */
			if (keywords[j] != NULL) {
				prev_sep = 0;
				continue;
			}
			/* Check for function */
			int word_len = 0;
			while (!is_separator(row->render[i])) {
				word_len++;
				i++;
			}
			if (row->render[i] == '(') {
				memset(&row->hl[i - word_len], KW_FN, word_len);
				prev_sep = 1;
			} else {
				prev_sep = 0;
			}
			continue;
		}

		prev_sep = is_separator(row->render[i]);
		i++;
	}
	int changed = (row->opened_comment != in_comment);
	row->opened_comment = in_comment;
	if (changed && row->idx + 1 < cur_editor->rows)
		update_highlight(&cur_editor->row[row->idx + 1]);
}

void select_syntax(void)
{
	if (strlen(cur_editor->filename) == 0) return;
	cur_editor->syntax = NULL;
	char *ext = strrchr(cur_editor->filename, '.');
	for (uint8_t i = 0; i < LANGS_LEN; i++) {
		language_t *lang = &langs[i];
		for (int j = 0; lang->extensions[j] != NULL; j++) {
			int is_ext = lang->extensions[j][0] == '.';
			if ((is_ext && ext && !strcmp(ext, lang->extensions[j])) ||
					(!is_ext && strstr(cur_editor->filename, lang->extensions[j]))) {
				cur_editor->syntax = lang;

				for (int row = 0; row < cur_editor->rows; row++) {
					update_highlight(&cur_editor->row[row]);
				}
				return;
			}
		}
	}
}

void die(const char *s)
{
	perror(s);
	exit(1);
}

void handle_sigwinch(int ignore)
{
	get_window_size(&rows, &cols);
	if (cols < 80 || rows < 24) {
		die("vip: Terminal size needs to be at least 80x24");
	}
	refresh_screen();
}

int main(int argc, char **argv)
{
	if (argc > 2 && !strcmp(argv[1], "-c")) {
		cat_mode = 1;
	} else {
		struct sigaction sa;
		sa.sa_handler = handle_sigwinch;
		sa.sa_flags = SA_RESTART;
		sigemptyset(&sa.sa_mask);

		if (sigaction(SIGWINCH, &sa, NULL) == -1) {
			perror("sigaction");
			exit(1);
		}

		if (tcgetattr(STDIN_FILENO, &oldt) == -1) {
			die("tcgetattr");
		}
		if (get_window_size(&rows, &cols) == -1) {
			die("get_window_size");
		}
		bprintf("\033[?1049h\033[2J\033[2q");

		newt = oldt;
		/* Disable canonical mode and echo */
		newt.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
		newt.c_cflag |= (CS8);
		newt.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
		newt.c_cc[VMIN] = 0;
		newt.c_cc[VTIME] = 1;
		if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &newt) == -1) {
			die("tcsetattr");
		}
	}

	for (int i = 0; i < argc; i++) {
		/* Create tabs for each arg */
		if (argv[i][0] != '-') {
			init_editor(i == 0 ? NULL : argv[i]);
		}
	}

	if (cat_mode) {
		refresh_screen();
		return 0;
	}

	while (1) {
		refresh_screen();
		int c = readch();
		switch (c) {
			case '\r':
				insert_new_line();
				break;

			case HOME_KEY:
				cur_editor->x = 0;
				break;

			case END_KEY:
				if (cur_editor->y < cur_editor->rows) {
					cur_editor->x = cur_editor->row[cur_editor->y].size;
				}
				break;

			case BACKSPACE:
			case CTRL_KEY('h'):
			case DEL_KEY:
				if (cur_editor->mode == INSERT) {
					if (c == DEL_KEY)
						move_xy(ARROW_RIGHT);
					del_char();
				} else if (cur_editor->mode == NORMAL) {
					move_xy(ARROW_LEFT);
				}
				break;

			case PAGE_UP:
			case PAGE_DOWN:
				if (c == PAGE_UP) {
					cur_editor->y = cur_editor->rowoff;
				} else if (c == PAGE_DOWN) {
					cur_editor->y = cur_editor->rowoff + rows - 1;
					if (cur_editor->y > cur_editor->rows) cur_editor->y = cur_editor->rows;
				}
				int times = rows;
				while (times--)
					move_xy(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
				break;

			case ARROW_UP:
			case ARROW_DOWN:
			case ARROW_LEFT:
			case ARROW_RIGHT:
				if (cur_editor->mode == INSERT) {
					move_xy(c);
				}
				break;

			case '\033':
				if (cur_editor->mode == INSERT) {
					move_xy(ARROW_LEFT);
				}
				cur_editor->mode = NORMAL;
				break;

			case 'i': /* PASSTHROUGH */
				if (cur_editor->mode == NORMAL) {
					cur_editor->mode = INSERT;
					break;
				}

			case 'v': /* PASSTHROUGH */
				if (cur_editor->mode == NORMAL) {
					cur_editor->mode = VISUAL;
					break;
				}

			case 'g': /* PASSTHROUGH */
				if (cur_editor->mode != INSERT) {
					c = readch();
					if (c == 'g') {
						cur_editor->y = 0;
					}
					break;
				}

			case 'G': /* PASSTHROUGH */
				if (cur_editor->mode != INSERT) {
					cur_editor->y = cur_editor->rows - 1;
					break;
				}

			case 'd': /* PASSTHROUGH */
				if (cur_editor->mode != INSERT) {
					c = readch();
					if (c == 'd') {
						del_row(cur_editor->y);
					}
					break;
				}

			case ':': /* PASSTHROUGH */
				if (cur_editor->mode == NORMAL) {
					cur_editor->mode = COMMAND;
					draw_status_bar();
					char *cmd = prompt_editor(":%s", NULL);
					if (cmd == NULL) {
						cur_editor->mode = NORMAL;
						break;
					}

					if (cmd[0] == 'q') {
						if (cmd[1] == '!') {
							cleanup();
							exit(0);
							break;
						} else {
							if (cur_editor->dirty) {
								wpprintw("%sNo write since last change for buffer \"%s\"", ERROR, cur_editor->filename);
								cur_editor->mode = NORMAL;
								break;
							}
							cleanup();
							exit(0);
							break;

						}
					} else if (cmd[0] == 'w') {
						save_file();
						cur_editor->mode = NORMAL;
						break;
					}

				}
			case '/': /* PASSTHROUGH */
				if (cur_editor->mode == NORMAL) {
					cur_editor->mode = COMMAND;
					find_editor();
					break;
				}

			case 'k': /* PASSTHROUGH */
				if (cur_editor->mode != INSERT) {
					move_xy(ARROW_UP);
					break;
				}

			case 'j': /* PASSTHROUGH */
				if (cur_editor->mode != INSERT) {
					move_xy(ARROW_DOWN);
					break;
				}

			case 'l': /* PASSTHROUGH */
				if (cur_editor->mode != INSERT) {
					move_xy(ARROW_RIGHT);
					break;
				}

			case 'h': /* PASSTHROUGH */
				if (cur_editor->mode != INSERT) {
					move_xy(ARROW_LEFT);
					break;
				}

			case 'O': /* PASSTHROUGH */
				if (cur_editor->mode == NORMAL) {
					shift_previous_line();
					cur_editor->mode = INSERT;
					break;
				}

			case 'o': /* PASSTHROUGH */
				if (cur_editor->mode == NORMAL) {
					shift_new_line();
					cur_editor->mode = INSERT;
					break;
				}

			case '0': /* PASSTHROUGH */
				if (cur_editor->mode == NORMAL) {
					cur_editor->x = 0;
					break;
				}

			case '$': /* PASSTHROUGH */
				if (cur_editor->mode == NORMAL) {
					cur_editor->x = cur_editor->row[cur_editor->y].size;
					break;
				}

			case ' ': /* PASSTHROUGH */
				if (cur_editor->mode == NORMAL) {
					c = readch();
					if (c == 'p') {
						c = readch();
						if (c == 'v') {
							pid_t pid = fork();
							if (pid == 0) {
								/* Child process */
								execlp("ccc", "ccc", cur_editor->cwd, "-p", NULL);
								_exit(1); /* Exit if exec fails */
							} else if (pid > 0) {
								/* Parent process */
								waitpid(pid, NULL, 0);
								char fpath[PATH_MAX];
								strcpy(fpath, "~/.cache/ccc/opened_file");
								replace_home(fpath);
								FILE *f = fopen(fpath, "r");
								char opened_file[PATH_MAX];
								fread(opened_file, sizeof(char), PATH_MAX, f);
								opened_file[strcspn(opened_file, "\n")] = 0;

								init_editor(opened_file);
							} else {
								/* Fork failed */
							}
						}
					} else if (c == 't') {
						c = readch();
						if (c > '0' && c <= '9') {
							cur_editor = &editor[c - '0'];
						}
					}
					break;
				}

			default:
				if (cur_editor->mode == INSERT) {
					insert_char(c);
				}
				break;
		}
	}

	cleanup();
	return 0;
}

void cleanup(void)
{
	/* Restore old terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldt) == -1) {
		die("tcsetattr");
	}
	bprintf("\033[2J\033[?1049l");
}

/*
 * Print line to the panel
 */
void wpprintw(const char *fmt, ...)
{
	char buffer[cols];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	move_cursor(rows, 1);
	/* Clear line and print formatted string */
	bprintf("\033[K%s\033[0m", buffer);
}

void move_cursor(int row, int col)
{
	bprintf("\033[%d;%dH", row, col);
}

int get_window_size(int *row, int *col)
{
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		/* Can't get window size */
		bprintf("\033[999C\033[999B");
		char buf[32];
		unsigned int i = 0;
		bprintf("\033[6n");
		while (i < sizeof(buf) - 1) {
			if (read(STDIN_FILENO, &buf[i], 1) != 1) {
				break;
			}
			if (buf[i] == 'R') {
				break;
			}
			i++;
		}
		buf[i] = '\0';
		if (buf[0] != '\033' || buf[1] != '[') {
			return -1;
		}
		if (sscanf(&buf[2], "%d;%d", row, col) != 2) {
			return -1;
		}
		return 0;
	} else {
		*col = ws.ws_col;
		*row = ws.ws_row;
		return 0;
	}
}

/*
 * printf, but write to STDOUT_FILENO
 */
void bprintf(const char *fmt, ...)
{
	char buffer[512];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (cat_mode) {
		printf("%s", buffer);
	} else {
		write(STDOUT_FILENO, buffer, strlen(buffer));
	}
}
