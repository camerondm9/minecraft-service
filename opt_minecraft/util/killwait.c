#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef SIGNALS
#define SIGNALS X(SIGHUP) X(SIGINT) X(SIGQUIT) X(SIGKILL) X(SIGUSR1) X(SIGUSR2) X(SIGTERM) X(SIGCONT) X(SIGSTOP)
#endif

#define X(m) #m,
const char* signal_names[] = { SIGNALS };
#undef X
#define X(m) m,
const int signal_numbers[] = { SIGNALS };
#undef X

//Compile time constraints to cause errors if things are very broken
const int signal_c = ((sizeof(signal_names) / sizeof(*signal_names)) == (sizeof(signal_numbers) / sizeof(*signal_numbers))) ? (sizeof(signal_names) / sizeof(*signal_names)) : (1 / 0);
#define X(m) (sizeof(#m) > 4)&&
const int signal_name_length_check = (SIGNALS 1) ? 0 : (1 / 0);
#undef X
//End compile time constraints

#if defined(SYS_pidfd_open) && defined(SYS_pidfd_send_signal)
	#define USE_POLL 1
#endif

void remove_pid(pid_t* array, int* count, int index)
{
	(*count)--;
	while (index < *count)
	{
		array[index] = array[index + 1];
		index++;
	}
	array[index] = 0;
}
void remove_fd_pid(struct pollfd* array1, pid_t* array2, int* count, int index)
{
	(*count)--;
	while (index < *count)
	{
		array1[index] = array1[index + 1];
		array2[index] = array2[index + 1];
		index++;
	}
	array1[index] = (struct pollfd){ 0 };
	array2[index] = 0;
}

int str_startswithi(const char* str, const char* prefix)
{
	return strncasecmp(str, prefix, strlen(prefix)) == 0;
}

void alarm_handler(int sig_number)
{
	_exit(1);
}

int main(int argc, char* argv[], char* envp[])
{
	//Process options...
	int verbose = 0;
	int timeout = -1;
	int sig = SIGTERM;
	pid_t pids[argc];
	int pid_c = 0;
	int mode = 0;
	for (int i = 1; i < argc; i++)
	{
		char* c = argv[i];
		if (mode == 1)
		{
			mode = 0;
			goto timeout;
		}
		else if (mode == 2)
		{
			mode = 0;
			goto signal;
		}
		if (*c == '-')
		{
			//Skip dashes...
			do
			{
				c++;
			} while (*c == '-');
			//Option...
			if (isdigit(*c))
			{
				long temp = strtol(c, &c, 10);
				while (isspace(*c))
				{
					c++;
				}
				if (*c == 0 && 0 <= temp && temp < NSIG)
				{
					sig = temp;
					continue;
				}
			}
			else
			{
				int j;
				for (j = 0; j < signal_c; j++)
				{
					if (strcasecmp(c, signal_names[j]) == 0 || strcasecmp(c, signal_names[j] + 3) == 0)
					{
						break;
					}
				}
				if (j < signal_c)
				{
					sig = signal_numbers[j];
					continue;
				}
				else if (strcasecmp(c, "version") == 0)
				{
					printf("killwait by Cameron Martens, version 0.1\n");
					return 1;
				}
				else if (strcasecmp(c, "help") == 0 || strcasecmp(c, "h") == 0 || strcmp(c, "?") == 0)
				{
					printf("Usage:\n");
					printf("    killwait [-s <signal> | -<signal>] [-t <time_ms>] <pid>...\n\n");
					printf("    Send a signal to a process (or multiple processes) and wait for them to exit.\n");
					printf("    If no signal is specified, the default is SIGTERM.\n");
					printf("    If no timeout is specified, the default is infinite.\n\n");
					printf("Options:\n");
					printf("    <pid>...        send signal to every <pid> specified\n");
					printf("    -<signal>, -s <signal>, --signal <signal>\n");
					printf("                    specify the <signal> to send\n");
					printf("    -t <time_ms>    specify the maximum time to wait (in milliseconds)\n");
					printf("    -l, --table     list all recognized signal names\n\n");
					printf("    -h, --help      display this help and exit\n");
					printf("    -v, --verbose   show information and error messages\n");
					printf("    --version       display version information and exit\n");
					return 1;
				}
				else if (strcasecmp(c, "verbose") == 0 || strcasecmp(c, "v") == 0)
				{
					verbose = 1;
					continue;
				}
				else if (strcasecmp(c, "table") == 0 || strcasecmp(c, "list") == 0 || strcasecmp(c, "l") == 0)
				{
					for (int i = 0; i < signal_c; i++)
					{
						printf("%2d) %s\n", signal_numbers[i], signal_names[i]);
					}
					return 1;
				}
				else if (str_startswithi(c, "timeout"))
				{
					c += 7;
					if (*c == '=')
					{
						c++;
						goto timeout;
					}
					else if (*c == 0)
					{
						mode = 1;
						continue;
					}
				}
				else if (str_startswithi(c, "t"))
				{
					c++;
					if (*c == '=')
					{
						c++;
					}
					if (*c == 0)
					{
						mode = 1;
						continue;
					}
timeout:			if (isdigit(*c))
					{
						long temp = strtol(c, &c, 10);
						if (strcasecmp(c, "s") == 0)
						{
							c++;
							temp *= 1000;
						}
						else if (strcasecmp(c, "ms") == 0)
						{
							c += 2;
						}
						while (isspace(*c))
						{
							c++;
						}
						if (*c == 0 && 0 < temp && temp < INT_MAX)
						{
							timeout = temp;
							continue;
						}
					}
				}
				else if (str_startswithi(c, "signal"))
				{
					c += 6;
					if (*c == '=')
					{
						c++;
						goto signal;
					}
					else if (*c == 0)
					{
						mode = 2;
						continue;
					}
				}
				else if (str_startswithi(c, "s"))
				{
					c++;
					if (*c == '=')
					{
						c++;
					}
					else if (*c == 0)
					{
						mode = 2;
						continue;
					}
signal:				if (isdigit(*c))
					{
						long temp = strtol(c, &c, 10);
						while (isspace(*c))
						{
							c++;
						}
						if (*c == 0 && 0 <= temp && temp < NSIG)
						{
							sig = temp;
							continue;
						}
					}
					else
					{
						int j;
						for (j = 0; j < signal_c; j++)
						{
							if (strcasecmp(c, signal_names[j]) == 0 || strcasecmp(c, signal_names[j] + 3) == 0)
							{
								break;
							}
						}
						if (j < signal_c)
						{
							sig = signal_numbers[j];
							continue;
						}
					}
				}
			}
			printf("Unknown option %s.\n", argv[i]);
			return 1;
		}
		else if (*c != 0)
		{
			//PID...
			long temp = strtol(c, &c, 10);
			while (isspace(*c))
			{
				c++;
			}
			if (*c == 0 && 0 < temp && temp == (pid_t)temp)
			{
				pids[pid_c++] = (pid_t)temp;
			}
			else
			{
				printf("Ignoring invalid PID %s.\n", argv[i]);
			}
		}
	}
	//Check if we have enough information...
	if (pid_c <= 0)
	{
		printf("Usage:    killwait [-s <signal> | -<signal>] [-t <time_ms>] <pid>...\n");
		return 1;
	}
	int sent = 0;
#ifdef USE_POLL
	//Begin monitoring processes...
	struct pollfd fds[pid_c];
	pid_t fd_pids[pid_c];
	int fd_c = 0;
	for (int i = pid_c - 1; i >= 0; i--)
	{
		int temp = syscall(SYS_pidfd_open, pids[i], 0);
		if (temp > 0)
		{
			fds[fd_c].events = POLLIN;
			fd_pids[fd_c] = pids[i];
			fds[fd_c++].fd = temp;
			//Use poll(pid_fd) or manual polling, but not both...
			remove_pid(pids, &pid_c, i);
		}
		else if (verbose)
		{
			printf("pidfd_open(%d) failure: %s\n", pids[i], strerror(errno));
		}
	}
	//Send signals...
	for (int i = fd_c - 1; i >= 0; i--)
	{
		if (syscall(SYS_pidfd_send_signal, fds[i].fd, sig, NULL, 0) == 0)
		{
			printf("pidfd_send_signal(%d) success\n", fd_pids[i]);
			sent++;
		}
		else
		{
			if (verbose)
			{
				printf("pidfd_send_signal(%d) failure: %s.\n", fd_pids[i], strerror(errno));
			}
			remove_fd_pid(fds, fd_pids, &fd_c, i);
		}
	}
#endif
	//Send more signals...
	for (int i = pid_c - 1; i >= 0; i--)
	{
		if (kill(pids[i], sig) == 0)
		{
			printf("kill(%d) success\n", pids[i]);
			sent++;
		}
		else
		{
			if (verbose)
			{
				printf("kill(%d) failure: %s.\n", pids[i], strerror(errno));
			}
			remove_pid(pids, &pid_c, i);
		}
	}
	if (verbose || sent == 0)
	{
		printf("Sent %d signal%s.\n", sent, (sent != 1) ? "s" : "");
		if (sent == 0)
		{
			return 1;
		}
	}
	//Start timer...
	if (timeout > 0)
	{
		signal(SIGALRM, alarm_handler);
		struct itimerval timer = { 0 };
		timer.it_value.tv_sec = timeout / 1000;
		timer.it_value.tv_usec = (timeout % 1000) * 1000;
		setitimer(ITIMER_REAL, &timer, NULL);
	}
#ifdef USE_POLL
	//Wait for processes to exit...
	while (fd_c > 0)
	{
		int changes = poll(fds, fd_c, -1);
		if (changes <= 0)
		{
			int e = errno;
			if (e != EINTR)
			{
				printf("Failed to wait for processes: %s\n", strerror(e));
				return 1;
			}
		}
		else
		{
			for (int i = fd_c - 1; i >= 0; i--)
			{
				if (fds[i].revents != 0)
				{
					//Process terminated or poll error...
					remove_fd_pid(fds, fd_pids, &fd_c, i);
				}
			}
		}
	}
#endif
	//Wait for more processes to exit...
	while (pid_c > 0)
	{
		struct timespec timer = { 0 };
		timer.tv_nsec = 10000000;
		nanosleep(&timer, NULL);
		for (int i = pid_c - 1; i >= 0; i--)
		{
			if (kill(pids[i], 0) != 0)
			{
				remove_pid(pids, &pid_c, i);
			}
		}
	}
	return 0;
}
