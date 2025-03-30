#include "shell/exec.h"
#include "shell/parser.h"
#include "shell/shell.h"
#include "shell/signal.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <readline/history.h>

static const char	*sh_main_prompt(const t_shell *shell)
{
	if (shell->is_interactive)
		return ("shell$ ");
	return ("");
}

static void	sh_main_select_input_adapter(t_shell *shell)
{
	if (shell->is_interactive)
		sh_input_adapter_use_readline(&shell->input);
	else
		sh_input_adapter_use_stream(&shell->input);
}

static void	sh_main_report_syntax_error(const t_parser *parser)
{
	const char	*token;

	token = sh_parser_error_token_text(parser);
	if (token == NULL)
		token = "newline";
	fprintf(stderr, "shell: %s `%s'\n",
		sh_parser_error_message(parser), token);
}

static void	sh_main_handle_line(t_shell *shell, const char *line)
{
	t_token_list		tokens;
	t_parser			parser;
	t_sequence_list	list;

	sh_token_list_init(&tokens);
	sh_sequence_list_init(&list);
	sh_lex_line(&tokens, line);
	sh_parser_init(&parser, &tokens);
	if (!sh_parse_sequence_list(&parser, &list))
	{
		sh_main_report_syntax_error(&parser);
		shell->last_status = 2;
	}
	else
		sh_executor_run(shell, &list);
	sh_sequence_list_destroy(&list);
	sh_token_list_destroy(&tokens);
}

static int	sh_main_repl(t_shell *shell)
{
	char	*line;

	line = NULL;
	while (1)
	{
		shell->signal_phase = SH_SIGNAL_PHASE_PROMPT;
		sh_signal_prompt_enter(shell);
		line = sh_input_adapter_read_line(&shell->input, sh_main_prompt(shell));
		sh_signal_prompt_leave(shell);
		if (sh_signal_prompt_interrupted(shell))
		{
			free(line);
			line = NULL;
			continue ;
		}
		if (line == NULL)
			break ;
		if (shell->is_interactive && line[0] != '\0')
			add_history(line);
		sh_main_handle_line(shell, line);
		free(line);
		if (shell->should_exit)
			break ;
	}
	shell->signal_phase = SH_SIGNAL_PHASE_INIT;
	if (shell->is_interactive)
		write(STDOUT_FILENO, "exit\n", 5);
	return (shell->last_status);
}

int	main(int argc, char **argv, char **envp)
{
	t_shell	shell;
	int		status;

	(void)argc;
	(void)argv;
	sh_shell_init(&shell, envp);
	sh_main_select_input_adapter(&shell);
	status = sh_main_repl(&shell);
	sh_shell_destroy(&shell);
	return (status);
}
