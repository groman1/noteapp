#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>

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

struct termios originalterminal;
colorpair_t pairs[8];

void initcolorpair(uint8_t id, uint8_t foreground, uint8_t background)
{
	if (id<8) pairs[id] = foreground<<4|background;
}

void init()
{
	tcgetattr(STDIN_FILENO, &originalterminal);
	struct termios terminal;
	cfmakeraw(&terminal);
	tcsetattr(STDIN_FILENO, 0, &terminal);
	initcolorpair(0, WHITE, BLACK);
	write(STDOUT_FILENO, "\x1b[?1049h", 8); // alternative buffer
}

void initinline()
{
	tcgetattr(STDIN_FILENO, &originalterminal);
	struct termios terminal;
	cfmakeraw(&terminal);
	tcsetattr(STDIN_FILENO, 0, &terminal);
	initcolorpair(0, WHITE, BLACK);
}

void deinit()
{
	tcsetattr(STDIN_FILENO, 0, &originalterminal);
	write(STDOUT_FILENO, "\x1b[?1049l", 8);
}


void setcursor(uint8_t status)
{
	if (status) write(STDOUT_FILENO, "\x1b[?25h", 6);
	else write(STDOUT_FILENO, "\x1b[?25l", 6);
}

void move(uint16_t y, uint16_t x)
{
	++y;
	++x;
	char movecmd[10] = "\x1b[";
	int currPos = 2;
	for (int i = 100; i>0; i/=10)
	{
		if (i!=0&&y/i==0) continue;
		movecmd[currPos++] = y/i%10+48;
	}
	movecmd[currPos++] = ';';
	for (int i = 100; i>0; i/=10)
	{
		if (i!=1&&x/i==0) continue;
		movecmd[currPos++] = x/i%10+48;
	}
	movecmd[currPos++] = 'H';

	write(STDOUT_FILENO, movecmd, currPos);
}

uint8_t in(void)
{
	unsigned char ret;
	read(STDIN_FILENO, &ret, 1);
	return ret;
}

uint8_t inesc()
{
	unsigned char ret;
	read(STDIN_FILENO, &ret, 1);
	if (ret==27)
	{
		unsigned char buffer[2];
		read(STDIN_FILENO, &buffer, 2);
		switch (buffer[0])
		{
			case 'O':
			{
				switch (buffer[1])
				{
					case 'P': ret = 170; break; // F1
					case 'Q': ret = 171; break; // F2
					case 'R': ret = 172; break; // F3
					case 'S': ret = 173; break; // F4
				}
				break;
			}
			case '[':
			{
				switch (buffer[1])
				{
					case '1': case '2':
					{
						read(STDIN_FILENO, &buffer, 2);
						switch (buffer[0])
						{
							case '5': ret = 174; break; // F5
							case '7': ret = 175; break;	// F6
							case '8': ret = 176; break;	// F7
							case '9': ret = 177; break;	// F8
							case '0': ret = 178; break; // F9
							case '1': ret = 179; break; // F10
							case '3': ret = 180; break; // F11
							case '4': ret = 181; break; // F12
							case '~': ret = 182; break; // Insert
						}
						break;
					}
					case '3': ret = 183; read(STDIN_FILENO, buffer, 1); break; // Delete
					case 'H': ret = 184; break;	// Home
					case 'F': ret = 185; break; // End
					case '5': ret = 186; read(STDIN_FILENO, buffer, 1); break; // PageUp
					case '6': ret = 187; read(STDIN_FILENO, buffer, 1); break; // PageDn
					case 'A': ret = 188; break; // ArrowUp
					case 'B': ret = 189; break; // ArrowDown
					case 'C': ret = 190; break; // ArrowRight
					case 'D': ret = 191; break; // ArrowLeft
				}
				break;
			}
		}
	}
	return ret;
}

void saveCursorPos()
{
	write(STDOUT_FILENO, "\x1b[s", 3);
}

void loadCursorPos()
{
	write(STDOUT_FILENO, "\x1b[u", 3);
}

void wrattr(attr_t attr)
{
	if (attr&NORMAL) write(STDOUT_FILENO, "\x1b[0m", 4);
	if (attr&BOLD) write(STDOUT_FILENO, "\x1b[1m", 4);
	if (attr&FAINT) write(STDOUT_FILENO, "\x1b[2m", 4);
	if (attr&REVERSE) write(STDOUT_FILENO, "\x1b[7m", 4);
	char colorstring[8] = "\x1b[30;47m";
	colorstring[3] = (pairs[attr>>5]>>4)+48;
	colorstring[6] = (pairs[attr>>5]%16)+48;
	write(STDOUT_FILENO, colorstring, 8);
}

void clear()
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
}

void cleartobot()
{
	write(STDOUT_FILENO, "\x1b[J", 3);
}

void getTermXY(uint16_t *y, uint16_t *x)
{
	struct winsize win;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win)) if (ioctl(STDIN_FILENO, TIOCGWINSZ, &win)) if (ioctl(STDERR_FILENO, TIOCGWINSZ, &win)) return;
	*y = win.ws_row;
	*x = win.ws_col;
}

void getCursorPos(uint16_t *y, uint16_t *x)
{
	write(STDOUT_FILENO, "\x1b[6n", 4);
	*y = 0;
	*x = 0;
	char buffer[2];
	int currPos = 2;
	read(STDIN_FILENO, buffer, 2);
	for (int i = 0; i<3; ++i)
	{
		read(STDIN_FILENO, buffer, 1);
		if (*buffer==';') break;
		*y *= 10;
		*y += *buffer-48;
	}
	for (int i = 0; i<3; ++i)
	{
		read(STDIN_FILENO, buffer, 1);
		if (*buffer=='R') break;
		*x *= 10;
		*x += *buffer-48;
	}
	--*y;
	--*x;
}

void clearline()
{
	write(STDOUT_FILENO, "\x1b[2K", 4);
}

void printsize(char *string, int len)
{
	write(STDOUT_FILENO, string, len);
}

void moveprintsize(uint16_t y, uint16_t x, char *string, int len)
{
	move(y,x);
	printsize(string, len);
}

void print(char *string)
{
	printsize(string, strlen(string));
}

void moveprint(uint16_t y, uint16_t x, char *string)
{
	move(y,x);
	print(string);
}
