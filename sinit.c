/* See LICENSE file for copyright and license details. */
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LEN(x) (sizeof (x) / sizeof *(x))

static void sigpoweroff(void);
static void sigreboot(void);
static void sigreap(void);
static void spawn(char *const []);

static struct {
	int sig;
	void (*handler)(void);
} sigmap[] = {
	/** 
	 * When reboot() is called from a process that isn't init, SIGINT signal is
	 * sent to init for LINUX_REBOOT_CMD_POWER_OFF  and LINUX_REBOOT_CMD_HALT.
	 */
	{ SIGUSR1, sigpoweroff },
	/** 
	 * When reboot() is called from a process that isn't init, SIGHUP signal is
	 * sent to init for LINUX_REBOOT_CMD_RESTART and LINUX_REBOOT_CMD_RESTART2.
	 */
	{ SIGHUP,  sigreboot   },
	{ SIGCHLD, sigreap     },
};

#include "config.h"

static sigset_t set;

int main(void)
{
	int sig;
	size_t i;

	/* Check if we're init */
	if(getpid() != 1)
		return 1;

	chdir("/");
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);

	/* Perform init tasks here */
	spawn(retroarch);

	/* Handle signals */
	while(1)
	{
		sigwait(&set, &sig);

		for (i = 0; i < LEN(sigmap); i++)
		{
			if (sigmap[i].sig == sig)
			{
				sigmap[i].handler();
				break;
			}
		}
	}

	/* not reachable */
	return 0;
}

/**
 * Power off.
 */
static void sigpoweroff(void)
{
	/* Any tasks to be done on power off should be performed here. */
	sync();
	reboot(RB_POWER_OFF);
}

static void sigreboot(void)
{
	/* Any tasks to be done on power off should be performed here. */
	sync();
	reboot(RB_AUTOBOOT);
}

/**
 * Remove zombie processes.
 */
static void sigreap(void)
{
	while(waitpid(-1, NULL, WNOHANG) > 0)
		;
}

static void spawn(char *const argv[])
{
	switch(fork())
	{
		case 0:
			if(sigprocmask(SIG_UNBLOCK, &set, NULL) != 0)
			{
				perror("sigprocmask");
				return;
			}

			if(setsid() < 0)
			{
				perror("setsid");
				return;
			}

			if(setgid(100) != 0)
			{
				perror("setgid");
				return;
			}

			if(setuid(1000) != 0)
			{
				perror("setuid");
				return;
			}

			execvp(argv[0], argv);
			perror("execvp");
			_exit(1);

		case -1:
			perror("fork");
	}
}
