#include "shell/builtin.h"

#include "shell/support/error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int	sh_builtin_is_n_option(const char *argument)
{
	size_t	index;

	if (argument == NULL || argument[0] != '-' || argument[1] == '\0')
		return (0);
	index = 1;
	while (argument[index] == 'n')
		index++;
	return (argument[index] == '\0');
}

static void	sh_builtin_free_envp(char **envp)
{
	size_t	index;

	index = 0;
	while (envp != NULL && envp[index] != NULL)
	{
		free(envp[index]);
		index++;
	}
	free(envp);
}

int	sh_builtin_echo(t_shell *shell, int argc, char **argv)
{
	int	index;
	int	first_argument;
	int	print_newline;

	(void)shell;
	index = 1;
	first_argument = 1;
	print_newline = 1;
	while (index < argc && sh_builtin_is_n_option(argv[index]))
	{
		print_newline = 0;
		index++;
	}
	while (index < argc)
	{
		if (!first_argument)
			printf(" ");
		printf("%s", argv[index]);
		first_argument = 0;
		index++;
	}
	if (print_newline)
		printf("\n");
	return (0);
}

int	sh_builtin_pwd(t_shell *shell, int argc, char **argv)
{
	char	buffer[4096];

	(void)shell;
	(void)argc;
	(void)argv;
	if (getcwd(buffer, sizeof(buffer)) == NULL)
	{
		sh_perror("pwd", NULL);
		return (1);
	}
	printf("%s\n", buffer);
	return (0);
}

int	sh_builtin_env(t_shell *shell, int argc, char **argv)
{
	char	**envp;
	size_t	index;

	(void)argc;
	(void)argv;
	envp = sh_env_store_to_envp(&shell->env);
	index = 0;
	while (envp[index] != NULL)
	{
		printf("%s\n", envp[index]);
		index++;
	}
	sh_builtin_free_envp(envp);
	return (0);
}

int	sh_builtin_true(t_shell *shell, int argc, char **argv)
{
	(void)shell;
	(void)argc;
	(void)argv;
	return (0);
}

int	sh_builtin_false(t_shell *shell, int argc, char **argv)
{
	(void)shell;
	(void)argc;
	(void)argv;
	return (1);
}
