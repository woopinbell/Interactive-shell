#include "shell/exec.h"

#include "shell/prepare.h"

#include <errno.h>
#include <sys/wait.h>

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
	shell->signal_phase = previous_phase;
	shell->last_status = 0;
	return (shell->last_status);
}
