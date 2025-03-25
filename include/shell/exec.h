#ifndef SHELL_EXEC_H
#define SHELL_EXEC_H

#include "shell/ast.h"
#include "shell/shell.h"

int	sh_executor_status_from_waitpid(int wait_status);
int	sh_executor_status_from_system_error(int error_number);
int	sh_executor_run(t_shell *shell, t_sequence_list *program);

#endif
