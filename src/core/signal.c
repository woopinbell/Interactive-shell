#include "shell/signal.h"

#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t	g_prompt_sigint = 0;
static struct sigaction		g_prev_sigint;
static struct sigaction		g_prev_sigquit;
static int					g_prompt_handlers_installed = 0;
static t_shell				*g_prompt_shell = NULL;

static void	sh_signal_handle_prompt_sigint(int signo)
{
	(void)signo;
	g_prompt_sigint = 1;
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

static void	sh_signal_install_prompt_handlers(void)
{
	struct sigaction	action;

	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = sh_signal_handle_prompt_sigint;
	sigaction(SIGINT, &action, NULL);
	action.sa_handler = SIG_IGN;
	sigaction(SIGQUIT, &action, NULL);
}

void	sh_signal_prompt_enter(t_shell *shell)
{
	if (!shell->is_interactive)
		return ;
	g_prompt_shell = shell;
	g_prompt_sigint = 0;
	sh_signal_configure(SIGINT, sh_signal_handle_prompt_sigint, &g_prev_sigint);
	sh_signal_configure(SIGQUIT, SIG_IGN, &g_prev_sigquit);
	g_prompt_handlers_installed = 1;
}

void	sh_signal_prompt_leave(t_shell *shell)
{
	if (!shell->is_interactive || !g_prompt_handlers_installed)
		return ;
	sigaction(SIGINT, &g_prev_sigint, NULL);
	sigaction(SIGQUIT, &g_prev_sigquit, NULL);
	g_prompt_shell = NULL;
	g_prompt_handlers_installed = 0;
}

void	sh_signal_prompt_refresh(void)
{
	if (!g_prompt_handlers_installed || g_prompt_shell == NULL)
		return ;
	sh_signal_install_prompt_handlers();
}

int	sh_signal_prompt_interrupted(t_shell *shell)
{
	if (!g_prompt_sigint)
		return (0);
	g_prompt_sigint = 0;
	shell->last_status = 128 + SIGINT;
	return (1);
}
