#include "shell/builtin.h"

#include "shell/support/alloc.h"
#include "shell/support/error.h"
#include "shell/support/identifier.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void	sh_builtin_free_lines(char **lines)
{
	size_t	index;

	index = 0;
	while (lines != NULL && lines[index] != NULL)
	{
		free(lines[index]);
		index++;
	}
	free(lines);
}

static void	sh_builtin_invalid_identifier(const char *builtin,
		const char *argument)
{
	fprintf(stderr, "shell: %s: `%s': not a valid identifier\n",
		builtin, argument);
}

static int	sh_builtin_getcwd_to_env(t_shell *shell, const char *key)
{
	char	buffer[PATH_MAX];

	if (getcwd(buffer, sizeof(buffer)) == NULL)
		return (0);
	sh_env_store_set(&shell->env, key, buffer, 1);
	return (1);
}

static void	sh_builtin_update_pwd(t_shell *shell)
{
	const char	*oldpwd;

	oldpwd = sh_env_store_get(&shell->env, "PWD");
	if (oldpwd != NULL)
		sh_env_store_set(&shell->env, "OLDPWD", oldpwd, 1);
	sh_builtin_getcwd_to_env(shell, "PWD");
}

static int	sh_builtin_export_assignment(t_shell *shell, const char *argument)
{
	size_t	key_len;
	char	*key;

	key_len = sh_identifier_key_len(argument);
	key = sh_xmalloc(key_len + 1);
	memcpy(key, argument, key_len);
	key[key_len] = '\0';
	sh_env_store_set(&shell->env, key, argument + key_len + 1, 1);
	free(key);
	return (0);
}

static int	sh_builtin_exit_status(const char *text, int *status)
{
	char	*endptr;
	long	value;

	endptr = NULL;
	value = strtol(text, &endptr, 10);
	if (text[0] == '\0' || endptr == NULL || *endptr != '\0')
		return (0);
	*status = (unsigned char)value;
	return (1);
}

int	sh_builtin_cd(t_shell *shell, int argc, char **argv)
{
	const char	*path;

	if (argc > 2)
	{
		sh_error("cd", "too many arguments");
		return (1);
	}
	if (argc == 1)
	{
		path = sh_env_store_get(&shell->env, "HOME");
		if (path == NULL)
		{
			sh_error("cd", "HOME not set");
			return (1);
		}
	}
	else
		path = argv[1];
	if (chdir(path) < 0)
	{
		sh_perror(path, NULL);
		return (1);
	}
	sh_builtin_update_pwd(shell);
	return (0);
}

int	sh_builtin_export(t_shell *shell, int argc, char **argv)
{
	char	**lines;
	int		index;
	int		status;

	if (argc == 1)
	{
		lines = sh_env_store_format_export(&shell->env);
		index = 0;
		while (lines[index] != NULL)
		{
			printf("%s\n", lines[index]);
			index++;
		}
		sh_builtin_free_lines(lines);
		return (0);
	}
	status = 0;
	index = 1;
	while (index < argc)
	{
		if (sh_is_assignment(argv[index]))
			status |= sh_builtin_export_assignment(shell, argv[index]);
		else if (sh_is_identifier(argv[index]))
			sh_env_store_set(&shell->env, argv[index], NULL, 0);
		else
		{
			sh_builtin_invalid_identifier("export", argv[index]);
			status = 1;
		}
		index++;
	}
	return (status);
}

int	sh_builtin_unset(t_shell *shell, int argc, char **argv)
{
	int	index;
	int	status;

	status = 0;
	index = 1;
	while (index < argc)
	{
		if (!sh_is_identifier(argv[index]))
		{
			sh_builtin_invalid_identifier("unset", argv[index]);
			status = 1;
		}
		else
			sh_env_store_unset(&shell->env, argv[index]);
		index++;
	}
	return (status);
}

int	sh_builtin_exit(t_shell *shell, int argc, char **argv)
{
	int	status;

	status = shell->last_status;
	if (argc > 2)
	{
		if (!sh_builtin_exit_status(argv[1], &status))
		{
			fprintf(stderr, "shell: exit: %s: numeric argument required\n",
				argv[1]);
			shell->should_exit = 1;
			return (2);
		}
		sh_error("exit", "too many arguments");
		return (1);
	}
	if (argc == 2)
	{
		if (!sh_builtin_exit_status(argv[1], &status))
		{
			fprintf(stderr, "shell: exit: %s: numeric argument required\n",
				argv[1]);
			shell->should_exit = 1;
			return (2);
		}
	}
	shell->should_exit = 1;
	return (status);
}
