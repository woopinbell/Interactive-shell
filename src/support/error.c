#include "shell/support/error.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static void	sh_error_prefix(void)
{
	fputs("shell", stderr);
}

void	sh_error(const char *subject, const char *message)
{
	sh_error_prefix();
	if (subject != NULL && subject[0] != '\0')
		fprintf(stderr, ": %s", subject);
	if (message != NULL && message[0] != '\0')
		fprintf(stderr, ": %s", message);
	fputc('\n', stderr);
}

void	sh_perror(const char *subject, const char *message)
{
	int	saved_errno;

	saved_errno = errno;
	sh_error_prefix();
	if (subject != NULL && subject[0] != '\0')
		fprintf(stderr, ": %s", subject);
	if (message != NULL && message[0] != '\0')
		fprintf(stderr, ": %s", message);
	fprintf(stderr, ": %s\n", strerror(saved_errno));
}
