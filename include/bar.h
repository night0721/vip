#ifndef BAR_H_
#define BAR_H_

void draw_status_bar(struct abuf *ab);
void draw_message_bar(struct abuf *ab);
void set_status_bar_message(const char *fmt, ...);

#endif
