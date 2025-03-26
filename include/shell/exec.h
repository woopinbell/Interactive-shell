#ifndef SHELL_EXEC_H
#define SHELL_EXEC_H

#include "shell/ast.h"
#include "shell/shell.h"

int		sh_executor_command_has_path(const char *command_name);
char	*sh_executor_resolve_direct_path(const char *command_name);
char	*sh_executor_resolve_path_search(t_shell *shell,
			const char *command_name);
char	*sh_executor_resolve_command_path(t_shell *shell,
			const char *command_name);
int	sh_executor_status_from_waitpid(int wait_status);
int	sh_executor_status_from_system_error(int error_number);
int	sh_executor_run(t_shell *shell, t_sequence_list *program);

#endif
