#include "shell/support/strbuf.h"

#include "shell/support/alloc.h"

#include <stdlib.h>
#include <string.h>

#define SH_STRBUF_MIN_CAPACITY 16

static void	sh_strbuf_bootstrap(t_strbuf *buf)
{
	buf->capacity = SH_STRBUF_MIN_CAPACITY;
	buf->data = sh_xcalloc(buf->capacity, sizeof(char));
	buf->len = 0;
}

static void	sh_strbuf_reserve(t_strbuf *buf, size_t needed)
{
	size_t	new_capacity;
	char	*new_data;

	if (buf->capacity > needed)
		return ;
	new_capacity = buf->capacity;
	while (new_capacity <= needed)
		new_capacity *= 2;
	new_data = sh_xmalloc(new_capacity);
	memcpy(new_data, buf->data, buf->len + 1);
	free(buf->data);
	buf->data = new_data;
	buf->capacity = new_capacity;
}

void	sh_strbuf_init(t_strbuf *buf)
{
	sh_strbuf_bootstrap(buf);
}

void	sh_strbuf_clear(t_strbuf *buf)
{
	buf->len = 0;
	buf->data[0] = '\0';
}

void	sh_strbuf_free(t_strbuf *buf)
{
	free(buf->data);
	buf->data = NULL;
	buf->len = 0;
	buf->capacity = 0;
}

void	sh_strbuf_append_n(t_strbuf *buf, const char *text, size_t len)
{
	sh_strbuf_reserve(buf, buf->len + len);
	memcpy(buf->data + buf->len, text, len);
	buf->len += len;
	buf->data[buf->len] = '\0';
}

void	sh_strbuf_append(t_strbuf *buf, const char *text)
{
	sh_strbuf_append_n(buf, text, strlen(text));
}

void	sh_strbuf_push_char(t_strbuf *buf, char c)
{
	sh_strbuf_reserve(buf, buf->len + 1);
	buf->data[buf->len] = c;
	buf->len += 1;
	buf->data[buf->len] = '\0';
}

char	*sh_strbuf_take(t_strbuf *buf)
{
	char	*taken;

	taken = buf->data;
	sh_strbuf_bootstrap(buf);
	return (taken);
}
