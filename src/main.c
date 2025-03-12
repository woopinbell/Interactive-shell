#include "shell/shell.h"

#include <stdlib.h>
#include <unistd.h>

#include <readline/history.h>

static const char	*sh_main_prompt(const t_shell *shell)
{
	if (shell->is_interactive)
		return ("shell$ ");
	return ("");
}

static void	sh_main_select_input_adapter(t_shell *shell)
{
	if (shell->is_interactive)
		sh_input_adapter_use_readline(&shell->input);
	else
		sh_input_adapter_use_stream(&shell->input);
}

static int	sh_main_repl(t_shell *shell)
{
	char	*line;

	shell->signal_phase = SH_SIGNAL_PHASE_PROMPT;
	line = sh_input_adapter_read_line(&shell->input, sh_main_prompt(shell));
	while (line != NULL)
	{
		if (shell->is_interactive && line[0] != '\0')
			add_history(line);
		free(line);
		shell->signal_phase = SH_SIGNAL_PHASE_PROMPT;
		line = sh_input_adapter_read_line(&shell->input,
				sh_main_prompt(shell));
	}
	shell->signal_phase = SH_SIGNAL_PHASE_INIT;
	if (shell->is_interactive)
		write(STDOUT_FILENO, "exit\n", 5);
	return (shell->last_status);
}

int	main(int argc, char **argv, char **envp)
{
	t_shell	shell;
	int		status;

	(void)argc;
	(void)argv;
	sh_shell_init(&shell, envp);
	sh_main_select_input_adapter(&shell);
	status = sh_main_repl(&shell);
	sh_shell_destroy(&shell);
	return (status);
}
