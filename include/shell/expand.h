#ifndef SHELL_EXPAND_H
#define SHELL_EXPAND_H

#include "shell/env.h"

#include <stddef.h>

typedef enum e_parameter_kind
{
	SH_PARAMETER_NONE,
	SH_PARAMETER_VARIABLE,
	SH_PARAMETER_STATUS
}	t_parameter_kind;

typedef struct s_parameter_expansion
{
	t_parameter_kind	kind;
	const char			*name;
	size_t				name_len;
	size_t				span_len;
}	t_parameter_expansion;

void	sh_parameter_expansion_init(t_parameter_expansion *expansion);
size_t	sh_parameter_scan(const char *text, t_parameter_expansion *expansion);
char	*sh_parameter_lookup(t_env_store *env, int last_status,
			const t_parameter_expansion *expansion);
char	*sh_parameter_expand(t_env_store *env, int last_status,
			const char *text, size_t *consumed);

#endif
