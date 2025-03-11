#include "shell/input.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <readline/readline.h>

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

static char	*sh_input_readline_read_line(void *context, const char *prompt)
{
	(void)context;
	return (readline(prompt));
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
	sh_input_adapter_set(adapter, SH_INPUT_ADAPTER_READLINE,
		NULL, sh_input_readline_read_line, NULL);
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
