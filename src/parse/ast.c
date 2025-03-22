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
	placeholder->delimiter = NULL;
	placeholder->should_expand = 0;
	placeholder->fd = -1;
	placeholder->path = NULL;
}

void	sh_heredoc_placeholder_destroy(t_heredoc_placeholder *placeholder)
{
	if (placeholder == NULL)
		return ;
	free(placeholder->delimiter);
	placeholder->delimiter = NULL;
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

void	sh_pipeline_command_node_init(t_pipeline_command_node *node)
{
	sh_simple_command_init(&node->command);
	node->next = NULL;
}

void	sh_pipeline_command_node_destroy(t_pipeline_command_node *node)
{
	if (node == NULL)
		return ;
	sh_simple_command_destroy(&node->command);
	node->next = NULL;
}

void	sh_pipeline_init(t_pipeline *pipeline)
{
	pipeline->head = NULL;
	pipeline->tail = NULL;
	pipeline->size = 0;
}

void	sh_pipeline_destroy(t_pipeline *pipeline)
{
	t_pipeline_command_node	*node;
	t_pipeline_command_node	*next;

	if (pipeline == NULL)
		return ;
	node = pipeline->head;
	while (node != NULL)
	{
		next = node->next;
		sh_pipeline_command_node_destroy(node);
		free(node);
		node = next;
	}
	pipeline->head = NULL;
	pipeline->tail = NULL;
	pipeline->size = 0;
}

void	sh_and_or_list_node_init(t_and_or_list_node *node)
{
	sh_pipeline_init(&node->pipeline);
	node->next_operator = SH_AND_OR_OPERATOR_NONE;
	node->next = NULL;
}

void	sh_and_or_list_node_destroy(t_and_or_list_node *node)
{
	if (node == NULL)
		return ;
	sh_pipeline_destroy(&node->pipeline);
	node->next_operator = SH_AND_OR_OPERATOR_NONE;
	node->next = NULL;
}

void	sh_and_or_list_init(t_and_or_list *list)
{
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
}

void	sh_and_or_list_destroy(t_and_or_list *list)
{
	t_and_or_list_node	*node;
	t_and_or_list_node	*next;

	if (list == NULL)
		return ;
	node = list->head;
	while (node != NULL)
	{
		next = node->next;
		sh_and_or_list_node_destroy(node);
		free(node);
		node = next;
	}
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
}

void	sh_sequence_list_node_init(t_sequence_list_node *node)
{
	sh_and_or_list_init(&node->and_or);
	node->next_separator = SH_SEQUENCE_SEPARATOR_NONE;
	node->next = NULL;
}

void	sh_sequence_list_node_destroy(t_sequence_list_node *node)
{
	if (node == NULL)
		return ;
	sh_and_or_list_destroy(&node->and_or);
	node->next_separator = SH_SEQUENCE_SEPARATOR_NONE;
	node->next = NULL;
}

void	sh_sequence_list_init(t_sequence_list *list)
{
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
}

void	sh_sequence_list_destroy(t_sequence_list *list)
{
	t_sequence_list_node	*node;
	t_sequence_list_node	*next;

	if (list == NULL)
		return ;
	node = list->head;
	while (node != NULL)
	{
		next = node->next;
		sh_sequence_list_node_destroy(node);
		free(node);
		node = next;
	}
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
}
