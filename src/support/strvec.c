#include "shell/support/strvec.h"

#include "shell/support/alloc.h"

#include <stdlib.h>

#define SH_STRVEC_MIN_CAPACITY 4

static void	sh_strvec_bootstrap(t_strvec *vec)
{
	vec->capacity = SH_STRVEC_MIN_CAPACITY;
	vec->data = sh_xcalloc(vec->capacity + 1, sizeof(char *));
	vec->len = 0;
}

static void	sh_strvec_reserve(t_strvec *vec, size_t needed)
{
	size_t	new_capacity;
	char	**new_data;
	size_t	index;

	if (vec->capacity >= needed)
		return ;
	new_capacity = vec->capacity;
	while (new_capacity < needed)
		new_capacity *= 2;
	new_data = sh_xcalloc(new_capacity + 1, sizeof(char *));
	index = 0;
	while (index < vec->len)
	{
		new_data[index] = vec->data[index];
		index++;
	}
	free(vec->data);
	vec->data = new_data;
	vec->capacity = new_capacity;
}

void	sh_strvec_init(t_strvec *vec)
{
	sh_strvec_bootstrap(vec);
}

void	sh_strvec_push(t_strvec *vec, const char *value)
{
	sh_strvec_reserve(vec, vec->len + 1);
	vec->data[vec->len] = sh_xstrdup(value);
	vec->len += 1;
	vec->data[vec->len] = NULL;
}

char	**sh_strvec_take(t_strvec *vec)
{
	char	**taken;

	taken = vec->data;
	sh_strvec_bootstrap(vec);
	return (taken);
}

void	sh_strtab_destroy(char **items)
{
	size_t	index;

	if (items == NULL)
		return ;
	index = 0;
	while (items[index] != NULL)
	{
		free(items[index]);
		index++;
	}
	free(items);
}

void	sh_strvec_destroy(t_strvec *vec)
{
	sh_strtab_destroy(vec->data);
	vec->data = NULL;
	vec->len = 0;
	vec->capacity = 0;
}
