#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

int main(int argc, char* argv[], char* envp[])
{
	struct timespec timeout = { 0 };
	int help = (argc <= 1);
	//Process arguments...
	for (int i = 1; i < argc; i++)
	{
		char* c = argv[i];
		long temp = strtol(c, &c, 10);
		while (isspace(*c))
		{
			c++;
		}
		if (*c == 0 && temp >= 0)
		{
			timeout.tv_sec = temp / 1000;
			timeout.tv_nsec = (temp % 1000) * 1000000;
		}
		else if (strcasecmp(argv[i], "--help") == 0)
		{
			help = 1;
		}
		else
		{
			printf("Unknown option: %s\n", argv[i]);
			help = 1;
		}
	}
	if (help)
	{
		printf("Usage:\n");
		printf("    fragile <timeout_ms>\n\n");
		printf("    Waits for any signal, with timeout (in milliseconds).\n");
		printf("    Timeout of 0 is infinite.\n");
		printf("    Exit code is signal number, 0 means timeout.\n");
		return 0;
	}
	//Wait for a signal...
	sigset_t signals;
	sigfillset(&signals);
	sigprocmask(SIG_BLOCK, &signals, NULL);
	int result = sigtimedwait(&signals, NULL, (timeout.tv_sec == 0 && timeout.tv_nsec == 0) ? NULL : &timeout);
	if (result == EAGAIN)
	{
		result = 0;
	}
	return result;
}
