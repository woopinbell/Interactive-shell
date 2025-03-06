#include "shell/support/path.h"

#include "shell/support/alloc.h"
#include "shell/support/strbuf.h"

#include <string.h>

char	*sh_path_join(const char *left, const char *right)
{
	t_strbuf	buf;
	char		*joined;
	size_t		left_len;
	int			need_separator;

	left_len = strlen(left);
	need_separator = (left_len > 0 && left[left_len - 1] != '/'
			&& right[0] != '\0' && right[0] != '/');
	sh_strbuf_init(&buf);
	sh_strbuf_append(&buf, left);
	if (need_separator)
		sh_strbuf_push_char(&buf, '/');
	if (left_len > 0 && left[left_len - 1] == '/' && right[0] == '/')
		sh_strbuf_append(&buf, right + 1);
	else
		sh_strbuf_append(&buf, right);
	joined = sh_strbuf_take(&buf);
	sh_strbuf_free(&buf);
	return (joined);
}
