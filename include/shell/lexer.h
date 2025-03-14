#ifndef SHELL_LEXER_H
#define SHELL_LEXER_H

#include <stddef.h>

typedef enum e_token_kind
{
	SH_TOKEN_WORD,
	SH_TOKEN_PIPE,
	SH_TOKEN_OR_IF,
	SH_TOKEN_AND_IF,
	SH_TOKEN_SEMICOLON,
	SH_TOKEN_REDIR_IN,
	SH_TOKEN_REDIR_OUT,
	SH_TOKEN_APPEND,
	SH_TOKEN_HEREDOC,
	SH_TOKEN_EOF
}	t_token_kind;

typedef enum e_quote_state_kind
{
	SH_QUOTE_STATE_NONE,
	SH_QUOTE_STATE_SINGLE,
	SH_QUOTE_STATE_DOUBLE
}	t_quote_state_kind;

typedef struct s_lexer_quote_state
{
	t_quote_state_kind	kind;
	int					is_closed;
}	t_lexer_quote_state;

typedef struct s_token
{
	t_token_kind			kind;
	char					*lexeme;
	t_lexer_quote_state		quote_state;
}	t_token;

typedef struct s_token_list
{
	t_token	*items;
	size_t	len;
	size_t	capacity;
}	t_token_list;

void		sh_token_destroy(t_token *token);
void		sh_token_list_init(t_token_list *list);
void		sh_token_list_destroy(t_token_list *list);
void		sh_lex_line(t_token_list *list, const char *input);
const char	*sh_token_kind_name(t_token_kind kind);

#endif
