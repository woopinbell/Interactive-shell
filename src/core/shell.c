#include "shell/shell.h"

#include <stdlib.h>
#include <unistd.h>

#include <readline/history.h>

static int	sh_shell_detect_interactive(void)
{
	return (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO));
}

static const char	*sh_shell_prompt(const t_shell *shell)
{
	if (shell->is_interactive)
		return ("shell$ ");
	return ("");
}

void	sh_shell_init(t_shell *shell, char **envp)
{
	sh_env_store_init_from_envp(&shell->env, envp);
	shell->last_status = 0;
	shell->is_interactive = sh_shell_detect_interactive();
	sh_input_adapter_init(&shell->input);
	if (shell->is_interactive)
		sh_input_adapter_use_readline(&shell->input);
	else
		sh_input_adapter_use_stream(&shell->input);
	shell->signal_phase = SH_SIGNAL_PHASE_INIT;
}

int	sh_shell_run(t_shell *shell)
{
	char	*line;

	shell->signal_phase = SH_SIGNAL_PHASE_PROMPT;
	line = sh_input_adapter_read_line(&shell->input, sh_shell_prompt(shell));
	while (line != NULL)
	{
		if (shell->is_interactive && line[0] != '\0')
			add_history(line);
		free(line);
		shell->signal_phase = SH_SIGNAL_PHASE_PROMPT;
		line = sh_input_adapter_read_line(&shell->input,
				sh_shell_prompt(shell));
	}
	shell->signal_phase = SH_SIGNAL_PHASE_INIT;
	if (shell->is_interactive)
		write(STDOUT_FILENO, "exit\n", 5);
	return (shell->last_status);
}

void	sh_shell_destroy(t_shell *shell)
{
	sh_input_adapter_destroy(&shell->input);
	sh_env_store_destroy(&shell->env);
	shell->last_status = 0;
	shell->is_interactive = 0;
	shell->signal_phase = SH_SIGNAL_PHASE_INIT;
}
