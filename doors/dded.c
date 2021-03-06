#include <ddlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

static struct dif *d;

static int slen;
static int spos;
static int xpos;
static int ypos;
static int lines;
static int highline;
static int linelen;

static char data[80 * 500];
static char *datapos;
static char *lineadd;
static char sodobuf[80];
static int node;

static void insertliner(int);
static void redraw(int);
static void insertchar(unsigned char);
static void scrolldown(void);
static void scrollup(void);
static void started(char *);
static void attach();
static char *fgetsnolf(char *, int, FILE *) __attr_bounded__ (__string__, 1, 2);

int main(int argc, char *argv[])
{
	if (argc == 1) {
		printf("This program requires MS Windows!\n");
		exit(1);
	}
	d = dd_initdoor(argv[1]);
	if (d == 0) {
		printf("Couldn't find socket!\n");
		exit(1);
	}
	dd_changestatus(d, "Running FSEditor..");

	node = atoi(argv[1]);
	started(argv[1]);
	while (1) {
		int key;

		for (key = 0; key < 79; key++) {
			if (lineadd[key] == 0) {
				for (; key < 79; key++)
					lineadd[key] = 0;
				break;
			}
		}
		key = dd_hotkey(d, HOT_CURSOR);

		if (key == 0 || key == -1) {
			break;
		} else if (key == 1) {
			dd_sendstring(d, "[0;44;33m[1;11H- Abort message (Yes/no)?: ");
			if (dd_hotkey(d, HOT_YESNO) != 2) {
				break;
			}
			dd_sendstring(d, "[0;44;33m[1;11H                              ");
			{
				char buf[50];
				sprintf(buf, "[%d;%dH[0m", spos, xpos + 1);
				dd_sendstring(d, buf);
			}
		} else if (key == 2) {
			lineadd = datapos = &data[lines * 80];
			ypos = lines;
			highline = lines + 4 - slen;
			if (highline < 0)
				highline = 0;

			spos = slen - 4 - ypos;
			if (spos > 0) {
				spos = ypos + 4;
			} else {
				spos = slen;
			}
			linelen = strlen(lineadd);
			xpos = 0;

			dd_sendstring(d, "[4;0H[J");
			redraw(0);
		} else if (key == 20) {
			datapos = lineadd = data;
			ypos = highline = 0;
			spos = ypos + 4;

			linelen = strlen(lineadd);
			xpos = 0;

			dd_sendstring(d, "[4;0H[J");
			redraw(0);

		} else if (key == 13 || key == 10) {
			char *s, *t;
			if (lines > 497)
				continue;
			s = datapos;
			t = sodobuf;
			while (*s) {
				*t++ = *s;
				*s++ = 0;
			}
			*t = 0;
			dd_sendstring(d, "[K");
			insertliner(0);
		} else if (key == 25) {
			char *s;

			if (xpos == linelen)
				continue;
			s = datapos;
			while (*s)
				*s++ = 0;
			dd_sendstring(d, "[K");
			linelen = xpos;
		} else if (key == 6) {
			attach();
		} else if (key == 24 || key == 26) {
			char buf1[80];
			char buf2[100];
			char *s;
			FILE *myf;
			int i;
			if (!lines && !*data)
				break;
			sprintf(buf1, "%s/fsed%d.txt", DDTMP, node);
			if ((myf = fopen(buf1, "w"))) {
				s = data;
				for (i = lines; i; i--) {
					sprintf(buf2, "%s\n", s);
					s = &s[80];
					fputs(buf2, myf);
				}
				if (*s) {
					sprintf(buf2, "%s\n", s);
					fputs(buf2, myf);
				}
				fclose(myf);
			}
			break;
		} else if (key == 8 || key == 127) {
			char *s, *t, *u;
			int i, j;
			char buf[300];

			if (!xpos) {
				if (!ypos)
					continue;
				s = &lineadd[-80];
				i = strlen(s);
				j = strlen(lineadd);
				if ((i + j) > 77)
					continue;
				sprintf(buf, "[M[%d;0H%s[%d;%dH", slen, &data[80 * (highline - 3 + slen)], spos, xpos);
				dd_sendstring(d, buf);
				if (spos == 4) {
					sprintf(buf, "[%d;0H[M", slen);
					dd_sendstring(d, buf);
					sprintf(buf, "[4;0H[L%s\n", &lineadd[80]);
					dd_sendstring(d, buf);
					spos++;
					highline--;
				}
				spos--;
				t = &s[i];
				sprintf(buf, "[%d;%dH%s[%d;%dH", spos, i + 1, lineadd, spos, i + 1);
				xpos = i;
				dd_sendstring(d, buf);
				u = lineadd;
				while (*u) {
					*t++ = *u++;
				}
				*u++ = 0;


				u = lineadd;
				i = ypos;
				while (i != lines) {
					s = &u[80];
					for (j = 0; j != 80; j++) {
						*u++ = *s++;
					}
					i++;
				}
				lineadd = &lineadd[-80];
				linelen = strlen(lineadd);
				datapos = &lineadd[xpos];
				memset(&data[lines * 80], 0, 80);
				ypos--;
				lines--;
				continue;
			}
			if (xpos != linelen) {
				datapos--;
				s = datapos;
				while (*(s + 1)) {
					*s = *(s + 1);
					s++;
				}
				*s = 0;
				dd_sendstring(d, "[D[P");
				xpos--;
				linelen--;
				continue;
			}
			dd_sendstring(d, "[D [D");
			datapos--;
			*datapos = 0;
			xpos--;
			linelen--;
		} else if (key == 18) {
			redraw(1);
		} else if (key == 9) {
			int i;
			for (i = 0; i < 8; i++)
				insertchar(' ');
		} else if (key == 11) {
			char buf[1024];
			char *s, *u;
			int i, j;

			if (ypos == lines)
				continue;
			sprintf(buf, "[M[%d;0H%s[%d;%dH", slen, &data[80 * (highline - 3 + slen)], spos, xpos);
			dd_sendstring(d, buf);

			memset(lineadd, 0, 79);
			u = lineadd;
			i = ypos;
			while (i != lines) {
				s = &u[80];
				for (j = 0; j != 80; j++) {
					*u++ = *s++;
				}
				i++;
			}
			linelen = strlen(lineadd);
			if (xpos > linelen) {
				xpos = linelen;
				if (xpos == 0)
					dd_sendstring(d, "\r");
				else {
					sprintf(buf, "\r[%dC", xpos);
					dd_sendstring(d, buf);
				}
			}
			datapos = &lineadd[xpos];
			lines--;

		} else if (key == 21) {
			dd_sendstring(d, "[4;0H[J");
			dd_sendstring(d, "            DDEd V1.0 Programmed by Antti H�yrynen (Hydra/Selleri).\n\n       Enter = new line                   Backspace = delete prev char\n       Del = delete current char          TAB = skip 8 columns\n\nCTRL/Z save and quit       CTRL/Y delete rest of line   CTRL/R redraw screen\nCTRL/A abort message       CTRL/K Kill line             CTRL/T jump to top\nCTRL/B jump to bottom      CTRL/F File attach\n\n                          Press any key to continue\n");
			if (dd_hotkey(d, 0) == -1)
				break;
			dd_sendstring(d, "[4;0H[J");
			redraw(0);
		} else if (key == 250) {
			scrollup();
		} else if (key == 251) {
			scrolldown();
		} else if (key == 252) {
			if (xpos == linelen) {
				if (lines == ypos)
					continue;
				scrolldown();
				datapos = lineadd;
				xpos = 0;
				{
					char buf[50];
					sprintf(buf, "[%d;%dH", spos, xpos + 1);
					dd_sendstring(d, buf);
				}
				continue;
			}
			datapos++;
			xpos++;
			dd_sendstring(d, "[C");
		} else if (key == 253) {
			if (!xpos && !ypos)
				continue;
			if (!xpos) {
				scrollup();
				datapos = &lineadd[strlen(lineadd)];
				xpos = strlen(lineadd);
				if (xpos) {
					char buf[50];
					sprintf(buf, "[%d;%dH", spos, xpos + 1);
					dd_sendstring(d, buf);
				}
				continue;
			}
			dd_sendstring(d, "[D");
			datapos--;
			xpos--;
		} else if (key > 31) {
			insertchar(key);
		}
	}
	dd_sendstring(d, "[0m[2J[H");
	dd_close(d);
	return 0;
}

static void attach()
{
	char fabuf[1024];
	struct DayDream_Message msg;
	int head;

	sprintf(fabuf, "%s/msgheader.%d", DDTMP, node);
	head = open(fabuf, O_RDONLY);
	if (head > -1) {
		read(head, &msg, sizeof(struct DayDream_Message));
		close(head);
		if (*msg.MSG_ATTACH == 1) {
			dd_sendstring(d, "[2J[H");
			dd_fileattach(d);
			redraw(1);
		}
	}
}

static char *fgetsnolf(char *buf, int n, FILE * fh)
{
	char *hih;
	char *s;

	hih = fgets(buf, n, fh);
	if (!hih)
		return 0;
	s = buf;
	while (*s) {
		if (*s == 13 || *s == 10) {
			*s = 0;
			break;
		}
		s++;
	}
	return hih;
}

static void started(char *node)
{
	char quotena[80];
	FILE *myf;
	char qbuf[79];

	slen = dd_getintval(d, USER_SCREENLENGTH);
	spos = 4;
	lines = 0;
	datapos = data;
	lineadd = data;
	linelen = 0;

	memset(data, 0, 80 * 500);
	sprintf(quotena, "%s/daydream%s.msg", DDTMP, node);
	if ((myf = fopen(quotena, "r"))) {
		while (fgetsnolf(qbuf, 79, myf)) {
			if (lines > 470)
				break;
			strcpy(lineadd, qbuf);
			lineadd = &lineadd[80];
			datapos = &datapos[80];
			lines++;
		}
		fclose(myf);
		ypos = lines;
		highline = lines + 4 - slen;
		if (highline < 0)
			highline = 0;
		spos = slen - 4 - ypos;
		if (spos > 0) {
			spos = ypos + 4;
		} else {
			spos = slen;
		}
		redraw(1);
	} else {
		dd_sendstring(d, "[2J[H[0;44;35mDDEd V1.2                                                                      \n[33mCtrl-Z=Save, Ctrl-A=Abort, Ctrl-U=Help, Cursor keys to move.                   \n[36m<---+----1----+----2----+----3----+----4----+----5----+----6----+----7->--+--->[0m\n");
	}
}

static void scrollup(void)
{
	char buf[500];
	if (!ypos)
		return;
	if (spos == 4) {
		sprintf(buf, "[%d;0H[M", slen);
		dd_sendstring(d, buf);
		sprintf(buf, "[4;0H[L%s\n", &lineadd[-80]);
		dd_sendstring(d, buf);
		spos++;
		highline--;
	}
	spos--;
	lineadd = &lineadd[-80];
	linelen = strlen(lineadd);
	ypos--;
	if (xpos > linelen) {
		xpos = linelen;
		if (xpos == 0)
			dd_sendstring(d, "\r");
		else {
			sprintf(buf, "\r[%dC", xpos);
			dd_sendstring(d, buf);
		}
	}
	dd_sendstring(d, "[A");
	datapos = &lineadd[xpos];
}

static void scrolldown(void)
{
	char buf[500];
	if (lines == ypos)
		return;
	if (slen == spos) {
		dd_sendstring(d, "[4;0H[M");
		sprintf(buf, "[%d;0H%s\n%s[A\r", spos - 1, lineadd, &lineadd[80]);
		dd_sendstring(d, buf);
		spos--;
		highline++;
	}
	spos++;
	lineadd = &lineadd[80];
	linelen = strlen(lineadd);
	ypos++;
	if (xpos > linelen) {
		xpos = linelen;
		if (xpos == 0)
			dd_sendstring(d, "\r");
		else {
			sprintf(buf, "\r[%dC", xpos);
			dd_sendstring(d, buf);
		}
	}
	dd_sendstring(d, "[B");
	datapos = &lineadd[xpos];
}

static void insertchar(unsigned char c)
{
	char buf[90];
	char buf2[100];
	char *s, *t, *u;
	char *a0, *a1;

	int i = 0, j = 0;

	if (lines > 497) {
		dd_sendstring(d, "[0;44;33m[1;11H- Out of space! ");
		dd_hotkey(d, 0);
		dd_sendstring(d, "[0;44;33m[1;11H                ");
		return;
	}
	if (linelen == 78) {
		if (xpos != 78) {
			s = &lineadd[linelen];
			while (1) {
				if (s == lineadd)
					return;
				if (s == datapos)
					return;
				if (*s == ' ')
					break;
				j++;
				s--;
			}
			t = &lineadd[80];
			while (*t) {
				i++;
				t++;
			}
			if ((j + i) > 77)
				return;
			s++;
			u = buf;
			while (*s) {
				*u++ = *s;
				*s++ = 0;
			}
			*u = 0;
			sprintf(buf2, "[%d;%dH[K", spos, linelen - j + 1);
			dd_sendstring(d, buf2);
			if (spos != slen) {
				sprintf(buf2, "[%d;0H[%d@%s", spos + 1, j, buf);
				dd_sendstring(d, buf2);
			}
			sprintf(buf2, "[%d;%dH", spos, xpos + 1);
			dd_sendstring(d, buf2);
			a0 = &lineadd[80 + i];
			a1 = &a0[j];
			while (i + 1) {
				*--a1 = *--a0;
				i--;
			}
			a0 = buf;
			a1 = &lineadd[80];
			while (*a0) {
				*a1++ = *a0++;
			}
			if (!*a1)
				*a1++ = ' ';
			linelen = strlen(lineadd);
			linelen--;
			lineadd[linelen] = 0;
			if (lines == ypos)
				lines++;
		} else {
			if (*datapos == ' ') {
				*sodobuf = 0;
			} else {
				s = datapos;
				while (1) {
					if (s == lineadd)
						return;
					if (*s == ' ')
						break;
					j++;
					s--;
				}
				*s++ = 0;
				t = sodobuf;
				while (*s) {
					*t++ = *s;
					*s++ = 0;
				}
				*t = c;
				t[1] = 0;
				sprintf(buf2, "[%dD[K", j);
				dd_sendstring(d, buf2);
			}
			insertliner(1);
			return;

		}
	}
	if (xpos != linelen) {
		s = &lineadd[linelen];
		t = &lineadd[xpos];
		while (1) {
			s[1] = *s;
			if (s == t)
				break;
			s--;
		}
		*s = c;
		dd_sendstring(d, "[@");
		buf[0] = c;
		buf[1] = 0;
		dd_sendstring(d, buf);
		linelen++;
		datapos++;
		xpos++;
		return;
	}
	linelen++;
	xpos++;
	*datapos++ = (char) c;
	buf[0] = c;
	buf[1] = 0;
	dd_sendstring(d, buf);
}

static void redraw(int mode)
{
	char buf[500];
	int i;
	char *s;
	int lin;

	if (mode) {
		dd_sendstring(d, "[2J[H[0;44;35mDDEd V1.2                                                                      \n[33mCtrl-Z=Save, Ctrl-A=Abort, Ctrl-U=Help, Cursor keys to move.                   \n[36m<---+----1----+----2----+----3----+----4----+----5----+----6----+----7->--+--->[0m\n");
	}
	s = &data[highline * 80];
	lin = highline;
	for (i = slen - 3; i; i--, lin++) {
		if (lin > lines)
			break;
		dd_sendstring(d, s);
		dd_sendstring(d, "\n");
		s = &s[80];
	}
	sprintf(buf, "[%d;%dH", spos, xpos + 1);
	dd_sendstring(d, buf);
}

static void insertliner(int mode)
{
	char buf[500];
	int k;
	char *s, *t, *a1;
	int ii;

	sprintf(buf, "[%d;0H[M", slen);
	dd_sendstring(d, buf);

	if (slen == spos) {
		dd_sendstring(d, "[4;0H[M");
		sprintf(buf, "[%d;0H%s\n%s[A\r", spos - 1, lineadd, &lineadd[80]);
		dd_sendstring(d, buf);
		spos--;
		highline++;
	}
	spos++;

	if (mode) {
		sprintf(buf, "[%d;0H[L%s", spos, sodobuf);
	} else {
		sprintf(buf, "[%d;0H[L%s[%d;0H", spos, sodobuf, spos);
	}
	dd_sendstring(d, buf);
	lineadd = &lineadd[80];
	xpos = 0;
	linelen = 0;
	datapos = lineadd;
	if (lines != ypos) {

		s = &data[((lines + 1) * 80)];
		t = &lineadd[-80];
		while (1) {
			if (s == t)
				break;
			a1 = &s[80];
			for (ii = 80; ii; ii--) {
				*a1++ = *s++;
			}
			s = &s[-160];
		}
	}
	ypos++;
	strcpy(datapos, sodobuf);
	if (mode) {
		k = strlen(datapos);
		xpos = k;
		datapos = &datapos[k];
		linelen = k;
	} else {
		linelen = strlen(lineadd);
	}
	lines++;
	if (!*datapos) {
		for (ii = 0; ii < 79; ii++)
			datapos[ii] = 0;
	}
}
