#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // for fork, execv
#include <sys/wait.h> // for waitpid
#include <dirent.h>  // For directory operations
#include <errno.h>   // For error handling
#include <ctype.h> // For isDigit

#define MAXLINE 1024
#define MAXARGS 128

void wsh_exit(char **args);
void wsh_cd(char **args);
void wsh_export();
void ws_local(char **args);
void ws_vars();
void ws_history();
void ws_ls();
void ws_history(char **args);
void interactive_shell();
int bash_shell(int argc, char *argv[]);
void execute_commands(char **args, char *original_line, int from_history);