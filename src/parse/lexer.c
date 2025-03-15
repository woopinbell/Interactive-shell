#include "shell/lexer.h"

#include "shell/support/alloc.h"
#include "shell/support/strbuf.h"

#include <stdlib.h>
#include <string.h>

#define SH_TOKEN_LIST_MIN_CAPACITY 8
#define SH_WORD_PART_LIST_MIN_CAPACITY 4

static t_lexer_quote_state	sh_lexer_quote_state(t_quote_state_kind kind,
		int is_closed)
{
	t_lexer_quote_state	state;

	state.kind = kind;
	state.is_closed = is_closed;
	return (state);
}

static t_lexer_quote_state	sh_lexer_plain_quote_state(void)
{
	return (sh_lexer_quote_state(SH_QUOTE_STATE_NONE, 1));
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

static void	sh_word_part_list_reserve(t_word_part_list *list, size_t needed)
{
	size_t		new_capacity;
	t_word_part	*new_items;
	size_t		index;

	if (list->capacity >= needed)
		return ;
	new_capacity = list->capacity;
	if (new_capacity == 0)
		new_capacity = SH_WORD_PART_LIST_MIN_CAPACITY;
	while (new_capacity < needed)
		new_capacity *= 2;
	new_items = sh_xcalloc(new_capacity, sizeof(t_word_part));
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

static void	sh_word_part_list_push(t_word_part_list *list, t_word_part part)
{
	sh_word_part_list_reserve(list, list->len + 1);
	list->items[list->len] = part;
	list->len += 1;
}

static t_token	sh_lexer_make_word_token(void)
{
	t_token	token;

	token.kind = SH_TOKEN_WORD;
	token.lexeme = NULL;
	token.quote_state = sh_lexer_plain_quote_state();
	token.parts.items = NULL;
	token.parts.len = 0;
	token.parts.capacity = 0;
	return (token);
}

static void	sh_lexer_append_part(t_token *token, const char *start, size_t len,
		t_lexer_quote_state quote_state)
{
	t_word_part	part;

	part.text = sh_lexer_dup_range(start, len);
	part.quote_state = quote_state;
	sh_word_part_list_push(&token->parts, part);
	if (!quote_state.is_closed)
		token->quote_state = quote_state;
}

static void	sh_lexer_finalize_word_token(t_token *token)
{
	t_strbuf	buf;
	size_t		index;

	sh_strbuf_init(&buf);
	index = 0;
	while (index < token->parts.len)
	{
		sh_strbuf_append(&buf, token->parts.items[index].text);
		index++;
	}
	token->lexeme = sh_strbuf_take(&buf);
	sh_strbuf_free(&buf);
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
	token.parts.items = NULL;
	token.parts.len = 0;
	token.parts.capacity = 0;
	sh_token_list_push(list, token);
}

static void	sh_lexer_reset(t_token_list *list)
{
	sh_token_list_destroy(list);
	sh_token_list_init(list);
}

static size_t	sh_lexer_scan_plain_part(const char *input, size_t index,
		t_token_kind *kind)
{
	size_t	start;

	start = index;
	while (input[index] != '\0'
		&& !sh_lexer_is_blank((unsigned char)input[index])
		&& input[index] != '\''
		&& input[index] != '"'
		&& sh_lexer_operator_len(input + index, kind) == 0)
		index++;
	return (index - start);
}

static size_t	sh_lexer_scan_quoted_part(const char *input, size_t index,
		t_token *token)
{
	char				quote_char;
	t_quote_state_kind	kind;
	size_t				start;
	int					is_closed;

	quote_char = input[index];
	if (quote_char == '\'')
		kind = SH_QUOTE_STATE_SINGLE;
	else
		kind = SH_QUOTE_STATE_DOUBLE;
	index++;
	start = index;
	while (input[index] != '\0' && input[index] != quote_char)
		index++;
	is_closed = (input[index] == quote_char);
	sh_lexer_append_part(token, input + start, index - start,
		sh_lexer_quote_state(kind, is_closed));
	if (is_closed)
		index++;
	return (index);
}

static size_t	sh_lexer_scan_word(const char *input, size_t index,
		t_token_list *list)
{
	t_token			token;
	t_token_kind	kind;
	size_t			part_len;

	token = sh_lexer_make_word_token();
	while (input[index] != '\0'
		&& !sh_lexer_is_blank((unsigned char)input[index])
		&& sh_lexer_operator_len(input + index, &kind) == 0)
	{
		if (input[index] == '\'' || input[index] == '"')
		{
			index = sh_lexer_scan_quoted_part(input, index, &token);
			continue ;
		}
		part_len = sh_lexer_scan_plain_part(input, index, &kind);
		sh_lexer_append_part(&token, input + index, part_len,
			sh_lexer_plain_quote_state());
		index += part_len;
	}
	sh_lexer_finalize_word_token(&token);
	sh_token_list_push(list, token);
	return (index);
}

void	sh_word_part_destroy(t_word_part *part)
{
	if (part == NULL)
		return ;
	free(part->text);
	part->text = NULL;
	part->quote_state = sh_lexer_plain_quote_state();
}

void	sh_token_destroy(t_token *token)
{
	size_t	index;

	if (token == NULL)
		return ;
	free(token->lexeme);
	token->lexeme = NULL;
	index = 0;
	while (index < token->parts.len)
	{
		sh_word_part_destroy(&token->parts.items[index]);
		index++;
	}
	free(token->parts.items);
	token->parts.items = NULL;
	token->parts.len = 0;
	token->parts.capacity = 0;
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
		index = sh_lexer_scan_word(input, index, list);
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
