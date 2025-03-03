#ifndef SHELL_ALLOC_H
#define SHELL_ALLOC_H

#include <stddef.h>

void	*sh_xmalloc(size_t size);
void	*sh_xcalloc(size_t count, size_t size);
char	*sh_xstrdup(const char *source);

#endif
