#include "shell/expand.h"

#include "shell/support/alloc.h"
#include "shell/support/identifier.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void	sh_parameter_expansion_init(t_parameter_expansion *expansion)
{
	expansion->kind = SH_PARAMETER_NONE;
	expansion->name = NULL;
	expansion->name_len = 0;
	expansion->span_len = 0;
}

size_t	sh_parameter_scan(const char *text, t_parameter_expansion *expansion)
{
	size_t	name_len;

	sh_parameter_expansion_init(expansion);
	if (text == NULL || text[0] != '$')
		return (0);
	if (text[1] == '?')
	{
		expansion->kind = SH_PARAMETER_STATUS;
		expansion->name = text + 1;
		expansion->name_len = 1;
		expansion->span_len = 2;
		return (expansion->span_len);
	}
	name_len = sh_parameter_identifier_len(text + 1);
	if (name_len == 0)
		return (0);
	expansion->kind = SH_PARAMETER_VARIABLE;
	expansion->name = text + 1;
	expansion->name_len = name_len;
	expansion->span_len = name_len + 1;
	return (expansion->span_len);
}

static char	*sh_parameter_status_value(int last_status)
{
	char	buffer[32];

	snprintf(buffer, sizeof(buffer), "%d", last_status);
	return (sh_xstrdup(buffer));
}

static char	*sh_parameter_variable_value(t_env_store *env,
		const t_parameter_expansion *expansion)
{
	char		*key;
	const char	*value;

	key = sh_xmalloc(expansion->name_len + 1);
	memcpy(key, expansion->name, expansion->name_len);
	key[expansion->name_len] = '\0';
	value = sh_env_store_get(env, key);
	free(key);
	if (value == NULL)
		return (sh_xstrdup(""));
	return (sh_xstrdup(value));
}

char	*sh_parameter_lookup(t_env_store *env, int last_status,
		const t_parameter_expansion *expansion)
{
	if (expansion == NULL || expansion->kind == SH_PARAMETER_NONE)
		return (NULL);
	if (expansion->kind == SH_PARAMETER_STATUS)
		return (sh_parameter_status_value(last_status));
	return (sh_parameter_variable_value(env, expansion));
}

char	*sh_parameter_expand(t_env_store *env, int last_status,
		const char *text, size_t *consumed)
{
	t_parameter_expansion	expansion;
	size_t					span_len;

	span_len = sh_parameter_scan(text, &expansion);
	if (consumed != NULL)
		*consumed = span_len;
	if (span_len == 0)
		return (NULL);
	return (sh_parameter_lookup(env, last_status, &expansion));
}
