#include "shell/builtin.h"
#include "shell/exec.h"

#include "shell/prepare.h"
#include "shell/support/alloc.h"
#include "shell/support/error.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct s_executor_saved_fds
{
	int	stdin_copy;
	int	stdout_copy;
}	t_executor_saved_fds;

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

static void	sh_executor_saved_fds_init(t_executor_saved_fds *saved)
{
	saved->stdin_copy = -1;
	saved->stdout_copy = -1;
}

static void	sh_executor_saved_fds_close(t_executor_saved_fds *saved)
{
	if (saved->stdin_copy >= 0)
		close(saved->stdin_copy);
	if (saved->stdout_copy >= 0)
		close(saved->stdout_copy);
	sh_executor_saved_fds_init(saved);
}

static int	sh_executor_redirection_target_fd(const t_redirection_node *node)
{
	if (node->kind == SH_REDIRECTION_INPUT || node->kind == SH_REDIRECTION_HEREDOC)
		return (STDIN_FILENO);
	return (STDOUT_FILENO);
}

static int	sh_executor_save_target_fd(int target_fd,
		t_executor_saved_fds *saved)
{
	if (saved == NULL)
		return (0);
	if (target_fd == STDIN_FILENO && saved->stdin_copy < 0)
	{
		saved->stdin_copy = dup(STDIN_FILENO);
		if (saved->stdin_copy < 0)
			return (sh_executor_status_from_system_error(errno));
	}
	if (target_fd == STDOUT_FILENO && saved->stdout_copy < 0)
	{
		saved->stdout_copy = dup(STDOUT_FILENO);
		if (saved->stdout_copy < 0)
			return (sh_executor_status_from_system_error(errno));
	}
	return (0);
}

static int	sh_executor_open_redirection_fd(const t_redirection_node *node)
{
	if (node->kind == SH_REDIRECTION_INPUT)
		return (open(node->operand.text, O_RDONLY));
	if (node->kind == SH_REDIRECTION_OUTPUT)
		return (open(node->operand.text, O_WRONLY | O_CREAT | O_TRUNC, 0644));
	if (node->kind == SH_REDIRECTION_APPEND)
		return (open(node->operand.text, O_WRONLY | O_CREAT | O_APPEND, 0644));
	if (node->kind == SH_REDIRECTION_HEREDOC)
		return (node->heredoc.fd);
	return (-1);
}

static int	sh_executor_apply_single_redirection(t_redirection_node *node,
		t_executor_saved_fds *saved)
{
	int	target_fd;
	int	source_fd;
	int	status;

	target_fd = sh_executor_redirection_target_fd(node);
	status = sh_executor_save_target_fd(target_fd, saved);
	if (status != 0)
		return (status);
	source_fd = sh_executor_open_redirection_fd(node);
	if (source_fd < 0)
	{
		if (node->kind == SH_REDIRECTION_HEREDOC)
			sh_error("heredoc", "not prepared");
		else
			sh_perror(node->operand.text, NULL);
		return (sh_executor_status_from_system_error(errno));
	}
	if (dup2(source_fd, target_fd) < 0)
	{
		if (node->kind != SH_REDIRECTION_HEREDOC)
			close(source_fd);
		sh_perror(node->operand.text, NULL);
		return (sh_executor_status_from_system_error(errno));
	}
	if (node->kind != SH_REDIRECTION_HEREDOC)
		close(source_fd);
	return (0);
}

static int	sh_executor_apply_command_redirections(t_simple_command *command,
		t_executor_saved_fds *saved)
{
	t_redirection_node	*node;
	int					status;

	node = command->redirections;
	while (node != NULL)
	{
		status = sh_executor_apply_single_redirection(node, saved);
		if (status != 0)
			return (status);
		node = node->next;
	}
	return (0);
}

static int	sh_executor_restore_saved_fds(t_executor_saved_fds *saved)
{
	if (saved->stdin_copy >= 0 && dup2(saved->stdin_copy, STDIN_FILENO) < 0)
		return (sh_executor_status_from_system_error(errno));
	if (saved->stdout_copy >= 0 && dup2(saved->stdout_copy, STDOUT_FILENO) < 0)
		return (sh_executor_status_from_system_error(errno));
	return (0);
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

static int	sh_executor_run_builtin_in_parent(t_shell *shell,
		t_simple_command *command, char **argv, int *handled)
{
	t_executor_saved_fds	saved;
	t_builtin_fn				run;
	int						status;

	run = sh_builtin_find(argv[0]);
	if (handled != NULL)
		*handled = (run != NULL);
	if (run == NULL)
		return (0);
	sh_executor_saved_fds_init(&saved);
	fflush(NULL);
	status = sh_executor_apply_command_redirections(command, &saved);
	if (status == 0)
		status = run(shell, command->argc, argv);
	fflush(NULL);
	if (sh_executor_restore_saved_fds(&saved) != 0 && status == 0)
		status = sh_executor_status_from_system_error(errno);
	sh_executor_saved_fds_close(&saved);
	return (status);
}

static int	sh_executor_apply_empty_command(t_simple_command *command)
{
	t_executor_saved_fds	saved;
	int						status;

	sh_executor_saved_fds_init(&saved);
	fflush(NULL);
	status = sh_executor_apply_command_redirections(command, &saved);
	fflush(NULL);
	if (sh_executor_restore_saved_fds(&saved) != 0 && status == 0)
		status = sh_executor_status_from_system_error(errno);
	sh_executor_saved_fds_close(&saved);
	return (status);
}

static void	sh_executor_child_exec(t_shell *shell, t_simple_command *command,
		char **argv, char **envp)
{
	char	*resolved_path;
	int		exit_status;

	exit_status = sh_executor_apply_command_redirections(command, NULL);
	if (exit_status != 0)
	{
		sh_executor_free_vector(envp);
		sh_executor_free_vector(argv);
		_exit(exit_status);
	}
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
		t_simple_command *command, char **argv)
{
	pid_t	child_pid;
	char	**envp;
	int		status;

	envp = sh_env_store_to_envp(&shell->env);
	child_pid = fork();
	if (child_pid < 0)
	{
		status = sh_executor_status_from_system_error(errno);
		sh_perror(argv[0], "fork");
		sh_executor_free_vector(envp);
		return (status);
	}
	if (child_pid == 0)
		sh_executor_child_exec(shell, command, argv, envp);
	sh_executor_free_vector(envp);
	return (sh_executor_wait_child(child_pid));
}

static int	sh_executor_run_simple_command(t_shell *shell,
		t_simple_command *command)
{
	char	**argv;
	int		handled;
	int		status;

	if (command->argc == 0)
		return (sh_executor_apply_empty_command(command));
	argv = sh_executor_command_argv(command);
	status = sh_executor_run_builtin_in_parent(shell, command, argv, &handled);
	if (!handled)
		status = sh_executor_run_external_command(shell, command, argv);
	sh_executor_free_vector(argv);
	return (status);
}

static int	sh_executor_run_pipeline(t_shell *shell, t_pipeline *pipeline)
{
	if (pipeline->size != 1)
	{
		sh_error("pipeline", "not supported yet");
		return (1);
	}
	return (sh_executor_run_simple_command(shell, &pipeline->head->command));
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
