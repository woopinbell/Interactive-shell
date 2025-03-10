#include "shell/shell.h"

#include <unistd.h>

static int	sh_shell_detect_interactive(void)
{
	return (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO));
}

void	sh_shell_init(t_shell *shell, char **envp)
{
	sh_env_store_init_from_envp(&shell->env, envp);
	shell->last_status = 0;
	shell->is_interactive = sh_shell_detect_interactive();
	sh_input_adapter_init(&shell->input);
	sh_input_adapter_set(&shell->input, SH_INPUT_ADAPTER_STDIO,
		NULL, NULL, NULL);
	shell->signal_phase = SH_SIGNAL_PHASE_INIT;
}

void	sh_shell_destroy(t_shell *shell)
{
	sh_input_adapter_destroy(&shell->input);
	sh_env_store_destroy(&shell->env);
	shell->last_status = 0;
	shell->is_interactive = 0;
	shell->signal_phase = SH_SIGNAL_PHASE_INIT;
}
