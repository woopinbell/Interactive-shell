#include "shell/builtin.h"

#include <stddef.h>
#include <string.h>

typedef struct s_builtin_entry
{
	const char		*name;
	t_builtin_fn	run;
}	t_builtin_entry;

static const t_builtin_entry	*sh_builtin_registry(void)
{
	static const t_builtin_entry	registry[] = {
		{"echo", sh_builtin_echo},
		{"pwd", sh_builtin_pwd},
		{"env", sh_builtin_env},
		{"true", sh_builtin_true},
		{"false", sh_builtin_false},
		{NULL, NULL}
	};

	return (registry);
}

t_builtin_fn	sh_builtin_find(const char *name)
{
	const t_builtin_entry	*entry;

	entry = sh_builtin_registry();
	while (entry->name != NULL)
	{
		if (strcmp(entry->name, name) == 0)
			return (entry->run);
		entry++;
	}
	return (NULL);
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
