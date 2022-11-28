#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


#define BUFFER_SIZE 256
#define POLL_MINTIME_NS 1000000

typedef struct message_line
{
	struct message_line* next;
	struct timespec expires;
	int length;
	char* buffer;
} message_line;

void enqueue_message(message_line** first, message_line** last, message_line* message)
{
	if (*last != NULL)
	{
		(*last)->next = message;
	}
	else
	{
		*first = message;
	}
	*last = message;
}

message_line* dequeue_message(message_line** first, message_line** last)
{
	message_line* result = *first;
	if (result != NULL)
	{
		*first = result->next;
		if (*first == NULL)
		{
			*last = NULL;
		}
	}
	return result;
}

volatile sig_atomic_t should_exit_error = 0;
volatile sig_atomic_t should_exit = 0;

void should_exit_handler(int sig)
{
	should_exit_error = sig;
	should_exit = 1;
}

void write_all(int fd, const void *buf, size_t count)
{
	while (count > 0)
	{
		int result = write(fd, buf, count);
		if (result > 0)
		{
			buf += result;
			count -= result;
		}
		else
		{
			if (result < 0)
			{
				result = errno;
				fprintf(stderr, "Error: %s\n", strerror(result));
				should_exit_error = result;
				should_exit = 1;
			}
			break;
		}
	}
}

int main(int argc, char* argv[], char* envp[])
{
	struct timespec max_age = { 1, 0 };
	char* filename = NULL;
	int help = (argc >= 2 && strcasecmp(argv[1], "--help") == 0);
	//Process arguments...
	int i = argc - 1;
	if (i >= 1)
	{
		filename = argv[i];
		i--;
	}
	if (i >= 1)
	{
		char* c = argv[i];
		long temp = strtol(c, &c, 10);
		while (isspace(*c))
		{
			c++;
		}
		if (*c == 0 && temp >= 0)
		{
			max_age.tv_sec = temp / 1000;
			max_age.tv_nsec = (temp % 1000) * 1000000;
		}
		else
		{
			printf("Unknown option: %s\n", argv[i]);
			help = 1;
		}
		i--;
	}
	if (help || filename == NULL || i >= 1)
	{
		printf("Usage:\n");
		printf("    fatalitee [<time_ms>] <file>\n\n");
		printf("    Echoes all input to the output, like tee.\n");
		printf("    Keeps only the last <time_ms> worth of messages (in milliseconds).\n");
		printf("    Writes them to <file> when input ends.\n");
		printf("    Default time to keep is 1 second.\n");
		return 0;
	}
	//Flag for termination when signals received...
	signal(SIGINT, should_exit_handler);
	signal(SIGTERM, should_exit_handler);
	signal(SIGHUP, should_exit_handler);
	//Open file...
	int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	if (file == -1)
	{
		int e = errno;
		fprintf(stderr, "Error: %s\n", strerror(e));
		return e;
	}
	//Prepare for polling...
	message_line* first = NULL;
	message_line* last = NULL;
	message_line* buffer = NULL;
	struct pollfd poll_stdin = { STDIN_FILENO, POLLIN, 0 };
	struct timespec now;
	sigset_t block_signals;
	sigemptyset(&block_signals);
	sigaddset(&block_signals, SIGINT);
	sigaddset(&block_signals, SIGTERM);
	sigaddset(&block_signals, SIGHUP);
	sigprocmask(SIG_BLOCK, &block_signals, NULL);
	sigemptyset(&block_signals);
	while (!should_exit)
	{
		//Compute timeout...
		if (first != NULL)
		{
			clock_gettime(CLOCK_MONOTONIC, &now);
			now.tv_sec = (first->expires.tv_sec - now.tv_sec);
			now.tv_nsec = (first->expires.tv_nsec - now.tv_nsec);
			if (now.tv_nsec < 0)
			{
				now.tv_sec--;
				now.tv_nsec += 1000000000;
			}
			if (now.tv_sec < 0)
			{
				now.tv_sec = 0;
				now.tv_nsec = POLL_MINTIME_NS;
			}
			else if (now.tv_sec == 0 && now.tv_nsec < POLL_MINTIME_NS)
			{
				now.tv_nsec = POLL_MINTIME_NS;
			}
		}
		else
		{
			now.tv_sec = 0;
			now.tv_nsec = 0;
		}
		//Wait for input...
		int e = ppoll(&poll_stdin, 1, (now.tv_sec == 0 && now.tv_nsec == 0) ? NULL : &now, &block_signals);
		if (e == -1)
		{
			e = errno;
			fprintf(stderr, "Error: %s\n", strerror(e));
			should_exit_error = e;
			should_exit = 1;
			break;
		}
		//Expire old messages...
		clock_gettime(CLOCK_MONOTONIC, &now);
		while (first != NULL && (first->expires.tv_sec < now.tv_sec || (first->expires.tv_sec == now.tv_sec && first->expires.tv_nsec < now.tv_nsec)))
		{
			if (buffer != NULL)
			{
				free(buffer);
			}
			buffer = dequeue_message(&first, &last);
		}
		//Read new message...
		if ((poll_stdin.revents & (POLLIN | POLLERR)) != 0)
		{
			if (buffer == NULL)
			{
				buffer = malloc(sizeof(*buffer) + BUFFER_SIZE);
				buffer->buffer = (char*)buffer + sizeof(*buffer);
			}
			buffer->next = NULL;
			buffer->length = read(STDIN_FILENO, buffer->buffer, BUFFER_SIZE);
			if (buffer->length > 0)
			{
				//Copy to stdout...
				write_all(STDOUT_FILENO, buffer->buffer, buffer->length);
				//Enqueue message...
				clock_gettime(CLOCK_MONOTONIC, &buffer->expires);
				buffer->expires.tv_sec += max_age.tv_sec;
				buffer->expires.tv_nsec += max_age.tv_nsec;
				if (buffer->expires.tv_nsec >= 1000000000)
				{
					buffer->expires.tv_sec++;
					buffer->expires.tv_nsec -= 1000000000;
				}
				enqueue_message(&first, &last, buffer);
				buffer = NULL;
			}
			else
			{
				//Check for error...
				e = 0;
				if (buffer->length < 0)
				{
					e = errno;
					fprintf(stderr, "Error: %s\n", strerror(e));
				}
				should_exit_error = e;
				should_exit = 1;
			}
		}
		else if ((poll_stdin.revents & POLLHUP) != 0)
		{
			should_exit_error = 0;
			should_exit = 1;
		}
	}
	//Expire old messages...
	if (buffer != NULL)
	{
		free(buffer);
		buffer = NULL;
	}
	clock_gettime(CLOCK_MONOTONIC, &now);
	while (first != NULL && (first->expires.tv_sec < now.tv_sec || (first->expires.tv_sec == now.tv_sec && first->expires.tv_nsec < now.tv_nsec)))
	{
		free(dequeue_message(&first, &last));
	}
	//Write messages that are new enough...
	while (first != NULL)
	{
		write_all(file, first->buffer, first->length);
		free(dequeue_message(&first, &last));
	}
	//Cleanup...
	close(file);
	return should_exit_error;
}
