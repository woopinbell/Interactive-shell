#include "shell/prepare.h"

#include "shell/expand.h"
#include "shell/support/alloc.h"
#include "shell/support/strbuf.h"

#include <stdlib.h>

static void	sh_prepare_word_part_text(t_shell *shell, t_strbuf *buf,
		const t_word_part *part)
{
	char	*expanded;

	if (part->quote_state.kind == SH_QUOTE_STATE_SINGLE)
	{
		sh_strbuf_append(buf, part->text);
		return ;
	}
	expanded = sh_expand_text(shell, part->text);
	sh_strbuf_append(buf, expanded);
	free(expanded);
}

static int	sh_prepare_redirection_operand(t_shell *shell,
		t_redirection_node *node)
{
	if (node->kind == SH_REDIRECTION_HEREDOC)
		return (1);
	return (sh_prepare_command_word(shell, &node->operand));
}

char	*sh_expand_text(t_shell *shell, const char *text)
{
	t_strbuf	buf;
	char		*expanded;
	size_t		index;
	size_t		consumed;

	sh_strbuf_init(&buf);
	index = 0;
	while (text != NULL && text[index] != '\0')
	{
		expanded = sh_parameter_expand(&shell->env, shell->last_status,
				text + index, &consumed);
		if (consumed > 0)
		{
			sh_strbuf_append(&buf, expanded);
			free(expanded);
			index += consumed;
			continue ;
		}
		sh_strbuf_push_char(&buf, text[index]);
		index++;
	}
	expanded = sh_strbuf_take(&buf);
	sh_strbuf_free(&buf);
	return (expanded);
}

char	*sh_prepare_word_text(t_shell *shell, const t_command_word *word)
{
	t_strbuf	buf;
	size_t		index;
	char		*prepared;

	if (word->parts.len == 0)
		return (sh_expand_text(shell, word->text));
	sh_strbuf_init(&buf);
	index = 0;
	while (index < word->parts.len)
	{
		sh_prepare_word_part_text(shell, &buf, &word->parts.items[index]);
		index++;
	}
	prepared = sh_strbuf_take(&buf);
	sh_strbuf_free(&buf);
	return (prepared);
}

int	sh_prepare_command_word(t_shell *shell, t_command_word *word)
{
	char	*prepared;

	prepared = sh_prepare_word_text(shell, word);
	free(word->text);
	word->text = prepared;
	return (1);
}

int	sh_prepare_simple_command_words(t_shell *shell, t_simple_command *command)
{
	t_redirection_node	*node;
	size_t				index;

	index = 0;
	while (index < command->argc)
	{
		sh_prepare_command_word(shell, &command->argv[index]);
		index++;
	}
	node = command->redirections;
	while (node != NULL)
	{
		if (!sh_prepare_redirection_operand(shell, node))
			return (0);
		node = node->next;
	}
	return (1);
}

int	sh_prepare_pipeline_words(t_shell *shell, t_pipeline *pipeline)
{
	t_pipeline_command_node	*node;

	node = pipeline->head;
	while (node != NULL)
	{
		if (!sh_prepare_simple_command_words(shell, &node->command))
			return (0);
		node = node->next;
	}
	return (1);
}

int	sh_prepare_and_or_list_words(t_shell *shell, t_and_or_list *list)
{
	t_and_or_list_node	*node;

	node = list->head;
	while (node != NULL)
	{
		if (!sh_prepare_pipeline_words(shell, &node->pipeline))
			return (0);
		node = node->next;
	}
	return (1);
}

int	sh_prepare_sequence_words(t_shell *shell, t_sequence_list *list)
{
	t_sequence_list_node	*node;

	node = list->head;
	while (node != NULL)
	{
		if (!sh_prepare_and_or_list_words(shell, &node->and_or))
			return (0);
		node = node->next;
	}
	return (1);
}
