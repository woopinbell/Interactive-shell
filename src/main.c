#include "shell/shell.h"

int	main(int argc, char **argv, char **envp)
{
	t_shell	shell;
	int		status;

	(void)argc;
	(void)argv;
	sh_shell_init(&shell, envp);
	status = sh_shell_run(&shell);
	sh_shell_destroy(&shell);
	return (status);
}
