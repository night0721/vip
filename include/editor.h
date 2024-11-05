#ifndef EDITOR_H_
#define EDITOR_H_

void refresh_screen(void);
void move_cursor(int key);
void scroll(void);
void insert_char(int c);
void insert_new_line(void);
void shift_new_line(void);
void del_char(void);
void init_editor(void);
void open_editor(char *filename);
char *prompt_editor(char *prompt, void (*callback)(char *, int));
void find_editor(void);

#endif
