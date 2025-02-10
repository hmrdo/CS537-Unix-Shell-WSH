#include "wsh.h"

// Allocate memory for polling input from stdin to line, and tokenizing arguments
char line[MAXLINE]; 
char *args[MAXARGS]; 
char **history_list;
int history_list_size = 5; // Originally 5

// Local var struct for shell variables
typedef struct LocalVar 
{
    char *name;
    char *value;
} LocalVar;

LocalVar *local_variables;
int num_local_variables = 0;

// return variable, error is -1
int return_var = 0;

void free_memory() {
    // Check if local_variables is allocated
    if (local_variables != NULL) 
    {
        // Free local variables
        for (int i = 0; i < num_local_variables; i++) 
        {
            // Free the name if it's not NULL
            if (local_variables[i].name != NULL) 
            {
                free(local_variables[i].name);  // Free name
            }
            // Free the value if it's not NULL
            if (local_variables[i].value != NULL) 
            {
                free(local_variables[i].value); // Free value
            }
        }
        // Free the array of LocalVar structs
        free(local_variables);
        local_variables = NULL; // Avoid dangling pointer
    }
    
    // Free history list
    for (int i = 0; i < history_list_size; i++) if (history_list[i] != NULL) free(history_list[i]); // Free each history item
    if (history_list != NULL) free(history_list); // Free the history list array
    history_list = NULL; // Avoid dangling pointer
    
    // Free args
    if (args != NULL)
    {
        for (int i = 0; i < MAXARGS; i++) 
        {
            if (args[i] != NULL) 
            {
                args[i] = NULL; // Avoid dangling pointer
            }
        }
    }
    
}

/*
 * This function checks for and substitutes variable values whenever a command is passed $
 */
char* substitute_var(char *arg)
{
    // No variable substitution
    if (arg[0] != '$') return arg;

    char* var_name = arg + 1;

    // Check if an environment variable
    char* env_value = getenv(var_name);
    if (env_value != NULL) return env_value;

    // Check if local variable
    for (int i = 0; i < num_local_variables; i++)
    if (strcmp(local_variables[i].name, var_name) == 0) return local_variables[i].value;

    // If it doesn't exist, return blank
    return "";
}

/*
 * When the user types exit, your shell should simply call the exit system call with 0 as a parameter. 
 * It is an error to pass any arguments to exit.
 */
void wsh_exit(char **args)
{
    // exit should be called with zero args
    if (args != NULL && args[1] != NULL)
    {
        perror("Exit");
        return_var = -1;
    } 
    else 
    {
        free_memory();
        exit(return_var);
    }
}

/*
 * cd always take one argument (0 or >1 args should be signaled as an error). 
 * To change directories, use the chdir() system call with the argument supplied by the user; 
 * if chdir fails, that is also an error.
 */
void wsh_cd(char **args)
{
    if (args[1] == NULL)
    {
        perror("cd");
        return_var = -1;
    }
    else if (chdir(args[1]) == -1)
    {
        perror("chdir");
        return_var = -1;
    } 

    return_var = 0;
}

/*
 * Used as export VAR=<value> to create or assign variable VAR as an environment variable.
 */
void wsh_export(char **args)
{
    // Invalid use of command
    if (args[1] == NULL)
     {
        perror("export");
        return_var = -1;
     }

    char *var_name = strtok(args[1], "=");
    char *var_assignment = strtok(NULL, "=");

    // double check
    if (var_assignment == NULL) var_assignment = "";

    setenv(var_name, var_assignment, 1);

    return_var = 0;
}

/*
 * Used as local VAR=<value> to create or assign variable VAR as a shell variable. 
 */
void wsh_local(char **args)
{
    // Delimit arg[1] by = to get the variable and the assignment
    char *var_name = strtok(args[1], "=");
    char *var_assignment = strtok(NULL, "=");

    // NULL assignment clears var
    if (var_assignment == NULL) var_assignment = "";

    // Check if the variable already exists and update or remove it
    for (int i = 0; i < num_local_variables; i++) {
        if (strcmp(local_variables[i].name, var_name) == 0) 
        {
            // If value is empty, clear the variable
            if (strcmp(var_assignment, "") == 0) 
            {
                free(local_variables[i].name);
                free(local_variables[i].value);

                // Shift the remaining variables up by one
                for (int j = i; j < num_local_variables - 1; j++) 
                {
                    local_variables[j] = local_variables[j + 1];
                }

                // Decrease the count of local variables
                num_local_variables--;

                // Resize the array to free up unused memory
                local_variables = realloc(local_variables, num_local_variables * sizeof(LocalVar));
                if (local_variables == NULL && num_local_variables > 0) 
                {
                    perror("realloc");  // Memory allocation failed
                    return_var = -1;
                }

                return;  // Variable removed, so exit
            } 
            else 
            {
                // Update existing value
                free(local_variables[i].value);
                local_variables[i].value = strdup(var_assignment);
                return_var = 0;
                return;
            }
        }
    }

    // Add new variable if not found
    local_variables = realloc(local_variables, (num_local_variables + 1) * sizeof(LocalVar));
    if (local_variables == NULL)
    {
        perror("realloc");
        return_var = -1;
    } 

    // Duplicate string to list with strdup
    local_variables[num_local_variables].name = strdup(var_name);
    local_variables[num_local_variables].value = strdup(var_assignment);
    num_local_variables++;
}
/*
 * Vars will print all of the local variables and their values in the format <var>=<value>, 
 * one variable per line. Variables should be pranted in insertion order, 
 * with the most recently created variables printing last.
 * Updates to existing variables will modify them in-place in the variable list,
 * without moving them around in the list.
 */
void wsh_vars()
{
    for (int i = 0; i < num_local_variables; i++)
    {
        printf("%s=%s\n", local_variables[i].name, local_variables[i].value);
    }
    return_var = 0;
}
/*
 * Keeps track of the last 5 commands, history shows the history list. 
 * Commands executeed more than once consecutively appear in the stored list once.
 * Most recent command is #1. Builtin commands should not be stored in history
 * Length of history is configurable, using history set <n>, where n is an integer.
 * If there are fewer commands in the history than its capacity, simply print the commands that are stored 
 * If a larger history is shrunk using history set, drop the commands which no longer fit into the history
 * To execute a command from the history, use history <n>, where n is the nth command in history.
 */
void wsh_history(char **args) 
{
    // Display history
    if (args[1] == NULL) 
    {
        for (int i = 0; i < history_list_size; i++) 
        {
            if (history_list[i] != NULL) 
            {
                printf("%d) %s\n", i + 1, history_list[i]);
            }
        }
    }
    // "history set <n>" changes length of history list
    else if (args[1] != NULL && strcmp(args[1], "set") == 0 && args[2] != NULL && isdigit(args[2][0])) {
        // Acquire size variable, and turn string to int for comparison
        int new_size = atoi(args[2]);
        if (new_size > 0) 
        {
            // If shrinking the history list
            if (new_size < history_list_size) 
            {
                // Free the entries that are beyond the new size
                for (int i = new_size; i < history_list_size; i++) 
                {
                    if (history_list[i] != NULL) 
                    {
                        free(history_list[i]);
                    }
                }
            }

            // Adjust the history list size with realloc
            history_list = realloc(history_list, new_size * sizeof(char *));
            if (history_list == NULL) 
            {
                perror("realloc");
                return_var = -1;
                return;
            }

            // Initialize new entries to NULL if expanding
            if (new_size > history_list_size) 
            {
                for (int i = history_list_size; i < new_size; i++) 
                {
                     history_list[i] = NULL;
                }
            }

        // Update the history list size to the new size
        history_list_size = new_size;
        } 
        else 
        {
            perror("Invalid history size");
            return_var = -1;
        }
    }

    // Otherwise if history <n>, access the nth command
    else if (isdigit(args[1][0])) 
    {   
        // Execute command from that index given
        int index = atoi(args[1]) - 1;  // Convert argument to int, subtract 1 to match 0-indexed array

        // Check if index is within bounds and there's a command in the history
        if (index >= 0 && index < history_list_size && history_list[index] != NULL)
        {
            // Copy command from history using strdup
            char *command = strdup(history_list[index]);
            if (command == NULL) 
            {
                perror("strdup");
                return_var = -1;
                return;
            }

            // Create a temporary args array to avoid overwriting the original args
            char *temp_args[MAXARGS];
            char *token = strtok(command, " ");
            int i = 0;

            // Reset temp_args and fill it with the tokens
            while (token != NULL)
            {
                temp_args[i++] = token;
                token = strtok(NULL, " ");
            }
            temp_args[i] = NULL; // Null terminate the temp_args array

            // Execute the command using execute_commands without recording in history
            execute_commands(temp_args, NULL, 1);

            // Free the duplicated command string
            free(command);
        }
        else 
        {
            perror("No command at provided index");
            return_var = -1;
        }
}


    return_var = 0;
}

/*
 * ls: Produces the same output as LANG=C ls -1, 
 * however you cannot spawn ls program because this is a built-in. 
 * This built-in does not implement any parameters.
 */
void wsh_ls() {
    DIR *dir;                   // Pointer to a directory stream
    struct dirent *entry;        // Pointer to a directory entry structure
    char **file_list = NULL;     // Array of strings to hold file names
    int file_count = 0;          // Number of files in the directory
    int max_files = 10;          // Initial size for dynamic array

    // Open the current directory (".")
    dir = opendir(".");

    if (dir == NULL) 
    {
        perror("opendir");
        return_var = -1;
        return;
    }

    // Allocate memory for the file list
    file_list = malloc(max_files * sizeof(char *));
    if (file_list == NULL) 
    {
        perror("malloc");
        // Close directory, else memory leak
        closedir(dir);
        return_var = -1;
        return;
    }

    // Read directory entries one by one using readdir
    while ((entry = readdir(dir)) != NULL) // readdir returns pointer to next directory entry
    {
        // Skip hidden files or directories
        if (entry->d_name[0] == '.') continue;

        // Check if we need to resize the file_list array
        if (file_count >= max_files) 
        {
            max_files = max_files * 2; // Double the size of the array

            // Realloc list with new size
            file_list = realloc(file_list, max_files * sizeof(char *));
            if (file_list == NULL) 
            {
                perror("realloc");
                closedir(dir);
                return_var = -1;
                return;
            }
        }

        // Store the file name by copying it from directory entry with strdup
        file_list[file_count] = strdup(entry->d_name);  
        if (file_list[file_count] == NULL) 
        {
            perror("strdup");
            closedir(dir);
            return_var = -1;
            return;
        }
        file_count++;
    }

    // Close the directory
    closedir(dir);

    // Sort file names by alphabet
    // Bubble sort??? IDK, from stackoverflow
    for (int i = 0; i < file_count - 1; i++) {
        for (int j = 0; j < file_count - i - 1; j++) {
            if (strcmp(file_list[j], file_list[j + 1]) > 0) 
            {
                // Swap the file names
                char *temp = file_list[j];
                file_list[j] = file_list[j + 1];
                file_list[j + 1] = temp;
            }
        }
    }

    // Print the list
    for (int i = 0; i < file_count; i++) 
    {
        printf("%s\n", file_list[i]);
        free(file_list[i]);  // Free after printing
    }

    // Free array when dnone
    free(file_list);

    return_var = 0;
}
   

void execute_commands(char **args, char *original_line, int from_history)
{
    // Check for redirections in commands //
    // For file redirections
    FILE *input;
    FILE *output;
    // Save stdin, stdout, and stderr for restore after commands sent
    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);
    int saved_stderr = dup(STDERR_FILENO);
    int i = 0;
    int cmd_executed = 0;
    int fd;

    while(args[i] != NULL)
    {
        if (strstr(args[i], "<") != NULL) 
        {
            fd = STDIN_FILENO;  // Default to stdin
            char *filename;

            // Check if there's a digit before '<'
            if (args[i][0] >= '0' && args[i][0] <= '9' && args[i][1] == '<') 
            {
                fd = args[i][0] - '0';  // Use the digit before '<' as file descriptor
                filename = strtok(args[i] + 2, " ");  // Get the filename after '<'
            } 
            else 
            {
                filename = strtok(args[i], "<");  // Get the filename after '<'
            }

            // Open the file for reading
            input = fopen(filename, "r");
            if (input != NULL) 
            {
                dup2(fileno(input), fd);  // Duplicate the file descriptor to redirect input
                fclose(input);
                args[i] = NULL;
            }
            else 
            {
                perror("fopen");
                return_var = -1;
            }
        }
        // Redirect output and append to file and error
        else if (strstr(args[i], "&>>") != NULL)
        {
            output = fopen(strtok(args[i], "&>>"), "a");
            dup2(fileno(output), STDOUT_FILENO);
            dup2(fileno(output), STDERR_FILENO);
            fclose(output); 
            args[i] = NULL;
        }
        // Redirect output and append to end of file
        else if (strstr(args[i], ">>") != NULL) 
        {
            int fd = STDOUT_FILENO;  // Default to stdout
            char *filename;

            // Check if there's a digit before '>>'
            if (args[i][0] >= '0' && args[i][0] <= '9' && args[i][1] == '>' && args[i][2] == '>') 
            {
                fd = args[i][0] - '0';  // Use the digit before '>>' as file descriptor
                filename = strtok(args[i] + 3, " ");  // Get the filename after '>>'
            } 
            else 
            {
                filename = strtok(args[i], ">>");  // Get the filename after '>>'
            }

            // Open the file for appending
            output = fopen(filename, "a");
            if (output != NULL) 
            {
                dup2(fileno(output), fd);  // Duplicate the file descriptor to redirect output
                fclose(output);
                args[i] = NULL;
            } 
            else 
            {
                perror("fopen");
                return_var = -1;
            }
        }
        // Redirect output and error 
        else if (strstr(args[i], "&>") != NULL)
        {
            output = fopen(strtok(args[i], "&>"), "w");
            dup2(fileno(output), STDOUT_FILENO);
            dup2(fileno(output), STDERR_FILENO);
            fclose(output);
            args[i] = NULL; 
        }
        else if (strstr(args[i], ">") != NULL) 
        {
            int fd = STDOUT_FILENO;  // Default to stdout
            char *filename;
    
            // Check if there's a digit before '>'
            if (args[i][0] >= '0' && args[i][0] <= '9' && args[i][1] == '>') 
            {
                fd = args[i][0] - '0';  // Use the digit before '>' as file descriptor
                filename = strtok(args[i] + 2, " ");  // Get the filename after '>'
            } 
            else 
            {
                filename = strtok(args[i], ">");  // Get the filename after '>'
            }

            // Open the file for writing
            output = fopen(filename, "w");
            if (output != NULL) 
            {
                dup2(fileno(output), fd);  // Duplicate the file descriptor to redirect output
                fclose(output);
                args[i] = NULL;
            } 
    
            else 
            {
                perror("fopen");
                return_var = -1;
            }
        }
        i++;
    }

    // BUILT-IN PROCESSING //
    if (strcmp(args[0], "exit") == 0)
    {
        wsh_exit(args);
        cmd_executed = 1;
    }
    else if (strcmp(args[0], "cd") == 0)
    {
        wsh_cd(args);
        cmd_executed = 1;
    } 
    else if (strcmp(args[0], "export") == 0)
    {
        wsh_export(args);
        cmd_executed = 1;
    }
    else if (strcmp(args[0], "local") == 0)
    {
         wsh_local(args);
         cmd_executed = 1;
    }
    else if (strcmp(args[0], "vars") == 0)
    {
        wsh_vars();
        cmd_executed = 1;
    }
    else if (strcmp(args[0], "history") == 0)
    {
        wsh_history(args);
        cmd_executed = 1;
    } 
    else if (strcmp(args[0], "ls") == 0)
    {
        wsh_ls(args);
        cmd_executed = 1;
    }

    // Relative / Full path check
    else if (access(args[0], X_OK) == 0)
    {
        // Fork the new process
        pid_t pid = fork();
            
        // Check for failed fork
        if(pid < 0)
        {
            perror("fork");
            return_var = -1;
        }

        // Fork successful, check if executable can be executed
        if (pid == 0) if (execv(args[0], args) == -1)
        {
            perror("execv");
            return_var = -1;
        } 

        cmd_executed = 1;
        // Wait for child(executable) to finish
        int status;
        if (waitpid(pid, &status, 0) >0)
        {
            if (WIFEXITED(status))
            { 
                return_var = WEXITSTATUS(status);
            }
            else return_var = -1;
        }
        else
        {
            perror("waitpid");
            return_var = -1;
        }
    }

    // PATHS SPECIFIED BY $PATH //
    else
    {
        // Get path
        char *path = getenv("PATH");
        char *path_copy = strdup(path); // copy to modify
        char *all_paths = strtok(path_copy, ":");    // find all paths in PATH variable, delimited by :

        while (all_paths != NULL)
        {
            // attach command to path
            char full_path[MAXLINE];
            snprintf(full_path, sizeof(full_path), "%s/%s", all_paths, args[0]);

            // Check if the command exists and is an executable
            if (access(full_path, X_OK) == 0)
            {
                // Fork the new process
                pid_t pid = fork();

                // Check for failed fork
                if (pid < 0) 
                {
                    perror("fork");
                    return_var = -1;
                }

                // Fork successful, check if executable can be executed
                if (pid == 0) if (execv(full_path, args) == -1) 
                {
                    perror("execv");
                    return_var = -1;
                } 

                cmd_executed = 1;
                // Wait for child(executable) to finish
                int status;
                if (waitpid(pid, &status, 0) >0)
                {
                    if (WIFEXITED(status))
                    { 
                       if(WEXITSTATUS(status) == 0) return_var = 0;
                    }       
                    else return_var = -1;
                }
                else
                {
                    perror("waitpid");
                    return_var = -1;
                }
            }

            // Otherwise, check next location
            all_paths = strtok(NULL, ":");
        }

        // free strdup
        free(path_copy);
    }

    // Store the original command in history, if not executed from history
    if (args[0] != NULL && from_history == 0 && cmd_executed == 1) 
    {
        if (!(strcmp(args[0], "exit") == 0) &&
            !(strcmp(args[0], "cd") == 0) &&
            !(strcmp(args[0], "export") == 0) &&
            !(strcmp(args[0], "local") == 0) &&
            !(strcmp(args[0], "vars") == 0) &&
            !(strcmp(args[0], "history") == 0) &&
            !(strcmp(args[0], "ls") == 0)) {

            // Add the original command line to history
            if (history_list[0] == NULL || strcmp(history_list[0], original_line) != 0) 
            {
                // Shift history list to make room for the new command
                if (history_list[history_list_size - 1] != NULL) 
                {
                    free(history_list[history_list_size - 1]);
                }
                for (int j = history_list_size - 1; j > 0; j--) 
                {
                    history_list[j] = history_list[j - 1];
                }
                // Add new command to history
                history_list[0] = strdup(original_line);
            }
        }
    }

    // Restore stdout and stdin
    dup2(saved_stdin, STDIN_FILENO);
    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stdin);
    close(saved_stdout);
    close(saved_stderr);

    if (cmd_executed == 0)
    {
        perror("Invalid command");
        return_var = -1;
    }
}


void interactive_shell()
{
    // Allocate memory for polling from stdin to line, and tokenizing arguments
    char *token;
    char original_line[MAXLINE];

    // Interactive 
    while (1)
    {   
        // If stdout is redirected and terminal is still stdiin, then print out shell prompt
        if (!isatty(STDOUT_FILENO) && isatty(STDIN_FILENO))
        {
            FILE *terminal = fopen("/dev/tty", "w");
            fprintf(terminal, "wsh> ");
            fclose(terminal);
        }
        

        printf("wsh> ");
        if (fgets(line, MAXLINE, stdin) != NULL)
        {  
            // Forces output buffer to be fed immediately (issue earlier)
            fflush(stdout);
        }

        // Reading from other input (not terminal), check if EOF
        else if (!isatty(STDIN_FILENO))
        {
            if (feof(stdin)) break;
        }
        
        // Check for empty input or comments
        if (line[0] == '\0' || line[0] == '#') continue;
       
        // Otherwise, tokenize input, check for command, and store args, delimited by spaces
        line[strcspn(line, "\n")] = 0;  // remove null terminating character

        // Store original line for history
        strcpy(original_line, line); 

        token = strtok(line, " ");

        // Reset arg index
        int i = 0;
    
        while (token != NULL)
        {
            args[i++] = substitute_var(token);
            token = strtok(NULL, " ");
        }

        args[i] = NULL; // null terminate args after tokenizing

        // Check for empty line
        if (args[0] == NULL) continue;

        // FREE TOKEN
        free(token);

        // Ensure that there are commands to execute
        if (args[0] != NULL) 
        {
            execute_commands(args, original_line, 0);
        }
    }
}

int bash_shell(int argc, char *argv[]) 
{
    // Open the script file
    FILE *input = fopen(argv[argc-1], "r");

    if (input == NULL) 
    {
        perror("fopen");
        return 0;
    }

    // Allocate memory for polling input from stdin to line
    char line[MAXLINE];
    char original_line[MAXLINE];

    // Iterate through the file until EOF (fgets returns NULL)
    while (fgets(line, MAXLINE, input) != NULL) 
    {
        // Remove newline character if present
        line[strcspn(line, "\n")] = 0;

        // Store original line for history
        strcpy(original_line, line); 
        
        // Ignore comments
        if (line[0] == '#') 
        {
            continue;  // Skip comments
        }

        // Check for empty input
        if (line[0] == '\0') continue;

        // Tokenize the input line and store args
        char *token = strtok(line, " ");
        int i = 0;

        while (token != NULL) 
        {
            args[i++] = substitute_var(token);  // Variable substitution
            token = strtok(NULL, " ");
        }

        args[i] = NULL; // Null-terminate args after tokenizing

        // Check for empty line
        if (args[0] == NULL) continue;
        
        // FREE TOKEN
        free(token);

        // Ensure that there are commands to execute
        if (args[0] != NULL) 
        {   
            // Close input on exit
            if (strcmp(args[0], "exit") == 0) fclose(input);
            execute_commands(args, original_line, 0);
        }
    }

    // Close the script file
    fclose(input);

    // To keep track on if bash ran or not
    return 1;
}


int main(int argc, char *argv[])
{ 
    // Initialize the history list
    history_list = malloc(history_list_size * sizeof(char *));
    if (history_list == NULL) 
    {
        perror("malloc");
        exit(-1);
    }
    for (int i = 0; i < history_list_size; i++) {
        history_list[i] = NULL;
    }

    // Set initial path
    setenv("PATH", "/bin", 1);  // This sets the PATH to only include /bin

    // For file redirections
    FILE *input;
    FILE *output;
    int i = 0;
    int fd = 0;

    if (argv[argc-1] != NULL)
    {
        // Redirect input from while
        if (strstr(argv[argc-1], "<") != NULL)
        {   
            fd = STDIN_FILENO;
            while (argv[argc-1][i] != 0)
            {
                if (argv[argc-1][i] == '<')
                {
                    if (isdigit(argv[argc-1][i-1]) >= 0)
                    {
                        // User inputted FD, 0, 1, 2, etc.
                        fd = argv[argc-1][i-1] - '0';
                    }
                }
            }

            input = fopen(strtok(argv[argc-1], "<"), "r");
            dup2(fileno(input), fd);
            fclose(input); 
        }
        // Redirect output and append to file and error
        else if (strstr(argv[argc-1], "&>>") != NULL)
        {
            output = fopen(strtok(argv[argc-1], "&>>"), "a");
            dup2(fileno(output), STDOUT_FILENO);
            dup2(fileno(output), STDERR_FILENO);
            fclose(output);
        }
        // Redirect output to a file
        else if (strstr(argv[argc-1], ">>") != NULL)
        {
            fd = STDOUT_FILENO;
            char *ptr = strstr(argv[argc-1], ">>");
            if (isdigit(*(ptr-1)) >= 0)
            {
                fd = *(ptr-1) - '0';
            }

            output = fopen(strtok(argv[argc-1], ">>"), "a");
            dup2(fileno(output), fd);
            fclose(output);
            free(ptr);
        }
        // Redirect output and error 
        else if (strstr(argv[argc-1], "&>") != NULL)
        {
            output = fopen(strtok(argv[argc-1], "&>"), "w");
            dup2(fileno(output), STDOUT_FILENO);
            dup2(fileno(output), STDERR_FILENO);
            fclose(output);
        }
        // Redirect output to a file
        else if (strstr(argv[argc-1], ">") != NULL)
        {
            fd = STDOUT_FILENO;
            while (argv[argc-1][i] != 0)
            {
                if (argv[argc-1][i] == '>')
                {
                    if (isdigit(argv[argc-1][i-1]))
                    {
                        // User inputted FD, 0, 1, 2, etc.
                        fd = argv[argc-1][i-1] - '0';
                    }
                }
            }
            output = fopen(strtok(argv[argc-1], ">"), "w"); 
            dup2(fileno(output), fd);
            fclose(output);
        }

        // If there's an argc > 1, it's in bash mode
        else if (argc > 1) 
        {   
            if (bash_shell(argc, argv) == 1)
            {
                free_memory();
                return return_var;
            }
        }
    }

    // Otherwise, interactive mode with redirections or not
    interactive_shell();
    free_memory();
    
    return return_var;
}
