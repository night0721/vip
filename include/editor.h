#ifndef EDITOR_H_
#define EDITOR_H_

void insert_char(int c);
void insert_new_line();
void shift_new_line();
void del_char();
void open_editor(char *filename);
char *prompt_editor(char *prompt, void (*callback)(char *, int));
void find_editor();

#endif
