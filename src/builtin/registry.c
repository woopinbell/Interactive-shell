#include "shell/builtin.h"

#include <stddef.h>
#include <string.h>

typedef struct s_builtin_entry
{
	const char		*name;
	t_builtin_fn	run;
	int				runs_in_parent;
}	t_builtin_entry;

static const t_builtin_entry	*sh_builtin_registry(void)
{
	static const t_builtin_entry	registry[] = {
		{"echo", sh_builtin_echo, 0},
		{"pwd", sh_builtin_pwd, 0},
		{"env", sh_builtin_env, 0},
		{"true", sh_builtin_true, 0},
		{"false", sh_builtin_false, 0},
		{"cd", sh_builtin_cd, 1},
		{"export", sh_builtin_export, 1},
		{"unset", sh_builtin_unset, 1},
		{"exit", sh_builtin_exit, 1},
		{NULL, NULL, 0}
	};

	return (registry);
}

static const t_builtin_entry	*sh_builtin_find_entry(const char *name)
{
	const t_builtin_entry	*entry;

	entry = sh_builtin_registry();
	while (entry->name != NULL)
	{
		if (strcmp(entry->name, name) == 0)
			return (entry);
		entry++;
	}
	return (NULL);
}

t_builtin_fn	sh_builtin_find(const char *name)
{
	const t_builtin_entry	*entry;

	entry = sh_builtin_find_entry(name);
	if (entry == NULL)
		return (NULL);
	return (entry->run);
}

int	sh_builtin_runs_in_parent(const char *name)
{
	const t_builtin_entry	*entry;

	entry = sh_builtin_find_entry(name);
	if (entry == NULL)
		return (0);
	return (entry->runs_in_parent);
}

int	sh_builtin_dispatch(t_shell *shell, int argc, char **argv, int *handled)
{
	t_builtin_fn	run;

	if (handled != NULL)
		*handled = 0;
	if (argc == 0 || argv == NULL || argv[0] == NULL)
		return (0);
	run = sh_builtin_find(argv[0]);
	if (run == NULL)
		return (0);
	if (handled != NULL)
		*handled = 1;
	return (run(shell, argc, argv));
}
