#ifndef SHELL_SUPPORT_PATH_H
#define SHELL_SUPPORT_PATH_H

#include <stddef.h>

char	*sh_path_join(const char *left, const char *right);
char	*sh_path_join_n(const char *left, const char *right, size_t right_len);

#endif
