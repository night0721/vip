#ifndef TERM_H_
#define TERM_H_

void die(const char *s);
void reset_term();
void setup_term();
int get_cursor_position(int *rows, int *cols);
int get_window_size(int *rows, int *cols);

#endif
