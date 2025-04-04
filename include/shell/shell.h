#ifndef SHELL_SHELL_H
#define SHELL_SHELL_H

#include "shell/env.h"
#include "shell/input.h"

typedef enum e_shell_signal_phase
{
	SH_SIGNAL_PHASE_INIT,
	SH_SIGNAL_PHASE_PROMPT,
	SH_SIGNAL_PHASE_EXECUTE,
	SH_SIGNAL_PHASE_HEREDOC,
	SH_SIGNAL_PHASE_PIPELINE_WAIT
}	t_shell_signal_phase;

typedef struct s_shell
{
	t_env_store				env;
	int						last_status;
	int						is_interactive;
	int						should_exit;
	t_input_adapter			input;
	t_shell_signal_phase	signal_phase;
}	t_shell;

void	sh_shell_init(t_shell *shell, char **envp);
void	sh_shell_destroy(t_shell *shell);

#endif
