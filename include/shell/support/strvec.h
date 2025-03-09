#ifndef SHELL_SUPPORT_STRVEC_H
#define SHELL_SUPPORT_STRVEC_H

#include <stddef.h>

typedef struct s_strvec
{
	char	**data;
	size_t	len;
	size_t	capacity;
}	t_strvec;

void	sh_strvec_init(t_strvec *vec);
void	sh_strvec_push(t_strvec *vec, const char *value);
char	**sh_strvec_take(t_strvec *vec);
void	sh_strtab_destroy(char **items);
void	sh_strvec_destroy(t_strvec *vec);

#endif
