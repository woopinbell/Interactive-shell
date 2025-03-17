#ifndef SHELL_AST_H
#define SHELL_AST_H

#include "shell/lexer.h"

#include <stddef.h>

typedef struct s_command_word
{
	char				*text;
	t_word_part_list	parts;
	t_lexer_quote_state	quote_state;
}	t_command_word;

typedef enum e_redirection_kind
{
	SH_REDIRECTION_NONE,
	SH_REDIRECTION_INPUT,
	SH_REDIRECTION_OUTPUT,
	SH_REDIRECTION_APPEND,
	SH_REDIRECTION_HEREDOC
}	t_redirection_kind;

typedef struct s_heredoc_placeholder
{
	int		should_expand;
	int		fd;
	char	*path;
}	t_heredoc_placeholder;

typedef struct s_redirection_node
{
	t_redirection_kind			kind;
	t_command_word				operand;
	t_heredoc_placeholder		heredoc;
	struct s_redirection_node	*next;
}	t_redirection_node;

typedef struct s_simple_command
{
	t_command_word		*argv;
	size_t				argc;
	size_t				argv_capacity;
	t_redirection_node	*redirections;
}	t_simple_command;

void	sh_command_word_init(t_command_word *word);
void	sh_command_word_destroy(t_command_word *word);
void	sh_heredoc_placeholder_init(t_heredoc_placeholder *placeholder);
void	sh_heredoc_placeholder_destroy(t_heredoc_placeholder *placeholder);
void	sh_redirection_node_init(t_redirection_node *node);
void	sh_redirection_node_destroy(t_redirection_node *node);
void	sh_simple_command_init(t_simple_command *command);
void	sh_simple_command_destroy(t_simple_command *command);

#endif
