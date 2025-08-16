#include <unistd.h>
#include <stdint.h>
#ifndef RAWTUI_H_
#define RAWTUI_H_

#define colorpair_t uint8_t
#define attr_t uint8_t
#define NORMAL 1
#define BOLD 1<<1
#define FAINT 1<<2
#define BLINK 1<<3
#define REVERSE 1<<4

#define BLACK 0
#define	RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define MAGENTA 5
#define CYAN 6
#define WHITE 7

#define COLORPAIR(color) color<<5

void init();
void initinline();
void deinit();
void setcursor(uint8_t visible);
void move(uint16_t y, uint16_t x);
void initcolorpair(uint8_t id, uint8_t foreground, uint8_t background);
uint8_t in();
uint8_t inesc();
void clear();
void cleartobot();
void saveCursorPos();
void loadCursorPos();
void wrattr(attr_t attr);
void getTermXY(uint16_t *y, uint16_t *x);
void getCursorPos(uint16_t *y, uint16_t *x);
void clearline();
void print(char *string);
void printsize(char *string, int len);
void moveprint(uint16_t y, uint16_t x, char *string);
void moveprintsize(uint16_t y, uint16_t x, char *string, int len);

#endif
