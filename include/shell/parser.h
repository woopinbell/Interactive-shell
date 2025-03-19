#ifndef SHELL_PARSER_H
#define SHELL_PARSER_H

#include "shell/ast.h"
#include "shell/lexer.h"

#include <stddef.h>

typedef struct s_parser
{
	const t_token_list	*tokens;
	size_t				index;
	const char			*error_message;
	const t_token		*error_token;
}	t_parser;

void			sh_parser_init(t_parser *parser, const t_token_list *tokens);
int				sh_parser_has_error(const t_parser *parser);
const char		*sh_parser_error_message(const t_parser *parser);
const char		*sh_parser_error_token_text(const t_parser *parser);
const t_token	*sh_parser_current(const t_parser *parser);
int				sh_parser_can_start_simple_command(const t_parser *parser);
int				sh_parse_simple_command(t_parser *parser,
					t_simple_command *command);

#endif
