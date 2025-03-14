#include "shell/lexer.h"

#include "shell/support/alloc.h"

#include <stdlib.h>
#include <string.h>

#define SH_TOKEN_LIST_MIN_CAPACITY 8

static t_lexer_quote_state	sh_lexer_plain_quote_state(void)
{
	t_lexer_quote_state	state;

	state.kind = SH_QUOTE_STATE_NONE;
	state.is_closed = 1;
	return (state);
}

static char	*sh_lexer_dup_range(const char *start, size_t len)
{
	char	*copy;

	copy = sh_xmalloc(len + 1);
	memcpy(copy, start, len);
	copy[len] = '\0';
	return (copy);
}

static int	sh_lexer_is_blank(int c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r'
		|| c == '\v' || c == '\f');
}

static size_t	sh_lexer_operator_len(const char *input, t_token_kind *kind)
{
	if (input[0] == '&' && input[1] == '&')
	{
		*kind = SH_TOKEN_AND_IF;
		return (2);
	}
	if (input[0] == '|' && input[1] == '|')
	{
		*kind = SH_TOKEN_OR_IF;
		return (2);
	}
	if (input[0] == '>' && input[1] == '>')
	{
		*kind = SH_TOKEN_APPEND;
		return (2);
	}
	if (input[0] == '<' && input[1] == '<')
	{
		*kind = SH_TOKEN_HEREDOC;
		return (2);
	}
	if (input[0] == '|')
		*kind = SH_TOKEN_PIPE;
	else if (input[0] == ';')
		*kind = SH_TOKEN_SEMICOLON;
	else if (input[0] == '<')
		*kind = SH_TOKEN_REDIR_IN;
	else if (input[0] == '>')
		*kind = SH_TOKEN_REDIR_OUT;
	else
		return (0);
	return (1);
}

static void	sh_token_list_reserve(t_token_list *list, size_t needed)
{
	size_t	new_capacity;
	t_token	*new_items;
	size_t	index;

	if (list->capacity >= needed)
		return ;
	new_capacity = list->capacity;
	if (new_capacity == 0)
		new_capacity = SH_TOKEN_LIST_MIN_CAPACITY;
	while (new_capacity < needed)
		new_capacity *= 2;
	new_items = sh_xcalloc(new_capacity, sizeof(t_token));
	index = 0;
	while (index < list->len)
	{
		new_items[index] = list->items[index];
		index++;
	}
	free(list->items);
	list->items = new_items;
	list->capacity = new_capacity;
}

static void	sh_token_list_push(t_token_list *list, t_token token)
{
	sh_token_list_reserve(list, list->len + 1);
	list->items[list->len] = token;
	list->len += 1;
}

static void	sh_lexer_push_token(t_token_list *list, t_token_kind kind,
		const char *start, size_t len)
{
	t_token	token;

	token.kind = kind;
	token.lexeme = NULL;
	if (start != NULL)
		token.lexeme = sh_lexer_dup_range(start, len);
	token.quote_state = sh_lexer_plain_quote_state();
	sh_token_list_push(list, token);
}

static void	sh_lexer_reset(t_token_list *list)
{
	sh_token_list_destroy(list);
	sh_token_list_init(list);
}

void	sh_token_destroy(t_token *token)
{
	if (token == NULL)
		return ;
	free(token->lexeme);
	token->lexeme = NULL;
	token->kind = SH_TOKEN_EOF;
	token->quote_state = sh_lexer_plain_quote_state();
}

void	sh_token_list_init(t_token_list *list)
{
	list->items = NULL;
	list->len = 0;
	list->capacity = 0;
}

void	sh_token_list_destroy(t_token_list *list)
{
	size_t	index;

	if (list == NULL)
		return ;
	index = 0;
	while (index < list->len)
	{
		sh_token_destroy(&list->items[index]);
		index++;
	}
	free(list->items);
	list->items = NULL;
	list->len = 0;
	list->capacity = 0;
}

void	sh_lex_line(t_token_list *list, const char *input)
{
	t_token_kind	kind;
	size_t			operator_len;
	size_t			start;
	size_t			index;

	sh_lexer_reset(list);
	index = 0;
	while (input != NULL && input[index] != '\0')
	{
		while (sh_lexer_is_blank((unsigned char)input[index]))
			index++;
		if (input[index] == '\0')
			break ;
		operator_len = sh_lexer_operator_len(input + index, &kind);
		if (operator_len > 0)
		{
			sh_lexer_push_token(list, kind, input + index, operator_len);
			index += operator_len;
			continue ;
		}
		start = index;
		while (input[index] != '\0'
			&& !sh_lexer_is_blank((unsigned char)input[index])
			&& sh_lexer_operator_len(input + index, &kind) == 0)
			index++;
		sh_lexer_push_token(list, SH_TOKEN_WORD, input + start, index - start);
	}
	sh_lexer_push_token(list, SH_TOKEN_EOF, NULL, 0);
}

const char	*sh_token_kind_name(t_token_kind kind)
{
	if (kind == SH_TOKEN_WORD)
		return ("WORD");
	if (kind == SH_TOKEN_PIPE)
		return ("|");
	if (kind == SH_TOKEN_OR_IF)
		return ("||");
	if (kind == SH_TOKEN_AND_IF)
		return ("&&");
	if (kind == SH_TOKEN_SEMICOLON)
		return (";");
	if (kind == SH_TOKEN_REDIR_IN)
		return ("<");
	if (kind == SH_TOKEN_REDIR_OUT)
		return (">");
	if (kind == SH_TOKEN_APPEND)
		return (">>");
	if (kind == SH_TOKEN_HEREDOC)
		return ("<<");
	if (kind == SH_TOKEN_EOF)
		return ("EOF");
	return ("UNKNOWN");
}
