#include "shell/env.h"

#include "shell/support/alloc.h"

#include <stdlib.h>

t_env_node	*sh_env_node_new(const char *key, const char *value, int has_value)
{
	t_env_node	*node;

	node = sh_xcalloc(1, sizeof(t_env_node));
	node->key = sh_xstrdup(key);
	if (has_value)
		node->value = sh_xstrdup(value);
	node->has_value = has_value;
	return (node);
}

void	sh_env_node_destroy(t_env_node *node)
{
	if (node == NULL)
		return ;
	free(node->key);
	free(node->value);
	free(node);
}

void	sh_env_store_init(t_env_store *store)
{
	store->head = NULL;
	store->tail = NULL;
	store->size = 0;
}

void	sh_env_store_append(t_env_store *store, t_env_node *node)
{
	if (node == NULL)
		return ;
	node->next = NULL;
	if (store->tail == NULL)
		store->head = node;
	else
		store->tail->next = node;
	store->tail = node;
	store->size += 1;
}

void	sh_env_store_destroy(t_env_store *store)
{
	t_env_node	*node;
	t_env_node	*next;

	node = store->head;
	while (node != NULL)
	{
		next = node->next;
		sh_env_node_destroy(node);
		node = next;
	}
	sh_env_store_init(store);
}
