#include "shell/input.h"

#include <stddef.h>

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

void	sh_input_adapter_destroy(t_input_adapter *adapter)
{
	if (adapter->destroy != NULL)
		adapter->destroy(adapter->context);
	sh_input_adapter_init(adapter);
}
