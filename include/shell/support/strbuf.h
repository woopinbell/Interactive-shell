#ifndef SHELL_SUPPORT_STRBUF_H
#define SHELL_SUPPORT_STRBUF_H

#include <stddef.h>

typedef struct s_strbuf
{
	char	*data;
	size_t	len;
	size_t	capacity;
}	t_strbuf;

void	sh_strbuf_init(t_strbuf *buf);
void	sh_strbuf_clear(t_strbuf *buf);
void	sh_strbuf_free(t_strbuf *buf);
void	sh_strbuf_append(t_strbuf *buf, const char *text);
void	sh_strbuf_append_n(t_strbuf *buf, const char *text, size_t len);
void	sh_strbuf_push_char(t_strbuf *buf, char c);
char	*sh_strbuf_take(t_strbuf *buf);

#endif
