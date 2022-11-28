#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/capability.h>

#ifndef TARGET
	#error "Use -DTARGET='"/bin/bash"' to define the target binary. Arguments can be defined with -DARGS_PREFIX='"pre"' and -DARGS_SUFFIX='"post"'."
	#define TARGET ""
#endif
#ifndef ARGS_PREFIX
	#define ARGS_PREFIX
#endif
#ifndef ARGS_SUFFIX
	#define ARGS_SUFFIX
#endif
#ifndef ARGS_SKIP
	#define ARGS_SKIP 1
#endif
#ifndef CAPS
	#define CAPS
#endif

const char* target_bin = TARGET;
const char* target_args_prefix[] = { ARGS_PREFIX };
const char* target_args_suffix[] = { ARGS_SUFFIX };
const int target_skip_args = ARGS_SKIP;
const cap_value_t target_caps[] = { CAPS };

const int prefix_c = sizeof(target_args_prefix) / sizeof(*target_args_prefix);
const int suffix_c = sizeof(target_args_suffix) / sizeof(*target_args_suffix);
const int cap_c = sizeof(target_caps) / sizeof(*target_caps);

void explain_build()
{
	printf("grantcap.c, compiled with these options:\n");
	printf("-DTARGET='\"%s\"'\n", target_bin);
	if (prefix_c > 0)
	{
		printf("-DARGS_PREFIX='");
		for (int i = 0; i < prefix_c; i++)
		{
			printf("%s\"%s\"", (i == 0) ? "" : ", ", target_args_prefix[i]);
		}
		printf("'\n");
	}
	if (suffix_c > 0)
	{
		printf("-DARGS_SUFFIX='");
		for (int i = 0; i < suffix_c; i++)
		{
			printf("%s\"%s\"", (i == 0) ? "" : ", ", target_args_suffix[i]);
		}
		printf("'\n");
	}
	if (target_skip_args != 1)
	{
		printf("-DARGS_SKIP=%d\n", target_skip_args);
	}
	if (cap_c > 0)
	{
		printf("-DCAPS='");
		for (int i = 0; i < cap_c; i++)
		{
			char* name = cap_to_name(target_caps[i]);
			printf("%s%s", (i == 0) ? "" : ", ", name);
			cap_free(name);
		}
		printf("'\n");
	}
}

int main(int argc, char* argv[], char* envp[])
{
	//Check if we're being interrogated...
	if (argc == 2 && strcmp(argv[1], "--explain-build") == 0)
	{
		explain_build();
		return 0;
	}
	//Activate new capabilities...
	if (cap_c > 0)
	{
		cap_t caps = cap_get_proc();
		if (caps == NULL || cap_set_flag(caps, CAP_INHERITABLE, cap_c, target_caps, CAP_SET) != 0 || cap_set_flag(caps, CAP_PERMITTED, cap_c, target_caps, CAP_SET) != 0 || cap_set_proc(caps) != 0)
		{
			int e = errno;
			printf("Failed to activate capabilities: %s\n", strerror(e));
			return e;
		}
		cap_free(caps);
		for (int i = 0; i < cap_c; i++)
		{
			if (cap_set_ambient(target_caps[i], CAP_SET) != 0)
			{
				int e = errno;
				printf("Failed to make capabilities ambient: %s\n", strerror(e));
				return e;
			}
		}
	}
	//Create modified arguments...
	int j = argc - target_skip_args;
	if (j < 0)
	{
		j = 0;
	}
	char* new_argv[1 + prefix_c + j + suffix_c + 1];
	j = 0;
	new_argv[j++] = (char*)target_bin;
	for (int i = 0; i < prefix_c; i++)
	{
		new_argv[j++] = (char*)target_args_prefix[i];
	}
	for (int i = target_skip_args; i < argc; i++)
	{
		new_argv[j++] = argv[i];
	}
	for (int i = 0; i < suffix_c; i++)
	{
		new_argv[j++] = (char*)target_args_suffix[i];
	}
	new_argv[j] = NULL;
	//Launch target...
	return execve(target_bin, new_argv, envp);
}
