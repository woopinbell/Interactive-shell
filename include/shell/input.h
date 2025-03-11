#ifndef SHELL_INPUT_H
#define SHELL_INPUT_H

typedef char	*(*t_input_read_fn)(void *context, const char *prompt);
typedef void	(*t_input_destroy_fn)(void *context);

typedef enum e_input_adapter_kind
{
	SH_INPUT_ADAPTER_NONE,
	SH_INPUT_ADAPTER_STREAM,
	SH_INPUT_ADAPTER_READLINE
}	t_input_adapter_kind;

typedef struct s_input_adapter
{
	t_input_adapter_kind	kind;
	void					*context;
	t_input_read_fn			read_line;
	t_input_destroy_fn		destroy;
}	t_input_adapter;

void	sh_input_adapter_init(t_input_adapter *adapter);
void	sh_input_adapter_set(t_input_adapter *adapter,
			t_input_adapter_kind kind, void *context,
			t_input_read_fn read_line, t_input_destroy_fn destroy);
void	sh_input_adapter_use_stream(t_input_adapter *adapter);
void	sh_input_adapter_use_readline(t_input_adapter *adapter);
char	*sh_input_adapter_read_line(t_input_adapter *adapter,
			const char *prompt);
void	sh_input_adapter_destroy(t_input_adapter *adapter);

#endif
