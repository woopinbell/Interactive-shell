#include "shell/support/identifier.h"

int	sh_is_identifier_start(int c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_');
}

int	sh_is_identifier_char(int c)
{
	return (sh_is_identifier_start(c) || (c >= '0' && c <= '9'));
}

size_t	sh_identifier_key_len(const char *text)
{
	size_t	index;

	if (text == NULL || !sh_is_identifier_start((unsigned char)text[0]))
		return (0);
	index = 1;
	while (text[index] != '\0' && text[index] != '=')
	{
		if (!sh_is_identifier_char((unsigned char)text[index]))
			return (0);
		index++;
	}
	return (index);
}

int	sh_is_identifier(const char *text)
{
	size_t	length;

	length = sh_identifier_key_len(text);
	return (length > 0 && text[length] == '\0');
}

int	sh_is_assignment(const char *text)
{
	size_t	length;

	length = sh_identifier_key_len(text);
	return (length > 0 && text[length] == '=');
}
