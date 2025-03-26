#include "shell/exec.h"

#include "shell/support/alloc.h"
#include "shell/support/path.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char	*sh_executor_path_entry_candidate(const char *entry,
		size_t entry_len, const char *command_name)
{
	char	*directory;
	char	*candidate;

	if (entry_len == 0)
		return (sh_xstrdup(command_name));
	directory = sh_path_join_n("", entry, entry_len);
	candidate = sh_path_join(directory, command_name);
	free(directory);
	return (candidate);
}

static char	*sh_executor_resolve_path_entries(const char *path_value,
		const char *command_name)
{
	const char	*entry;
	const char	*separator;
	char		*candidate;

	entry = path_value;
	while (entry != NULL)
	{
		separator = strchr(entry, ':');
		if (separator == NULL)
			candidate = sh_executor_path_entry_candidate(entry,
					strlen(entry), command_name);
		else
			candidate = sh_executor_path_entry_candidate(entry,
					(size_t)(separator - entry), command_name);
		if (access(candidate, F_OK) == 0)
			return (candidate);
		free(candidate);
		if (separator == NULL)
			break ;
		entry = separator + 1;
	}
	return (NULL);
}

int	sh_executor_command_has_path(const char *command_name)
{
	if (command_name == NULL)
		return (0);
	return (strchr(command_name, '/') != NULL);
}

char	*sh_executor_resolve_direct_path(const char *command_name)
{
	if (!sh_executor_command_has_path(command_name))
		return (NULL);
	return (sh_xstrdup(command_name));
}

char	*sh_executor_resolve_path_search(t_shell *shell, const char *command_name)
{
	const char	*path_value;

	if (command_name == NULL || command_name[0] == '\0')
		return (NULL);
	if (sh_executor_command_has_path(command_name))
		return (NULL);
	path_value = sh_env_store_get(&shell->env, "PATH");
	if (path_value == NULL || path_value[0] == '\0')
		return (NULL);
	return (sh_executor_resolve_path_entries(path_value, command_name));
}

char	*sh_executor_resolve_command_path(t_shell *shell,
		const char *command_name)
{
	char	*resolved;

	resolved = sh_executor_resolve_direct_path(command_name);
	if (resolved != NULL)
		return (resolved);
	return (sh_executor_resolve_path_search(shell, command_name));
}
