#ifndef SHELL_BUILTIN_H
#define SHELL_BUILTIN_H

#include "shell/shell.h"

typedef int	(*t_builtin_fn)(t_shell *shell, int argc, char **argv);

t_builtin_fn	sh_builtin_find(const char *name);
int				sh_builtin_dispatch(t_shell *shell, int argc,
					char **argv, int *handled);
int				sh_builtin_echo(t_shell *shell, int argc, char **argv);
int				sh_builtin_pwd(t_shell *shell, int argc, char **argv);
int				sh_builtin_env(t_shell *shell, int argc, char **argv);
int				sh_builtin_true(t_shell *shell, int argc, char **argv);
int				sh_builtin_false(t_shell *shell, int argc, char **argv);

#endif
