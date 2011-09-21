#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <daydream.h>
#include <ddcommon.h>

int replymessage(struct DayDream_Message *msgd)
{
	char qbuffer[4096];
	char input[2048];
	char msgin[10];
	FILE *msgfd;
	FILE *quotefd;
	char *s, *t;
	int i;
	int hola;
	int l = 0;

	struct DayDream_Message header;

	s = (char *) &header;
	for (hola = 0; hola < sizeof(struct DayDream_Message); hola++) {
		*s++ = 0;
	}

	snprintf(qbuffer, sizeof qbuffer, "%s/messages/base%3.3d/msg%5.5d", 
		conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, 
		msgd->MSG_NUMBER);

	if (!(msgfd = fopen(qbuffer, "r")))
		return 0;

	snprintf(qbuffer, sizeof qbuffer, "%s/daydream%d.mtm", DDTMP, node);
	unlink(qbuffer);

	if (!(quotefd = fopen(qbuffer, "w"))) {
		fclose(msgfd);
		return 0;
	}
	if (current_msgbase->MSGBASE_FLAGS & (1L << 3)) {
		if (current_msgbase->MSGBASE_FLAGS & (1L << 4)) {
			s = msgd->MSG_AUTHOR;
			t = msgin;
			*t++ = *s++;
			while (*s) {
				if (*s == ' ' && *(s + 1)) {
					s++;
					*t++ = *s;
				}
				s++;
			}
			*t++ = '>';
			*t = 0;
		} else {
			msgin[0] = '>';
			msgin[1] = 0;
		}
	} else {
		msgin[0] = 0;
	}


	while (fgets(input, 2048, msgfd)) {
		if (l == 0 && !strncmp("AREA:", input, 5))
			continue;
		l++;
		if (*input == 1)
			continue;
		if (!strncmp("SEEN-BY:", input, 8))
			break;

		stripansi(input);
		snprintf(qbuffer, sizeof qbuffer, "%s%s", msgin, input);
		t = qbuffer;
		for (;;) {
			if (strlen(t) > 76) {
				char k;

				s = &t[75];
				for (;;) {
					if (s == t) {
						k = t[75];
						t[75] = 0;
						s = &t[75];
						break;
					} else {
						if (*s == ' ') {
							k = *s;
							*s = 0;
							break;
						}
						s--;
					}

				}
				fputs(t, quotefd);
				fputs("\n", quotefd);
				*s = k;
				if (k == ' ')
					s++;
				t = s;
				snprintf(qbuffer, sizeof qbuffer, "%s%s", 
					msgin, t);
				t = qbuffer;
			} else {
				fputs(t, quotefd);
				break;
			}
		}
	}

	if (!msgin[0]) {
		snprintf(qbuffer, sizeof qbuffer, "---[ %s ]", 
			msgd->MSG_AUTHOR);

		for (i = strlen(qbuffer); i < 75; i++) {
			strlcat(qbuffer, "-", sizeof qbuffer);
		}
		strlcat(qbuffer, "\n\n", sizeof qbuffer);
		fputs(qbuffer, quotefd);
	}
	fclose(msgfd);
	fclose(quotefd);

	strlcpy(header.MSG_RECEIVER, msgd->MSG_AUTHOR, sizeof header.MSG_RECEIVER);
	strlcpy(header.MSG_SUBJECT, msgd->MSG_SUBJECT, sizeof header.MSG_SUBJECT);
	header.MSG_ORIGINAL = msgd->MSG_NUMBER;
	if (msgd->MSG_FLAGS & (1L << 0))
		header.MSG_FLAGS |= (1L << 0);
	if (toupper(current_msgbase->MSGBASE_FN_FLAGS) == 'N') {
		if (msgd->MSG_FN_ORIG_ZONE) {
			header.MSG_FN_DEST_ZONE = msgd->MSG_FN_ORIG_ZONE;
		} else {
			header.MSG_FN_DEST_ZONE = msgd->MSG_FN_PACKET_ORIG_ZONE;
		}
		header.MSG_FN_DEST_NET = msgd->MSG_FN_ORIG_NET;
		header.MSG_FN_DEST_NODE = msgd->MSG_FN_ORIG_NODE;
		header.MSG_FN_DEST_POINT = msgd->MSG_FN_ORIG_POINT;

	}
	i = entermsg(&header, 1, 0);
	getmsgptrs();
	return i;
}

/* FIXME! better to move this to entermsg.c */
int askqlines(void)
{
	char qbuffer[200];
	char input[100];
	int ql;
	int lcount;
	int outp;
	int startn;
	int endn;
	int line;
	FILE *qfd, *msgfd;

	snprintf(qbuffer, sizeof qbuffer, "%s/daydream%d.mtm", DDTMP, node);
	if (!(qfd = fopen(qbuffer, "r")))
		return 0;

	DDPut("\n");

	for (;;) {
		ql = 0;
		lcount = user.user_screenlength;
		outp = 1;

		while (fgets(input, 77, qfd)) {
			ql++;
			if (outp) {
				ddprintf(sd[lel2str], ql, input);
				lcount--;
			}
			if (lcount == 0) {
				int hot;

				DDPut(sd[morepromptstr]);
				hot = HotKey(0);
				DDPut("\r                                                         \r");
				if (hot == 'N' || hot == 'n') {
					outp = 0;
					lcount = -1;
				} else if (hot == 'C' || hot == 'c') {
					lcount = -1;
				} else {
					lcount = user.user_screenlength;
				}
			}
		}
		ddprintf(sd[lequotestr], ql);
		qbuffer[0] = 0;
		if (!(Prompt(qbuffer, 3, PROMPT_NOCRLF))) {
			fclose(qfd);
			return 0;
		}
		if (!strcasecmp(qbuffer, "l")) {
			DDPut("\n\n");
			fseek(qfd, 0, SEEK_SET);
		} else if ((!strcasecmp(qbuffer, "*")) || *qbuffer == 0) {
			startn = 1;
			endn = ql;
			break;
		} else if ((startn = atoi(qbuffer))) {
			DDPut(sd[ledeltostr]);
			qbuffer[0] = 0;
			if (!(Prompt(qbuffer, 3, PROMPT_NOCRLF))) {
				fclose(qfd);
				return 0;
			}
			if ((endn = atoi(qbuffer))) {
				break;
			} else {
				fclose(qfd);
				return 0;
			}
		} else {
			fclose(qfd);
			return 0;
		}

	}

	fseek(qfd, 0, SEEK_SET);
	if (startn < 1 || endn > ql) {
		fclose(qfd);
		return 0;
	}
	line = 1;

	snprintf(qbuffer, sizeof qbuffer, "%s/daydream%d.msg", DDTMP, node);
	if (!(msgfd = fopen(qbuffer, "w"))) {
		fclose(qfd);
		return 0;
	}

	while (fgets(input, 77, qfd)) {
		if (startn <= line && endn >= line) {
			fputs(input, msgfd);
		}
		line++;
	}
	fclose(qfd);
	fclose(msgfd);
	return 1;
}

int getreplyid(int msg, char *de, size_t delen)
{
	FILE *msgf;
	char buf[1024];

	snprintf(buf, sizeof buf, "%s/messages/base%3.3d/msg%5.5d", 
		conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, 
		msg);
	msgf = fopen(buf, "r");
	if (!msgf)
		return 0;
	while (fgets(buf, 1024, msgf)) {
		if (!strncmp(buf, "\001MSGID:", 7)) {
			buf[1] = 'R';
			buf[2] = 'E';
			buf[3] = 'P';
			buf[4] = 'L';
			buf[5] = 'Y';
			fclose(msgf);
			strlcpy(de, buf, delen);
			return 1;
		}
	}
	fclose(msgf);
	return 0;
}
