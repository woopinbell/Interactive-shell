#include "shell/parser.h"

#include "shell/support/alloc.h"

#include <stdlib.h>
#include <string.h>

static int	sh_parser_is_redirection_token(t_token_kind kind)
{
	return (kind == SH_TOKEN_REDIR_IN || kind == SH_TOKEN_REDIR_OUT
		|| kind == SH_TOKEN_APPEND || kind == SH_TOKEN_HEREDOC);
}

static int	sh_parser_token_can_start_simple_command(const t_token *token)
{
	if (token == NULL)
		return (0);
	return (token->kind == SH_TOKEN_WORD
		|| sh_parser_is_redirection_token(token->kind));
}

static const t_token	*sh_parser_peek(const t_parser *parser, size_t offset)
{
	size_t	index;

	if (parser->tokens == NULL || parser->tokens->len == 0)
		return (NULL);
	index = parser->index + offset;
	if (index >= parser->tokens->len)
		return (&parser->tokens->items[parser->tokens->len - 1]);
	return (&parser->tokens->items[index]);
}

static void	sh_parser_set_syntax_error(t_parser *parser, const t_token *token)
{
	if (parser->error_message != NULL)
		return ;
	parser->error_message = "syntax error near unexpected token";
	parser->error_token = token;
}

static void	sh_parser_advance(t_parser *parser)
{
	if (parser->tokens == NULL || parser->index >= parser->tokens->len)
		return ;
	if (parser->tokens->items[parser->index].kind != SH_TOKEN_EOF)
		parser->index += 1;
}

static void	sh_parser_copy_word_parts(t_word_part_list *dest,
		const t_word_part_list *source)
{
	size_t	index;

	dest->items = NULL;
	dest->len = source->len;
	dest->capacity = source->len;
	if (source->len == 0)
		return ;
	dest->items = sh_xcalloc(source->len, sizeof(t_word_part));
	index = 0;
	while (index < source->len)
	{
		dest->items[index].text = sh_xstrdup(source->items[index].text);
		dest->items[index].quote_state = source->items[index].quote_state;
		index++;
	}
}

static void	sh_parser_copy_command_word(t_command_word *word,
		const t_token *token)
{
	sh_command_word_init(word);
	if (token->lexeme != NULL)
		word->text = sh_xstrdup(token->lexeme);
	word->quote_state = token->quote_state;
	sh_parser_copy_word_parts(&word->parts, &token->parts);
}

static void	sh_parser_simple_command_reserve_argv(t_simple_command *command,
		size_t needed)
{
	size_t			new_capacity;
	t_command_word	*new_argv;
	size_t			index;

	if (command->argv_capacity >= needed)
		return ;
	new_capacity = command->argv_capacity;
	if (new_capacity == 0)
		new_capacity = 4;
	while (new_capacity < needed)
		new_capacity *= 2;
	new_argv = sh_xcalloc(new_capacity, sizeof(t_command_word));
	index = 0;
	while (index < command->argc)
	{
		new_argv[index] = command->argv[index];
		index++;
	}
	free(command->argv);
	command->argv = new_argv;
	command->argv_capacity = new_capacity;
}

static void	sh_parser_append_argv_word(t_simple_command *command,
		const t_token *token)
{
	sh_parser_simple_command_reserve_argv(command, command->argc + 1);
	sh_parser_copy_command_word(&command->argv[command->argc], token);
	command->argc += 1;
}

static t_redirection_kind	sh_parser_redirection_kind(t_token_kind kind)
{
	if (kind == SH_TOKEN_REDIR_IN)
		return (SH_REDIRECTION_INPUT);
	if (kind == SH_TOKEN_REDIR_OUT)
		return (SH_REDIRECTION_OUTPUT);
	if (kind == SH_TOKEN_APPEND)
		return (SH_REDIRECTION_APPEND);
	if (kind == SH_TOKEN_HEREDOC)
		return (SH_REDIRECTION_HEREDOC);
	return (SH_REDIRECTION_NONE);
}

static int	sh_parser_word_has_quoted_part(const t_command_word *word)
{
	size_t	index;

	index = 0;
	while (index < word->parts.len)
	{
		if (word->parts.items[index].quote_state.kind != SH_QUOTE_STATE_NONE)
			return (1);
		index++;
	}
	return (0);
}

static void	sh_parser_fill_heredoc_metadata(t_redirection_node *node)
{
	if (node->kind != SH_REDIRECTION_HEREDOC)
		return ;
	if (node->operand.text != NULL)
		node->heredoc.delimiter = sh_xstrdup(node->operand.text);
	node->heredoc.should_expand = !sh_parser_word_has_quoted_part(&node->operand);
}

static void	sh_parser_append_redirection(t_simple_command *command,
		t_redirection_node *node)
{
	t_redirection_node	*tail;

	if (command->redirections == NULL)
	{
		command->redirections = node;
		return ;
	}
	tail = command->redirections;
	while (tail->next != NULL)
		tail = tail->next;
	tail->next = node;
}

static void	sh_parser_append_pipeline_node(t_pipeline *pipeline,
		t_pipeline_command_node *node)
{
	if (pipeline->tail == NULL)
		pipeline->head = node;
	else
		pipeline->tail->next = node;
	pipeline->tail = node;
	pipeline->size += 1;
}

static void	sh_parser_append_and_or_node(t_and_or_list *list,
		t_and_or_list_node *node)
{
	if (list->tail == NULL)
		list->head = node;
	else
		list->tail->next = node;
	list->tail = node;
	list->size += 1;
}

static void	sh_parser_append_sequence_node(t_sequence_list *list,
		t_sequence_list_node *node)
{
	if (list->tail == NULL)
		list->head = node;
	else
		list->tail->next = node;
	list->tail = node;
	list->size += 1;
}

static int	sh_parser_parse_redirection(t_parser *parser,
		t_simple_command *command)
{
	t_redirection_node	*node;
	t_token_kind		token_kind;
	const t_token		*operand;

	token_kind = sh_parser_current(parser)->kind;
	sh_parser_advance(parser);
	operand = sh_parser_current(parser);
	if (operand == NULL || operand->kind != SH_TOKEN_WORD)
	{
		sh_parser_set_syntax_error(parser, operand);
		return (0);
	}
	node = sh_xcalloc(1, sizeof(t_redirection_node));
	sh_redirection_node_init(node);
	node->kind = sh_parser_redirection_kind(token_kind);
	sh_parser_copy_command_word(&node->operand, operand);
	sh_parser_fill_heredoc_metadata(node);
	sh_parser_append_redirection(command, node);
	sh_parser_advance(parser);
	return (1);
}

static int	sh_parser_has_missing_followup(const t_parser *parser)
{
	const t_token	*current;
	const t_token	*next;

	current = sh_parser_current(parser);
	if (current == NULL)
		return (0);
	if (current->kind != SH_TOKEN_PIPE && current->kind != SH_TOKEN_AND_IF
		&& current->kind != SH_TOKEN_OR_IF)
		return (0);
	next = sh_parser_peek(parser, 1);
	return (!sh_parser_token_can_start_simple_command(next));
}

void	sh_parser_init(t_parser *parser, const t_token_list *tokens)
{
	parser->tokens = tokens;
	parser->index = 0;
	parser->error_message = NULL;
	parser->error_token = NULL;
}

int	sh_parser_has_error(const t_parser *parser)
{
	return (parser->error_message != NULL);
}

const char	*sh_parser_error_message(const t_parser *parser)
{
	return (parser->error_message);
}

const char	*sh_parser_error_token_text(const t_parser *parser)
{
	if (parser->error_token == NULL)
		return (NULL);
	if (parser->error_token->kind == SH_TOKEN_EOF)
		return ("newline");
	if (parser->error_token->lexeme != NULL)
		return (parser->error_token->lexeme);
	return (sh_token_kind_name(parser->error_token->kind));
}

const t_token	*sh_parser_current(const t_parser *parser)
{
	return (sh_parser_peek(parser, 0));
}

int	sh_parser_can_start_simple_command(const t_parser *parser)
{
	return (sh_parser_token_can_start_simple_command(sh_parser_current(parser)));
}

int	sh_parse_simple_command(t_parser *parser, t_simple_command *command)
{
	const t_token	*current;

	if (!sh_parser_can_start_simple_command(parser))
	{
		sh_parser_set_syntax_error(parser, sh_parser_current(parser));
		return (0);
	}
	while (!sh_parser_has_error(parser))
	{
		current = sh_parser_current(parser);
		if (current == NULL)
			break ;
		if (current->kind == SH_TOKEN_WORD)
		{
			sh_parser_append_argv_word(command, current);
			sh_parser_advance(parser);
			continue ;
		}
		if (sh_parser_is_redirection_token(current->kind))
		{
			if (!sh_parser_parse_redirection(parser, command))
				return (0);
			continue ;
		}
		break ;
	}
	if (sh_parser_has_missing_followup(parser))
	{
		sh_parser_set_syntax_error(parser, sh_parser_peek(parser, 1));
		return (0);
	}
	return (!sh_parser_has_error(parser));
}

int	sh_parse_pipeline(t_parser *parser, t_pipeline *pipeline)
{
	t_pipeline_command_node	*node;

	node = sh_xcalloc(1, sizeof(t_pipeline_command_node));
	sh_pipeline_command_node_init(node);
	if (!sh_parse_simple_command(parser, &node->command))
	{
		free(node);
		return (0);
	}
	sh_parser_append_pipeline_node(pipeline, node);
	while (sh_parser_current(parser) != NULL
		&& sh_parser_current(parser)->kind == SH_TOKEN_PIPE)
	{
		sh_parser_advance(parser);
		node = sh_xcalloc(1, sizeof(t_pipeline_command_node));
		sh_pipeline_command_node_init(node);
		if (!sh_parse_simple_command(parser, &node->command))
		{
			free(node);
			return (0);
		}
		sh_parser_append_pipeline_node(pipeline, node);
	}
	return (!sh_parser_has_error(parser));
}

int	sh_parse_and_or_list(t_parser *parser, t_and_or_list *list)
{
	t_and_or_list_node	*node;
	t_token_kind		kind;

	node = sh_xcalloc(1, sizeof(t_and_or_list_node));
	sh_and_or_list_node_init(node);
	if (!sh_parse_pipeline(parser, &node->pipeline))
	{
		free(node);
		return (0);
	}
	sh_parser_append_and_or_node(list, node);
	while (sh_parser_current(parser) != NULL
		&& (sh_parser_current(parser)->kind == SH_TOKEN_AND_IF
			|| sh_parser_current(parser)->kind == SH_TOKEN_OR_IF))
	{
		kind = sh_parser_current(parser)->kind;
		if (kind == SH_TOKEN_AND_IF)
			node->next_operator = SH_AND_OR_OPERATOR_AND_IF;
		else
			node->next_operator = SH_AND_OR_OPERATOR_OR_IF;
		sh_parser_advance(parser);
		node = sh_xcalloc(1, sizeof(t_and_or_list_node));
		sh_and_or_list_node_init(node);
		if (!sh_parse_pipeline(parser, &node->pipeline))
		{
			free(node);
			return (0);
		}
		sh_parser_append_and_or_node(list, node);
	}
	return (!sh_parser_has_error(parser));
}

int	sh_parse_sequence_list(t_parser *parser, t_sequence_list *list)
{
	t_sequence_list_node	*node;

	if (sh_parser_current(parser) == NULL
		|| sh_parser_current(parser)->kind == SH_TOKEN_EOF)
		return (1);
	node = sh_xcalloc(1, sizeof(t_sequence_list_node));
	sh_sequence_list_node_init(node);
	if (!sh_parse_and_or_list(parser, &node->and_or))
	{
		free(node);
		return (0);
	}
	sh_parser_append_sequence_node(list, node);
	while (sh_parser_current(parser) != NULL
		&& sh_parser_current(parser)->kind == SH_TOKEN_SEMICOLON)
	{
		node->next_separator = SH_SEQUENCE_SEPARATOR_SEMICOLON;
		sh_parser_advance(parser);
		if (sh_parser_current(parser) == NULL
			|| sh_parser_current(parser)->kind == SH_TOKEN_EOF)
			break ;
		node = sh_xcalloc(1, sizeof(t_sequence_list_node));
		sh_sequence_list_node_init(node);
		if (!sh_parse_and_or_list(parser, &node->and_or))
		{
			free(node);
			return (0);
		}
		sh_parser_append_sequence_node(list, node);
	}
	return (!sh_parser_has_error(parser));
}
