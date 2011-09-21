/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software written by Ken Arnold and
 * published in UNIX Review, Vol. 6, No. 8.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ddftp.h>

/*
 * Special version of popen which avoids call to shell.  This insures noone
 * may create a pipe to a hidden program as a side effect of a list or dir
 * command.
 */
static int *pids;
static int fds;

FILE *ftpd_popen(char *program, char *type)
{
	char *cp;
	FILE *iop;
	int argc, pdes[2], pid;
	char *argv[100];

	if ((*type != 'r' && *type != 'w') || type[1])
		return NULL;

	if (!pids) {
		if ((fds = getdtablesize()) <= 0)
			return NULL;
		if ((pids = (int *) malloc(fds * sizeof(int))) == NULL)
			return NULL;
		memset(pids, 0, fds * sizeof(int));
	}
	if (pipe(pdes) == -1)
		return NULL;

	/* break up string into pieces */
	for (argc = 0, cp = program;; cp = NULL)
		if (!(argv[argc++] = strtok(cp, " \t\n")))
			break;

	argv[argc] = NULL;

	iop = NULL;
	switch (pid = fork()) {
	case -1:
		close(pdes[0]);
		close(pdes[1]);
		_exit(1);
		/* NOTREACHED */
	case 0:
		if (*type == 'r') {
			if (pdes[1] != 1) {
				dup2(pdes[1], 1);
				dup2(pdes[1], 2);
				close(pdes[1]);
			}
			close(pdes[0]);
		} else {
			if (pdes[0] != 0) {
				dup2(pdes[0], 0);
				close(pdes[0]);
			}
			close(pdes[1]);
		}
		execv(argv[0], argv);
		_exit(1);
	}
	/* parent; assume fdopen can't fail...  */
	if (*type == 'r') {
		iop = fdopen(pdes[0], type);
		close(pdes[1]);
	} else {
		iop = fdopen(pdes[1], type);
		close(pdes[0]);
	}
	pids[fileno(iop)] = pid;
	return iop;
}

int ftpd_pclose(FILE * iop)
{
	int fdes;
	int stat_loc;
	int pid;
	sigset_t sigset, saved_sigset;

	/*
	 * pclose returns -1 if stream is not associated with a
	 * `popened' command, or, if already `pclosed'.
	 */
	if (pids == 0 || pids[fdes = fileno(iop)] == 0)
		return -1;
	fclose(iop);
	/* FIXME: why are these signals blocked anyway? */
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGQUIT);
	sigaddset(&sigset, SIGHUP);
	sigprocmask(SIG_BLOCK, &sigset, &saved_sigset);
	while ((pid = wait((int *) &stat_loc)) != pids[fdes] && pid != -1);
	sigprocmask(SIG_SETMASK, &saved_sigset, NULL);
	pids[fdes] = 0;
	return (pid == -1) ? -1 : WEXITSTATUS(stat_loc);
}
