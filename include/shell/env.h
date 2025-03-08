#ifndef SHELL_ENV_H
#define SHELL_ENV_H

#include <stddef.h>

typedef struct s_env_node
{
	char				*key;
	char				*value;
	int					has_value;
	struct s_env_node	*next;
}	t_env_node;

typedef struct s_env_store
{
	t_env_node	*head;
	t_env_node	*tail;
	size_t		size;
}	t_env_store;

t_env_node	*sh_env_node_new(const char *key, const char *value, int has_value);
void		sh_env_node_destroy(t_env_node *node);
void		sh_env_store_init(t_env_store *store);
void		sh_env_store_init_from_envp(t_env_store *store, char **envp);
void		sh_env_store_append(t_env_store *store, t_env_node *node);
t_env_node	*sh_env_store_find(t_env_store *store, const char *key);
const char	*sh_env_store_get(t_env_store *store, const char *key);
int			sh_env_store_has(t_env_store *store, const char *key);
t_env_node	*sh_env_store_set(t_env_store *store, const char *key,
				const char *value, int has_value);
int			sh_env_store_unset(t_env_store *store, const char *key);
void		sh_env_store_destroy(t_env_store *store);

#endif
