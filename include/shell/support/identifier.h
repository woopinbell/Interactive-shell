#ifndef SHELL_SUPPORT_IDENTIFIER_H
#define SHELL_SUPPORT_IDENTIFIER_H

#include <stddef.h>

int		sh_is_identifier_start(int c);
int		sh_is_identifier_char(int c);
size_t	sh_identifier_key_len(const char *text);
size_t	sh_parameter_identifier_len(const char *text);
int		sh_is_identifier(const char *text);
int		sh_is_assignment(const char *text);

#endif
