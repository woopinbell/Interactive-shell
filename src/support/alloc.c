#include "shell/support/alloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void	sh_xalloc_abort(void)
{
	fputs("shell: fatal: memory allocation failed\n", stderr);
	exit(EXIT_FAILURE);
}

void	*sh_xmalloc(size_t size)
{
	void	*memory;

	if (size == 0)
		size = 1;
	memory = malloc(size);
	if (memory == NULL)
		sh_xalloc_abort();
	return (memory);
}

void	*sh_xcalloc(size_t count, size_t size)
{
	void	*memory;

	if (count == 0 || size == 0)
	{
		count = 1;
		size = 1;
	}
	memory = calloc(count, size);
	if (memory == NULL)
		sh_xalloc_abort();
	return (memory);
}

char	*sh_xstrdup(const char *source)
{
	size_t	length;
	char	*copy;

	length = strlen(source) + 1;
	copy = sh_xmalloc(length);
	memcpy(copy, source, length);
	return (copy);
}
