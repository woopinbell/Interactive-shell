#ifndef SHELL_SIGNAL_H
#define SHELL_SIGNAL_H

#include "shell/shell.h"

void	sh_signal_prompt_enter(t_shell *shell);
void	sh_signal_prompt_leave(t_shell *shell);
void	sh_signal_heredoc_enter(t_shell *shell);
void	sh_signal_heredoc_leave(t_shell *shell);
void	sh_signal_input_refresh(void);
int		sh_signal_prompt_interrupted(t_shell *shell);
int		sh_signal_heredoc_interrupted(t_shell *shell);
void	sh_signal_wait_enter(t_shell *shell);
void	sh_signal_wait_leave(t_shell *shell);
void	sh_signal_child_default(void);

#endif
