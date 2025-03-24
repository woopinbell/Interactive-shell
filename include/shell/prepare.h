#ifndef SHELL_PREPARE_H
#define SHELL_PREPARE_H

#include "shell/ast.h"
#include "shell/shell.h"

char	*sh_expand_text(t_shell *shell, const char *text);
char	*sh_prepare_word_text(t_shell *shell, const t_command_word *word);
int		sh_prepare_command_word(t_shell *shell, t_command_word *word);
int		sh_prepare_simple_command_words(t_shell *shell,
			t_simple_command *command);
int		sh_prepare_pipeline_words(t_shell *shell, t_pipeline *pipeline);
int		sh_prepare_and_or_list_words(t_shell *shell, t_and_or_list *list);
int		sh_prepare_sequence_words(t_shell *shell, t_sequence_list *list);
int		sh_prepare_simple_command_heredocs(t_shell *shell,
			t_simple_command *command);
int		sh_prepare_pipeline_heredocs(t_shell *shell, t_pipeline *pipeline);
int		sh_prepare_and_or_list_heredocs(t_shell *shell, t_and_or_list *list);
int		sh_prepare_sequence_heredocs(t_shell *shell, t_sequence_list *list);

#endif
