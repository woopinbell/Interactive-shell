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
	char	*delimiter;
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

typedef struct s_pipeline_command_node
{
	t_simple_command					command;
	struct s_pipeline_command_node		*next;
}	t_pipeline_command_node;

typedef struct s_pipeline
{
	t_pipeline_command_node	*head;
	t_pipeline_command_node	*tail;
	size_t					size;
}	t_pipeline;

typedef enum e_and_or_operator
{
	SH_AND_OR_OPERATOR_NONE,
	SH_AND_OR_OPERATOR_AND_IF,
	SH_AND_OR_OPERATOR_OR_IF
}	t_and_or_operator;

typedef struct s_and_or_list_node
{
	t_pipeline					pipeline;
	t_and_or_operator			next_operator;
	struct s_and_or_list_node	*next;
}	t_and_or_list_node;

typedef struct s_and_or_list
{
	t_and_or_list_node	*head;
	t_and_or_list_node	*tail;
	size_t				size;
}	t_and_or_list;

typedef enum e_sequence_separator
{
	SH_SEQUENCE_SEPARATOR_NONE,
	SH_SEQUENCE_SEPARATOR_SEMICOLON
}	t_sequence_separator;

typedef struct s_sequence_list_node
{
	t_and_or_list				and_or;
	t_sequence_separator		next_separator;
	struct s_sequence_list_node	*next;
}	t_sequence_list_node;

typedef struct s_sequence_list
{
	t_sequence_list_node	*head;
	t_sequence_list_node	*tail;
	size_t					size;
}	t_sequence_list;

void	sh_command_word_init(t_command_word *word);
void	sh_command_word_destroy(t_command_word *word);
void	sh_heredoc_placeholder_init(t_heredoc_placeholder *placeholder);
void	sh_heredoc_placeholder_destroy(t_heredoc_placeholder *placeholder);
void	sh_redirection_node_init(t_redirection_node *node);
void	sh_redirection_node_destroy(t_redirection_node *node);
void	sh_simple_command_init(t_simple_command *command);
void	sh_simple_command_destroy(t_simple_command *command);
void	sh_pipeline_command_node_init(t_pipeline_command_node *node);
void	sh_pipeline_command_node_destroy(t_pipeline_command_node *node);
void	sh_pipeline_init(t_pipeline *pipeline);
void	sh_pipeline_destroy(t_pipeline *pipeline);
void	sh_and_or_list_node_init(t_and_or_list_node *node);
void	sh_and_or_list_node_destroy(t_and_or_list_node *node);
void	sh_and_or_list_init(t_and_or_list *list);
void	sh_and_or_list_destroy(t_and_or_list *list);
void	sh_sequence_list_node_init(t_sequence_list_node *node);
void	sh_sequence_list_node_destroy(t_sequence_list_node *node);
void	sh_sequence_list_init(t_sequence_list *list);
void	sh_sequence_list_destroy(t_sequence_list *list);

#endif
