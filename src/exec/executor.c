#include "shell/exec.h"

#include "shell/prepare.h"
#include "shell/support/alloc.h"
#include "shell/support/error.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void	sh_executor_free_vector(char **values)
{
	size_t	index;

	if (values == NULL)
		return ;
	index = 0;
	while (values[index] != NULL)
	{
		free(values[index]);
		index++;
	}
	free(values);
}

static char	**sh_executor_command_argv(const t_simple_command *command)
{
	char	**argv;
	size_t	index;

	argv = sh_xcalloc(command->argc + 1, sizeof(char *));
	index = 0;
	while (index < command->argc)
	{
		argv[index] = sh_xstrdup(command->argv[index].text);
		index++;
	}
	return (argv);
}

static void	sh_executor_child_exec(t_shell *shell, char **argv, char **envp)
{
	char	*resolved_path;
	int		exit_status;

	resolved_path = sh_executor_resolve_command_path(shell, argv[0]);
	if (resolved_path == NULL)
	{
		sh_error(argv[0], "command not found");
		sh_executor_free_vector(envp);
		sh_executor_free_vector(argv);
		_exit(127);
	}
	execve(resolved_path, argv, envp);
	exit_status = sh_executor_status_from_system_error(errno);
	sh_perror(argv[0], NULL);
	free(resolved_path);
	sh_executor_free_vector(envp);
	sh_executor_free_vector(argv);
	_exit(exit_status);
}

static int	sh_executor_wait_child(pid_t child_pid)
{
	int	wait_status;

	wait_status = 0;
	if (waitpid(child_pid, &wait_status, 0) < 0)
		return (sh_executor_status_from_system_error(errno));
	return (sh_executor_status_from_waitpid(wait_status));
}

static int	sh_executor_run_external_command(t_shell *shell,
		t_simple_command *command)
{
	pid_t	child_pid;
	char	**argv;
	char	**envp;
	int		status;

	if (command->argc == 0)
		return (0);
	argv = sh_executor_command_argv(command);
	envp = sh_env_store_to_envp(&shell->env);
	child_pid = fork();
	if (child_pid < 0)
	{
		status = sh_executor_status_from_system_error(errno);
		sh_perror(command->argv[0].text, "fork");
		sh_executor_free_vector(envp);
		sh_executor_free_vector(argv);
		return (status);
	}
	if (child_pid == 0)
		sh_executor_child_exec(shell, argv, envp);
	sh_executor_free_vector(envp);
	sh_executor_free_vector(argv);
	return (sh_executor_wait_child(child_pid));
}

static int	sh_executor_run_pipeline(t_shell *shell, t_pipeline *pipeline)
{
	if (pipeline->size != 1)
	{
		sh_error("pipeline", "not supported yet");
		return (1);
	}
	return (sh_executor_run_external_command(shell, &pipeline->head->command));
}

static int	sh_executor_run_and_or_list(t_shell *shell, t_and_or_list *list)
{
	if (list->size != 1)
	{
		sh_error("and/or", "not supported yet");
		return (1);
	}
	return (sh_executor_run_pipeline(shell, &list->head->pipeline));
}

static int	sh_executor_run_sequence_list(t_shell *shell,
		t_sequence_list *program)
{
	t_sequence_list_node	*node;
	int						status;

	status = 0;
	node = program->head;
	while (node != NULL)
	{
		status = sh_executor_run_and_or_list(shell, &node->and_or);
		node = node->next;
	}
	return (status);
}

int	sh_executor_status_from_waitpid(int wait_status)
{
	if (WIFEXITED(wait_status))
		return (WEXITSTATUS(wait_status));
	if (WIFSIGNALED(wait_status))
		return (128 + WTERMSIG(wait_status));
	if (WIFSTOPPED(wait_status))
		return (128 + WSTOPSIG(wait_status));
	return (1);
}

int	sh_executor_status_from_system_error(int error_number)
{
	if (error_number == ENOENT)
		return (127);
	if (error_number == EACCES || error_number == EPERM
		|| error_number == EISDIR || error_number == ENOEXEC)
		return (126);
	return (1);
}

int	sh_executor_run(t_shell *shell, t_sequence_list *program)
{
	t_shell_signal_phase	previous_phase;
	int						status;

	previous_phase = shell->signal_phase;
	shell->signal_phase = SH_SIGNAL_PHASE_EXECUTE;
	if (!sh_prepare_sequence_words(shell, program))
	{
		shell->signal_phase = previous_phase;
		return (shell->last_status);
	}
	if (!sh_prepare_sequence_heredocs(shell, program))
	{
		shell->signal_phase = previous_phase;
		return (shell->last_status);
	}
	status = sh_executor_run_sequence_list(shell, program);
	shell->signal_phase = previous_phase;
	shell->last_status = status;
	return (shell->last_status);
}
