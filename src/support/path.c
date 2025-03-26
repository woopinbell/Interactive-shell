#include "shell/support/path.h"

#include "shell/support/alloc.h"
#include "shell/support/strbuf.h"

#include <stddef.h>
#include <string.h>

static char	*sh_path_join_len(const char *left, const char *right,
		size_t right_len)
{
	t_strbuf	buf;
	char		*joined;
	size_t		left_len;
	int			need_separator;

	left_len = strlen(left);
	need_separator = (left_len > 0 && left[left_len - 1] != '/'
			&& right_len > 0 && right[0] != '/');
	sh_strbuf_init(&buf);
	sh_strbuf_append(&buf, left);
	if (need_separator)
		sh_strbuf_push_char(&buf, '/');
	if (left_len > 0 && left[left_len - 1] == '/' && right_len > 0
		&& right[0] == '/')
		sh_strbuf_append_n(&buf, right + 1, right_len - 1);
	else
		sh_strbuf_append_n(&buf, right, right_len);
	joined = sh_strbuf_take(&buf);
	sh_strbuf_free(&buf);
	return (joined);
}

char	*sh_path_join(const char *left, const char *right)
{
	return (sh_path_join_len(left, right, strlen(right)));
}

char	*sh_path_join_n(const char *left, const char *right, size_t right_len)
{
	return (sh_path_join_len(left, right, right_len));
}
