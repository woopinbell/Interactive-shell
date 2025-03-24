#include "shell/prepare.h"

#include "shell/expand.h"
#include "shell/support/alloc.h"
#include "shell/support/error.h"
#include "shell/support/strbuf.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static void	sh_prepare_heredoc_reset(t_heredoc_placeholder *heredoc)
{
	if (heredoc->fd >= 0)
	{
		close(heredoc->fd);
		heredoc->fd = -1;
	}
	if (heredoc->path != NULL)
		unlink(heredoc->path);
	free(heredoc->path);
	heredoc->path = NULL;
}

static int	sh_prepare_write_all(int fd, const char *text, size_t len)
{
	ssize_t	written;
	size_t	offset;

	offset = 0;
	while (offset < len)
	{
		written = write(fd, text + offset, len - offset);
		if (written < 0)
			return (0);
		offset += (size_t)written;
	}
	return (1);
}

static int	sh_prepare_write_line(int fd, const char *line)
{
	size_t	len;

	len = strlen(line);
	if (!sh_prepare_write_all(fd, line, len))
		return (0);
	return (sh_prepare_write_all(fd, "\n", 1));
}

static int	sh_prepare_open_heredoc_storage(t_heredoc_placeholder *heredoc)
{
	char	*path;

	path = sh_xstrdup("/tmp/shell-heredoc-XXXXXX");
	heredoc->fd = mkstemp(path);
	if (heredoc->fd < 0)
	{
		free(path);
		return (0);
	}
	heredoc->path = path;
	return (1);
}

static int	sh_prepare_collect_heredoc_line(t_shell *shell,
		t_heredoc_placeholder *heredoc, char *line)
{
	char	*prepared_line;
	char	*original_line;

	original_line = line;
	prepared_line = line;
	if (heredoc->should_expand)
	{
		prepared_line = sh_expand_text(shell, line);
	}
	if (!sh_prepare_write_line(heredoc->fd, prepared_line))
	{
		free(original_line);
		if (prepared_line != original_line)
			free(prepared_line);
		return (0);
	}
	free(original_line);
	if (prepared_line != original_line)
		free(prepared_line);
	return (1);
}

static int	sh_prepare_collect_heredoc(t_shell *shell,
		t_redirection_node *node)
{
	t_shell_signal_phase	previous_phase;
	char					*line;

	if (node->kind != SH_REDIRECTION_HEREDOC || node->heredoc.delimiter == NULL)
		return (1);
	sh_prepare_heredoc_reset(&node->heredoc);
	if (!sh_prepare_open_heredoc_storage(&node->heredoc))
	{
		shell->last_status = 1;
		sh_perror("heredoc", "mkstemp");
		return (0);
	}
	previous_phase = shell->signal_phase;
	shell->signal_phase = SH_SIGNAL_PHASE_HEREDOC;
	while (1)
	{
		line = sh_input_adapter_read_line(&shell->input,
				shell->is_interactive ? "heredoc> " : "");
		if (line == NULL)
			break ;
		if (strcmp(line, node->heredoc.delimiter) == 0)
		{
			free(line);
			break ;
		}
		if (!sh_prepare_collect_heredoc_line(shell, &node->heredoc, line))
		{
			shell->signal_phase = previous_phase;
			shell->last_status = 1;
			sh_perror("heredoc", "write");
			sh_prepare_heredoc_reset(&node->heredoc);
			return (0);
		}
	}
	if (lseek(node->heredoc.fd, 0, SEEK_SET) < 0)
	{
		shell->signal_phase = previous_phase;
		shell->last_status = 1;
		sh_perror("heredoc", "lseek");
		sh_prepare_heredoc_reset(&node->heredoc);
		return (0);
	}
	shell->signal_phase = previous_phase;
	return (1);
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

int	sh_prepare_simple_command_heredocs(t_shell *shell, t_simple_command *command)
{
	t_redirection_node	*node;

	node = command->redirections;
	while (node != NULL)
	{
		if (!sh_prepare_collect_heredoc(shell, node))
			return (0);
		node = node->next;
	}
	return (1);
}

int	sh_prepare_pipeline_heredocs(t_shell *shell, t_pipeline *pipeline)
{
	t_pipeline_command_node	*node;

	node = pipeline->head;
	while (node != NULL)
	{
		if (!sh_prepare_simple_command_heredocs(shell, &node->command))
			return (0);
		node = node->next;
	}
	return (1);
}

int	sh_prepare_and_or_list_heredocs(t_shell *shell, t_and_or_list *list)
{
	t_and_or_list_node	*node;

	node = list->head;
	while (node != NULL)
	{
		if (!sh_prepare_pipeline_heredocs(shell, &node->pipeline))
			return (0);
		node = node->next;
	}
	return (1);
}

int	sh_prepare_sequence_heredocs(t_shell *shell, t_sequence_list *list)
{
	t_sequence_list_node	*node;

	node = list->head;
	while (node != NULL)
	{
		if (!sh_prepare_and_or_list_heredocs(shell, &node->and_or))
			return (0);
		node = node->next;
	}
	return (1);
}
