#include "shell/ast.h"

#include <stdlib.h>

static t_lexer_quote_state	sh_ast_plain_quote_state(void)
{
	t_lexer_quote_state	state;

	state.kind = SH_QUOTE_STATE_NONE;
	state.is_closed = 1;
	return (state);
}

void	sh_command_word_init(t_command_word *word)
{
	word->text = NULL;
	word->parts.items = NULL;
	word->parts.len = 0;
	word->parts.capacity = 0;
	word->quote_state = sh_ast_plain_quote_state();
}

void	sh_command_word_destroy(t_command_word *word)
{
	size_t	index;

	if (word == NULL)
		return ;
	free(word->text);
	word->text = NULL;
	index = 0;
	while (index < word->parts.len)
	{
		sh_word_part_destroy(&word->parts.items[index]);
		index++;
	}
	free(word->parts.items);
	word->parts.items = NULL;
	word->parts.len = 0;
	word->parts.capacity = 0;
	word->quote_state = sh_ast_plain_quote_state();
}

void	sh_heredoc_placeholder_init(t_heredoc_placeholder *placeholder)
{
	placeholder->should_expand = 0;
	placeholder->fd = -1;
	placeholder->path = NULL;
}

void	sh_heredoc_placeholder_destroy(t_heredoc_placeholder *placeholder)
{
	if (placeholder == NULL)
		return ;
	free(placeholder->path);
	placeholder->path = NULL;
	placeholder->fd = -1;
	placeholder->should_expand = 0;
}

void	sh_redirection_node_init(t_redirection_node *node)
{
	node->kind = SH_REDIRECTION_NONE;
	sh_command_word_init(&node->operand);
	sh_heredoc_placeholder_init(&node->heredoc);
	node->next = NULL;
}

void	sh_redirection_node_destroy(t_redirection_node *node)
{
	if (node == NULL)
		return ;
	sh_command_word_destroy(&node->operand);
	sh_heredoc_placeholder_destroy(&node->heredoc);
	node->kind = SH_REDIRECTION_NONE;
	node->next = NULL;
}

void	sh_simple_command_init(t_simple_command *command)
{
	command->argv = NULL;
	command->argc = 0;
	command->argv_capacity = 0;
	command->redirections = NULL;
}

void	sh_simple_command_destroy(t_simple_command *command)
{
	t_redirection_node	*node;
	t_redirection_node	*next;
	size_t				index;

	if (command == NULL)
		return ;
	index = 0;
	while (index < command->argc)
	{
		sh_command_word_destroy(&command->argv[index]);
		index++;
	}
	free(command->argv);
	command->argv = NULL;
	command->argc = 0;
	command->argv_capacity = 0;
	node = command->redirections;
	while (node != NULL)
	{
		next = node->next;
		sh_redirection_node_destroy(node);
		free(node);
		node = next;
	}
	command->redirections = NULL;
}
