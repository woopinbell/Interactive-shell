#ifndef SHELL_LEXER_H
#define SHELL_LEXER_H

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

#endif
