#include "shell/env.h"

#include "shell/support/alloc.h"
#include "shell/support/strbuf.h"
#include "shell/support/strvec.h"

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

static char	*sh_env_store_join_pair(const char *key, const char *value)
{
	t_strbuf	buf;
	char		*joined;

	sh_strbuf_init(&buf);
	sh_strbuf_append(&buf, key);
	sh_strbuf_push_char(&buf, '=');
	sh_strbuf_append(&buf, value);
	joined = sh_strbuf_take(&buf);
	sh_strbuf_free(&buf);
	return (joined);
}

static void	sh_env_store_append_escaped_value(t_strbuf *buf, const char *value)
{
	size_t	index;

	index = 0;
	while (value[index] != '\0')
	{
		if (value[index] == '\\' || value[index] == '"'
			|| value[index] == '$' || value[index] == '`')
			sh_strbuf_push_char(buf, '\\');
		sh_strbuf_push_char(buf, value[index]);
		index++;
	}
}

static char	*sh_env_store_format_export_node(const t_env_node *node)
{
	t_strbuf	buf;
	char		*formatted;

	sh_strbuf_init(&buf);
	sh_strbuf_append(&buf, "declare -x ");
	sh_strbuf_append(&buf, node->key);
	if (node->has_value)
	{
		sh_strbuf_append(&buf, "=\"");
		sh_env_store_append_escaped_value(&buf, node->value);
		sh_strbuf_push_char(&buf, '"');
	}
	formatted = sh_strbuf_take(&buf);
	sh_strbuf_free(&buf);
	return (formatted);
}

static int	sh_env_node_key_compare(const void *left, const void *right)
{
	const t_env_node *const	*node_left;
	const t_env_node *const	*node_right;

	node_left = left;
	node_right = right;
	return (strcmp((*node_left)->key, (*node_right)->key));
}

static t_env_node	**sh_env_store_sorted_nodes(const t_env_store *store)
{
	t_env_node	**nodes;
	t_env_node	*current;
	size_t		index;

	nodes = sh_xcalloc(store->size, sizeof(t_env_node *));
	current = store->head;
	index = 0;
	while (current != NULL)
	{
		nodes[index] = current;
		current = current->next;
		index++;
	}
	if (store->size > 1)
		qsort(nodes, store->size, sizeof(t_env_node *), sh_env_node_key_compare);
	return (nodes);
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

char	**sh_env_store_to_envp(const t_env_store *store)
{
	t_strvec	entries;
	t_env_node	*node;
	char		*entry;
	char		**serialized;

	sh_strvec_init(&entries);
	node = store->head;
	while (node != NULL)
	{
		if (node->has_value)
		{
			entry = sh_env_store_join_pair(node->key, node->value);
			sh_strvec_push(&entries, entry);
			free(entry);
		}
		node = node->next;
	}
	serialized = sh_strvec_take(&entries);
	sh_strvec_destroy(&entries);
	return (serialized);
}

char	**sh_env_store_format_export(const t_env_store *store)
{
	t_strvec	entries;
	t_env_node	**nodes;
	char		*line;
	char		**formatted;
	size_t		index;

	sh_strvec_init(&entries);
	nodes = sh_env_store_sorted_nodes(store);
	index = 0;
	while (index < store->size)
	{
		line = sh_env_store_format_export_node(nodes[index]);
		sh_strvec_push(&entries, line);
		free(line);
		index++;
	}
	free(nodes);
	formatted = sh_strvec_take(&entries);
	sh_strvec_destroy(&entries);
	return (formatted);
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
