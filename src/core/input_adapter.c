#include "shell/input.h"
#include "shell/signal.h"

#include "shell/support/alloc.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

#include <readline/readline.h>

typedef struct s_readline_context
{
	char	*line;
	int		done;
}	t_readline_context;

static char	*sh_input_stream_read_line(void *context, const char *prompt)
{
	FILE	*stream;
	char	*line;
	size_t	capacity;
	ssize_t	read_size;

	(void)prompt;
	stream = context;
	if (stream == NULL)
		stream = stdin;
	line = NULL;
	capacity = 0;
	read_size = getline(&line, &capacity, stream);
	if (read_size < 0)
	{
		free(line);
		return (NULL);
	}
	if (read_size > 0 && line[read_size - 1] == '\n')
		line[--read_size] = '\0';
	if (read_size > 0 && line[read_size - 1] == '\r')
		line[read_size - 1] = '\0';
	return (line);
}

static t_readline_context	*g_readline_context = NULL;

static void	sh_input_readline_handle_line(char *line)
{
	if (g_readline_context == NULL)
	{
		free(line);
		return ;
	}
	g_readline_context->line = line;
	g_readline_context->done = 1;
}

static void	sh_input_readline_destroy(void *context)
{
	free(context);
}

static char	*sh_input_readline_read_line(void *context, const char *prompt)
{
	t_readline_context	*readline_context;
	fd_set				readfds;
	int					ready;

	readline_context = context;
	readline_context->line = NULL;
	readline_context->done = 0;
	g_readline_context = readline_context;
	rl_callback_handler_install(prompt, sh_input_readline_handle_line);
	sh_signal_prompt_refresh();
	while (!readline_context->done)
	{
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, NULL);
		if (ready > 0 && FD_ISSET(STDIN_FILENO, &readfds))
			rl_callback_read_char();
		else if (ready < 0 && errno == EINTR)
			break ;
		else if (ready < 0)
		{
			rl_callback_handler_remove();
			g_readline_context = NULL;
			return (NULL);
		}
	}
	if (!readline_context->done)
		rl_free_line_state();
	rl_callback_handler_remove();
	g_readline_context = NULL;
	return (readline_context->line);
}

void	sh_input_adapter_init(t_input_adapter *adapter)
{
	adapter->kind = SH_INPUT_ADAPTER_NONE;
	adapter->context = NULL;
	adapter->read_line = NULL;
	adapter->destroy = NULL;
}

void	sh_input_adapter_set(t_input_adapter *adapter,
		t_input_adapter_kind kind, void *context,
		t_input_read_fn read_line, t_input_destroy_fn destroy)
{
	sh_input_adapter_destroy(adapter);
	adapter->kind = kind;
	adapter->context = context;
	adapter->read_line = read_line;
	adapter->destroy = destroy;
}

void	sh_input_adapter_use_stream(t_input_adapter *adapter)
{
	sh_input_adapter_set(adapter, SH_INPUT_ADAPTER_STREAM,
		stdin, sh_input_stream_read_line, NULL);
}

void	sh_input_adapter_use_readline(t_input_adapter *adapter)
{
	t_readline_context	*context;

	context = sh_xcalloc(1, sizeof(t_readline_context));
	sh_input_adapter_set(adapter, SH_INPUT_ADAPTER_READLINE,
		context, sh_input_readline_read_line, sh_input_readline_destroy);
}

char	*sh_input_adapter_read_line(t_input_adapter *adapter, const char *prompt)
{
	if (adapter->read_line == NULL)
		return (NULL);
	return (adapter->read_line(adapter->context, prompt));
}

void	sh_input_adapter_destroy(t_input_adapter *adapter)
{
	if (adapter->destroy != NULL)
		adapter->destroy(adapter->context);
	sh_input_adapter_init(adapter);
}
