#include "shell/signal.h"

#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t	g_input_sigint = 0;
static struct sigaction		g_input_prev_sigint;
static struct sigaction		g_input_prev_sigquit;
static struct sigaction		g_wait_prev_sigint;
static struct sigaction		g_wait_prev_sigquit;
static int					g_input_handlers_installed = 0;
static int					g_wait_handlers_installed = 0;
static t_shell				*g_input_shell = NULL;

static void	sh_signal_handle_input_sigint(int signo)
{
	(void)signo;
	g_input_sigint = 1;
	write(STDOUT_FILENO, "\n", 1);
}

static void	sh_signal_configure(int signo, void (*handler)(int),
		struct sigaction *previous)
{
	struct sigaction	action;

	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = handler;
	sigaction(signo, &action, previous);
}

static void	sh_signal_install_input_handlers(void)
{
	struct sigaction	action;

	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = sh_signal_handle_input_sigint;
	sigaction(SIGINT, &action, NULL);
	action.sa_handler = SIG_IGN;
	sigaction(SIGQUIT, &action, NULL);
}

static void	sh_signal_input_enter(t_shell *shell)
{
	if (!shell->is_interactive)
		return ;
	g_input_shell = shell;
	g_input_sigint = 0;
	sh_signal_configure(SIGINT, sh_signal_handle_input_sigint,
		&g_input_prev_sigint);
	sh_signal_configure(SIGQUIT, SIG_IGN, &g_input_prev_sigquit);
	g_input_handlers_installed = 1;
}

static void	sh_signal_input_leave(t_shell *shell)
{
	if (!shell->is_interactive || !g_input_handlers_installed)
		return ;
	sigaction(SIGINT, &g_input_prev_sigint, NULL);
	sigaction(SIGQUIT, &g_input_prev_sigquit, NULL);
	g_input_shell = NULL;
	g_input_handlers_installed = 0;
}

static int	sh_signal_input_interrupted(t_shell *shell)
{
	if (!g_input_sigint)
		return (0);
	g_input_sigint = 0;
	shell->last_status = 128 + SIGINT;
	return (1);
}

void	sh_signal_prompt_enter(t_shell *shell)
{
	sh_signal_input_enter(shell);
}

void	sh_signal_prompt_leave(t_shell *shell)
{
	sh_signal_input_leave(shell);
}

void	sh_signal_heredoc_enter(t_shell *shell)
{
	sh_signal_input_enter(shell);
}

void	sh_signal_heredoc_leave(t_shell *shell)
{
	sh_signal_input_leave(shell);
}

void	sh_signal_input_refresh(void)
{
	if (!g_input_handlers_installed || g_input_shell == NULL)
		return ;
	sh_signal_install_input_handlers();
}

int	sh_signal_prompt_interrupted(t_shell *shell)
{
	return (sh_signal_input_interrupted(shell));
}

int	sh_signal_heredoc_interrupted(t_shell *shell)
{
	return (sh_signal_input_interrupted(shell));
}

void	sh_signal_wait_enter(t_shell *shell)
{
	if (!shell->is_interactive)
		return ;
	sh_signal_configure(SIGINT, SIG_IGN, &g_wait_prev_sigint);
	sh_signal_configure(SIGQUIT, SIG_IGN, &g_wait_prev_sigquit);
	g_wait_handlers_installed = 1;
}

void	sh_signal_wait_leave(t_shell *shell)
{
	if (!shell->is_interactive || !g_wait_handlers_installed)
		return ;
	sigaction(SIGINT, &g_wait_prev_sigint, NULL);
	sigaction(SIGQUIT, &g_wait_prev_sigquit, NULL);
	g_wait_handlers_installed = 0;
}

void	sh_signal_child_default(void)
{
	struct sigaction	action;

	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = SIG_DFL;
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGQUIT, &action, NULL);
}
