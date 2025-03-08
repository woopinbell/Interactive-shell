#include "shell/env.h"

#include "shell/support/alloc.h"

#include <stdlib.h>
#include <string.h>

static char	*sh_env_dup_range(const char *start, size_t len)
{
	char	*copy;

	copy = sh_xmalloc(len + 1);
	memcpy(copy, start, len);
	copy[len] = '\0';
	return (copy);
}

static const char	*sh_env_value_or_empty(const char *value)
{
	if (value == NULL)
		return ("");
	return (value);
}

static int	sh_env_key_equals(const t_env_node *node, const char *key)
{
	return (strcmp(node->key, key) == 0);
}

static void	sh_env_node_replace_value(t_env_node *node, const char *value,
		int has_value)
{
	free(node->value);
	node->value = NULL;
	if (has_value)
		node->value = sh_xstrdup(sh_env_value_or_empty(value));
	node->has_value = has_value;
}

static void	sh_env_store_load_entry(t_env_store *store, const char *entry)
{
	const char	*equal_sign;
	char		*key;

	equal_sign = strchr(entry, '=');
	if (equal_sign == NULL)
	{
		sh_env_store_append(store, sh_env_node_new(entry, NULL, 0));
		return ;
	}
	key = sh_env_dup_range(entry, (size_t)(equal_sign - entry));
	sh_env_store_append(store, sh_env_node_new(key, equal_sign + 1, 1));
	free(key);
}

t_env_node	*sh_env_node_new(const char *key, const char *value, int has_value)
{
	t_env_node	*node;

	node = sh_xcalloc(1, sizeof(t_env_node));
	node->key = sh_xstrdup(key);
	if (has_value)
		node->value = sh_xstrdup(sh_env_value_or_empty(value));
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

void	sh_env_store_init_from_envp(t_env_store *store, char **envp)
{
	size_t	index;

	sh_env_store_init(store);
	index = 0;
	while (envp != NULL && envp[index] != NULL)
	{
		sh_env_store_load_entry(store, envp[index]);
		index++;
	}
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

t_env_node	*sh_env_store_find(t_env_store *store, const char *key)
{
	t_env_node	*node;

	node = store->head;
	while (node != NULL)
	{
		if (sh_env_key_equals(node, key))
			return (node);
		node = node->next;
	}
	return (NULL);
}

const char	*sh_env_store_get(t_env_store *store, const char *key)
{
	t_env_node	*node;

	node = sh_env_store_find(store, key);
	if (node == NULL || !node->has_value)
		return (NULL);
	return (node->value);
}

int	sh_env_store_has(t_env_store *store, const char *key)
{
	return (sh_env_store_find(store, key) != NULL);
}

t_env_node	*sh_env_store_set(t_env_store *store, const char *key,
		const char *value, int has_value)
{
	t_env_node	*node;

	node = sh_env_store_find(store, key);
	if (node != NULL)
	{
		sh_env_node_replace_value(node, value, has_value);
		return (node);
	}
	node = sh_env_node_new(key, value, has_value);
	sh_env_store_append(store, node);
	return (node);
}

int	sh_env_store_unset(t_env_store *store, const char *key)
{
	t_env_node	*prev;
	t_env_node	*node;

	prev = NULL;
	node = store->head;
	while (node != NULL)
	{
		if (sh_env_key_equals(node, key))
		{
			if (prev == NULL)
				store->head = node->next;
			else
				prev->next = node->next;
			if (store->tail == node)
				store->tail = prev;
			store->size -= 1;
			sh_env_node_destroy(node);
			return (1);
		}
		prev = node;
		node = node->next;
	}
	return (0);
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
