#include "shell/builtin.h"
#include "shell/exec.h"

#include "shell/prepare.h"
#include "shell/signal.h"
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

static void	sh_executor_child_run_command(t_shell *shell,
		t_simple_command *command)
{
	t_builtin_fn	run;
	char			**argv;
	char			**envp;
	char			*resolved_path;
	int				exit_status;

	if (command->argc == 0)
		_exit(0);
	argv = sh_executor_command_argv(command);
	run = sh_builtin_find(argv[0]);
	if (run != NULL)
	{
		exit_status = run(shell, command->argc, argv);
		fflush(NULL);
		sh_executor_free_vector(argv);
		_exit(exit_status);
	}
	envp = sh_env_store_to_envp(&shell->env);
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

static void	sh_executor_child_run_stage(t_shell *shell, t_simple_command *command,
		int input_fd, int output_fd)
{
	int	exit_status;

	if (input_fd != STDIN_FILENO && dup2(input_fd, STDIN_FILENO) < 0)
		_exit(sh_executor_status_from_system_error(errno));
	if (output_fd != STDOUT_FILENO && dup2(output_fd, STDOUT_FILENO) < 0)
		_exit(sh_executor_status_from_system_error(errno));
	if (input_fd != STDIN_FILENO)
		close(input_fd);
	if (output_fd != STDOUT_FILENO)
		close(output_fd);
	exit_status = sh_executor_apply_command_redirections(command, NULL);
	if (exit_status != 0)
		_exit(exit_status);
	sh_executor_child_run_command(shell, command);
}

static int	sh_executor_wait_child(pid_t child_pid)
{
	int	wait_status;

	wait_status = 0;
	if (waitpid(child_pid, &wait_status, 0) < 0)
		return (sh_executor_status_from_system_error(errno));
	return (sh_executor_status_from_waitpid(wait_status));
}

static int	sh_executor_wait_child_with_signals(t_shell *shell, pid_t child_pid)
{
	t_shell_signal_phase	previous_phase;
	int						status;

	previous_phase = shell->signal_phase;
	shell->signal_phase = SH_SIGNAL_PHASE_PIPELINE_WAIT;
	sh_signal_wait_enter(shell);
	status = sh_executor_wait_child(child_pid);
	sh_signal_wait_leave(shell);
	shell->signal_phase = previous_phase;
	return (status);
}

static int	sh_executor_run_external_command(t_shell *shell,
		t_simple_command *command, char **argv)
{
	pid_t	child_pid;
	int		status;

	child_pid = fork();
	if (child_pid < 0)
	{
		status = sh_executor_status_from_system_error(errno);
		sh_perror(argv[0], "fork");
		return (status);
	}
	if (child_pid == 0)
	{
		sh_signal_child_default();
		sh_executor_child_run_stage(shell, command, STDIN_FILENO, STDOUT_FILENO);
	}
	return (sh_executor_wait_child_with_signals(shell, child_pid));
}

static int	sh_executor_run_simple_command(t_shell *shell,
		t_simple_command *command, int allow_parent_builtin)
{
	char	**argv;
	int		handled;
	int		status;

	if (command->argc == 0)
		return (sh_executor_apply_empty_command(command));
	argv = sh_executor_command_argv(command);
	handled = 0;
	status = 0;
	if (allow_parent_builtin)
		status = sh_executor_run_builtin_in_parent(shell, command, argv, &handled);
	if (!handled)
		status = sh_executor_run_external_command(shell, command, argv);
	sh_executor_free_vector(argv);
	return (status);
}

static int	sh_executor_wait_pipeline_children(pid_t *pids, size_t count)
{
	int		wait_status;
	int		last_status;
	size_t	index;

	last_status = 0;
	index = 0;
	while (index < count)
	{
		wait_status = 0;
		if (waitpid(pids[index], &wait_status, 0) < 0)
			last_status = sh_executor_status_from_system_error(errno);
		else if (index == count - 1)
			last_status = sh_executor_status_from_waitpid(wait_status);
		index++;
	}
	return (last_status);
}

static int	sh_executor_wait_pipeline_children_with_signals(t_shell *shell,
		pid_t *pids, size_t count)
{
	t_shell_signal_phase	previous_phase;
	int						status;

	previous_phase = shell->signal_phase;
	shell->signal_phase = SH_SIGNAL_PHASE_PIPELINE_WAIT;
	sh_signal_wait_enter(shell);
	status = sh_executor_wait_pipeline_children(pids, count);
	sh_signal_wait_leave(shell);
	shell->signal_phase = previous_phase;
	return (status);
}

static pid_t	sh_executor_spawn_pipeline_stage(t_shell *shell,
		t_simple_command *command, int input_fd, int output_fd)
{
	pid_t	child_pid;

	child_pid = fork();
	if (child_pid < 0)
		return (-1);
	if (child_pid == 0)
	{
		sh_signal_child_default();
		sh_executor_child_run_stage(shell, command, input_fd, output_fd);
	}
	return (child_pid);
}

static int	sh_executor_run_pipeline_stages(t_shell *shell, t_pipeline *pipeline)
{
	t_pipeline_command_node	*node;
	pid_t					*pids;
	size_t					index;
	int						pipefd[2];
	int						input_fd;

	pids = sh_xcalloc(pipeline->size, sizeof(pid_t));
	node = pipeline->head;
	index = 0;
	input_fd = STDIN_FILENO;
	while (node != NULL)
	{
		pipefd[0] = -1;
		pipefd[1] = -1;
		if (node->next != NULL && pipe(pipefd) < 0)
			break ;
		pids[index] = sh_executor_spawn_pipeline_stage(shell, &node->command,
				input_fd, node->next != NULL ? pipefd[1] : STDOUT_FILENO);
		if (input_fd != STDIN_FILENO)
			close(input_fd);
		if (pipefd[1] >= 0)
			close(pipefd[1]);
		if (pids[index] < 0)
		{
			if (pipefd[0] >= 0)
				close(pipefd[0]);
			break ;
		}
		input_fd = pipefd[0];
		node = node->next;
		index++;
	}
	if (input_fd != STDIN_FILENO && node == NULL)
		close(input_fd);
	if (node != NULL)
	{
		if (input_fd != STDIN_FILENO)
			close(input_fd);
		if (index > 0)
			sh_executor_wait_pipeline_children_with_signals(shell, pids, index);
		free(pids);
		return (sh_executor_status_from_system_error(errno));
	}
	input_fd = sh_executor_wait_pipeline_children_with_signals(shell,
			pids, index);
	free(pids);
	return (input_fd);
}

static int	sh_executor_run_pipeline(t_shell *shell, t_pipeline *pipeline)
{
	if (pipeline->size == 1)
		return (sh_executor_run_simple_command(shell,
				&pipeline->head->command, 1));
	return (sh_executor_run_pipeline_stages(shell, pipeline));
}

static int	sh_executor_should_run_and_or(t_and_or_operator operator, int status)
{
	if (operator == SH_AND_OR_OPERATOR_AND_IF)
		return (status == 0);
	if (operator == SH_AND_OR_OPERATOR_OR_IF)
		return (status != 0);
	return (1);
}

static int	sh_executor_run_and_or_list(t_shell *shell, t_and_or_list *list)
{
	t_and_or_list_node	*node;
	t_and_or_list_node	*next;
	int					status;

	if (list->head == NULL)
		return (0);
	status = sh_executor_run_pipeline(shell, &list->head->pipeline);
	if (shell->should_exit)
		return (status);
	node = list->head;
	while (node->next != NULL)
	{
		next = node->next;
		if (!sh_executor_should_run_and_or(node->next_operator, status))
		{
			node = next;
			continue ;
		}
		status = sh_executor_run_pipeline(shell, &next->pipeline);
		if (shell->should_exit)
			return (status);
		node = next;
	}
	return (status);
}

static int	sh_executor_should_continue_sequence(
		const t_sequence_list_node *node)
{
	return (node->next != NULL
		&& node->next_separator == SH_SEQUENCE_SEPARATOR_SEMICOLON);
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
		if (shell->should_exit)
			break ;
		if (!sh_executor_should_continue_sequence(node))
			break ;
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
	if (program->head == NULL)
	{
		shell->signal_phase = previous_phase;
		return (shell->last_status);
	}
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
