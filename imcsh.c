#define _XOPEN_SOURCE 700 // This was done to surpress warnings for `sigchild` / `signal`
// Source: https://stackoverflow.com/questions/6491019/struct-sigaction-incomplete-error

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h> // For signal handling
#include <spawn.h>  // For `posix_spawn()` solution
#include <fcntl.h>  // For writing stdout to a file
#include <limits.h> // For HOST_NAME_MAX
#include <pwd.h>    // For getpwuid

#define MAX_INPUT_SIZE 1024
// #define HOST_NAME_MAX 100
#define ANSI_COLOR_GREEN "\x1b[32m" // For coloring the prompt, source: https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c
#define ANSI_COLOR_RED "\x1b[31m"   // For errors
#define ANSI_COLOR_RESET "\x1b[0m"  // Reset color

// Initialize global variables to store username and hostname
// Source: https://stackoverflow.com/questions/1451825/c-programming-printing-current-user
const char *g_username = NULL;
char g_hostname[HOST_NAME_MAX + 1] = {0};

// ------------------------
// |   Helper functions   |
// ------------------------

// Function to set the global hostname and username variables before the shell runs
void initialize_shell()
{
    // Retrieve the passwd structure for the current user
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL)
    {
        perror("getpwuid failed");
        exit(EXIT_FAILURE);
    }

    // Assign username
    g_username = pw->pw_name;

    // Retrieve hostname
    if (gethostname(g_hostname, sizeof(g_hostname)) == -1)
    {
        perror("gethostname failed");
        exit(EXIT_FAILURE);
    }
}

// Function to display the ASCII art title
void display_title()
{
    printf("  _____ __  __  _____  _____ _    _ \n");
    printf(" |_   _|  \\/  |/ ____|/ ____| |  | |\n");
    printf("   | | | \\  / | |    | (___ | |__| |\n");
    printf("   | | | |\\/| | |     \\___ \\|  __  |\n");
    printf("  _| |_| |  | | |____ ____) | |  | |\n");
    printf(" |_____|_|  |_|\\_____|_____/|_|  |_|\n");
    printf(" IMCSH Version 1.1 created by David Fodor\n");
    printf("\n");
}

// Function to prompt user
void prompt_user()
{
    printf(ANSI_COLOR_GREEN "%s@%s> " ANSI_COLOR_RESET, g_username, g_hostname); // From `stdio`
}

char *read_input()
{
    char *input = malloc(MAX_INPUT_SIZE); // Allocate memory for the input buffer, `stdlib`
    // CHECK: allocation successful
    if (!input)
    {
        fprintf(stderr, ANSI_COLOR_RED "imcsh: allocation error\n" ANSI_COLOR_RESET);
        exit(EXIT_FAILURE); // For now simply exit, `stdlib`
    }
    // CHECK: input is not empty
    if (fgets(input, 1024, stdin) == NULL) // From `stdin`
    {
        free(input);
        return NULL;
    }
    // Check that input fit into the allocated space
    // - If newline char is not found in the string it didn't fit into buffer
    if (strchr(input, '\n') == NULL) // From `stringh`
    {
        fprintf(stderr, ANSI_COLOR_RED "imcsh: input too long\n" ANSI_COLOR_RESET);
        free(input);
        return NULL;
    }
    // Remove the trailing newline character
    input[strcspn(input, "\n")] = '\0';
    return input;
}

// Keeping track of running processes
#define INITIAL_MAX_PROCESSES 100

// Declare a pointer to store process IDs
pid_t *running_processes;
size_t num_running_processes = 0;
size_t max_processes = INITIAL_MAX_PROCESSES;

void add_running_process(pid_t pid)
{
    if (num_running_processes >= max_processes)
    {
        // Optionally, resize the array if needed
        max_processes *= 2;
        running_processes = realloc(running_processes, max_processes * sizeof(pid_t));
        if (running_processes == NULL)
        {
            fprintf(stderr, "Memory reallocation failed\n");
            exit(EXIT_FAILURE);
        }
    }
    running_processes[num_running_processes++] = pid;
}

void remove_running_process(pid_t pid)
{
    for (size_t i = 0; i < num_running_processes; ++i)
    {
        if (running_processes[i] == pid)
        {
            // Shift remaining elements to the left
            for (size_t j = i; j < num_running_processes - 1; ++j)
            {
                running_processes[j] = running_processes[j + 1];
            }
            --num_running_processes;
            break;
        }
    }
}

// ------------------------------------------------------
// |   Define possible functions for the shell to use   |
// ------------------------------------------------------
void help(char *args, int background, const char *output_file)
{
    (void)args;
    (void)background;

    if (output_file != NULL)
    {
        FILE *out = fopen(output_file, "a"); // Append mode
        if (out == NULL)
        {
            fprintf(stderr, ANSI_COLOR_RED "imcsh: Failed to open file '%s' for writing\n" ANSI_COLOR_RESET, output_file);
            return;
        }

        fprintf(out, "Available commands:\n");
        fprintf(out, "  globalusage - Display basic information about the shell\n");
        fprintf(out, "  help        - Show this help message\n");
        fprintf(out, "  echo        - Echos the user's input\n");
        fprintf(out, "  quit        - Quit the shell\n");
        fprintf(out, "  exec        - Execute a program like a regular shell would do\n");
        fclose(out);

        printf("Output redirected to -> %s\n", output_file);
    }
    else
    {
        printf("Available commands:\n");
        printf("  globalusage - Display basic information about the shell\n");
        printf("  help        - Show this help message\n");
        printf("  echo        - Echos the user's input\n");
        printf("  quit        - Quit the shell\n");
        printf("  exec        - Execute a program like a regular shell would do\n");
    }
}

void globalusage(char *args, int background, const char *output_file)
{
    (void)args;
    (void)background;

    if (output_file != NULL)
    {
        FILE *out = fopen(output_file, "a"); // Append mode
        if (out == NULL)
        {
            fprintf(stderr, ANSI_COLOR_RED "imcsh: Failed to open file '%s' for writing\n" ANSI_COLOR_RESET, output_file);
            return;
        }

        fprintf(out, "IMCSH Version 1.1 created by David Fodor\n");
        fclose(out);

        printf("Output redirected to -> %s\n", output_file);
    }
    else
    {
        printf("IMCSH Version 1.1 created by David Fodor\n");
    }
}

void echo(char *args, int background, const char *output_file)
{
    (void)background;

    if (output_file != NULL)
    {
        FILE *out = fopen(output_file, "a"); // Append mode
        if (out == NULL)
        {
            fprintf(stderr, ANSI_COLOR_RED "imcsh: Failed to open file '%s' for writing\n" ANSI_COLOR_RESET, output_file);
            return;
        }

        fprintf(out, "%s\n", args);
        fclose(out);

        printf("Output redirected to -> %s\n", output_file);
    }
    else
    {
        printf("%s\n", args);
    }
}

void quit_shell(char *args, int background, const char *output_file)
{
    (void)args;
    (void)background;
    (void)output_file;

    if (num_running_processes > 0)
    {
        printf("The following processes are running:\n");
        for (size_t i = 0; i < num_running_processes; ++i)
        {
            printf("    -> pid: %d\n", running_processes[i]);
        }
    }

    char answer;
    int valid = 0; // Interesting how C didn't have bool in it's original form

    while (!valid)
    {
        printf("Are you sure you want to quit? [Y/n]: ");

        // Attempt to read a single character input
        int result = scanf(" %c", &answer); // The space before %c skips any leading whitespace

        if (result == 1) // If successfull `scanf` returns 1
        {
            if (answer == 'y' || answer == 'Y' || answer == 'n' || answer == 'N')
            {
                valid = 1; // Valid input received
            }
            else
            {
                printf("Invalid input. Please enter 'Y' or 'n'.\n\n");
            }
        }
        else
        {
            printf("Invalid input. Please enter 'Y' or 'n'.\n\n");
        }

        // Clear the input buffer to remove any remaining characters
        // This prevents leftover characters from being read in the next iteration
        while (getchar() != '\n' && getchar() != EOF)
            ;
    }

    if (answer == 'n' || answer == 'N')
    {
        return;
    }
    else
    {
        // Loop through all the remaining children, killing them to not get orphan processes
        // Source: https://stackoverflow.com/questions/6501522/how-to-kill-a-child-process-by-the-parent-process
        if (num_running_processes > 0)
        {
            // printf("The following processes are running:\n");
            for (size_t i = 0; i < num_running_processes; ++i)
            {
                printf("Killing process with pid: %d...\n", running_processes[i]);
                kill(running_processes[i], SIGKILL);
            }
        }
    }

    printf("Quitting shell...\n");
    exit(0);
}

void execute_program(char *args, int background, const char *output_file)
{
    // Tokenize the args to create argv array of the function we actually want to execute
    int bufsize = 64;
    int position = 0;
    char **argv = malloc(bufsize * sizeof(char *));
    if (!argv)
    {
        fprintf(stderr, ANSI_COLOR_RED "imcsh: allocation error\n" ANSI_COLOR_RESET);
        exit(EXIT_FAILURE);
    }

    char *token = strtok(args, " ");
    while (token != NULL)
    {
        argv[position++] = token;

        if (position >= bufsize) // Dynamically add to the buffer size if it were exceeded
        {
            bufsize += 64;
            argv = realloc(argv, bufsize * sizeof(char *));
            if (!argv)
            {
                fprintf(stderr, ANSI_COLOR_RED "imcsh: allocation error\n" ANSI_COLOR_RESET);
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, " ");
    }
    argv[position] = NULL; // Null-terminate the argument list - expected by both `execvp` and `posix-spawn`

    // ---------------
    // SOLUTION 1:
    // Fork a child process and run the function via execvp, that replaces the processes to be run
    // Source: https://stackoverflow.com/questions/5460421/how-do-you-write-a-c-program-to-execute-another-program
    // pid_t pid = fork();
    // if (pid < 0)
    // {
    //     // Forking error
    //     perror("imcsh");
    //     free(argv);
    //     return;
    // }
    // else if (pid == 0)
    // {
    //     // Child process
    //     if (execvp(argv[0], argv) == -1)
    // ...
    // ...
    // ...
    // ---------------

    // ---------------
    // SOLUTION 2:
    // While not as common, I have found the `posix_spawn()` function that works similarly to how Windows handles child processes
    // - apparently this may be more memory efficient as well, as this does not copy the memory of the parent process
    // - also a bit less complex, no need for two separate calls - PID checks and such
    // - disadvantage is the loss of customizability, however this is not an issue for us as we are just executing
    // Source: https://buffaloitservices.com/an-in-depth-look-at-child-process-creation-in-c
    // Writing to file: https://unix.stackexchange.com/questions/252901/get-output-of-posix-spawn
    // Docs: https://man7.org/linux/man-pages/man3/posix_spawn.3.html

    // Variables for posix_spawn
    pid_t pid;
    // Initialize file actions - these behind the scenes use the usual `open()`, `close()`, `dup2()` functions
    posix_spawn_file_actions_t file_actions;
    posix_spawn_file_actions_init(&file_actions);

    // If output redirection is specified, add file actions to redirect stdout
    if (output_file != NULL)
    {
        int fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1)
        {
            fprintf(stderr, ANSI_COLOR_RED "imcsh: Failed to open file '%s' for writing\n" ANSI_COLOR_RESET, output_file);
            posix_spawn_file_actions_destroy(&file_actions);
            free(argv);
            return;
        }

        // Redirect stdout to the file
        posix_spawn_file_actions_adddup2(&file_actions, fd, STDOUT_FILENO); // Redirect the stdout to the fd
        posix_spawn_file_actions_addclose(&file_actions, fd);               // Close the file
    }

    // Spawn the new process
    int status = posix_spawnp(&pid, argv[0], (output_file != NULL) ? &file_actions : NULL, NULL, argv, NULL); // `spawnp` variant to access PATH variables
    if (status == 0)
    {
        // If background process just start
        if (background)
        {
            printf("Started process with PID %d\n", pid);
            add_running_process(pid);
        }
        else // Wait for the child process to finish
        {
            if (waitpid(pid, &status, 0) == -1)
            {
                perror("imcsh");
            }
            else
            {
                if (WIFEXITED(status))
                {
                    printf("Process %d terminated with exit status %d\n", pid, WEXITSTATUS(status));
                }
                else if (WIFSIGNALED(status))
                {
                    printf("Process %d terminated due to signal %d\n", pid, WTERMSIG(status));
                }
            }
        }
    }
    else
    {
        fprintf(stderr, ANSI_COLOR_RED "imcsh: posix_spawn error: %s\n" ANSI_COLOR_RESET, strerror(status));
    }

    // Clean up
    posix_spawn_file_actions_destroy(&file_actions);
    free(argv); // Free the allocated sapce for arguments
}

// Create a lookup table of the possible functions
// (It could have been made a hash map for efficiency, but looping should be fine for this few options)
typedef void (*function_ptr)(char *args, int background, const char *output_file);
typedef struct
{
    const char *name;
    function_ptr func;
    int expects_args;        // 1 if the function expects arguments, 0 otherwise
    int supports_background; // 1 if the function supports background execution, 0 otherwise
    int supports_output;     // 1 if the function supports output redirection, 0 otherwise
} function_entry;

function_entry function_table[] = {
    {"globalusage", globalusage, 0, 0, 1},
    {"help", help, 0, 0, 1},
    {"echo", echo, 1, 0, 1},
    {"quit", quit_shell, 0, 0, 0},
    {"exec", execute_program, 1, 1, 1},
    {NULL, NULL, 0, 0, 0} // Sentinel value to mark the end of the table
};

// ---------------------
// |   Input handler   |
// ---------------------
void handle_input(char *input_str)
{
    char *function;
    char *arguments;
    char *output_file = NULL;
    int background = 0;

    // Split the input into function and arguments
    function = strtok(input_str, " "); // the function for the `imcsh` shell to run, from `string`
    arguments = strtok(NULL, "");      // Get the rest
    // printf("User wants to use function: %s\n", function);
    // printf("With arguments: %s\n", arguments ? arguments : "None");

    // CHECK: input is not empty
    if (function == NULL)
    {
        return;
    }

    // CHECK: for modifiers if there are caught arguments
    if (arguments != NULL)
    {
        // Check for background execution '&' at the end
        size_t len = strlen(arguments);
        if (len > 0 && arguments[len - 1] == '&')
        {
            background = 1;
            arguments[len - 1] = '\0'; // Remove '&'

            // Remove any trailing whitespace
            while (len > 1 && (arguments[len - 2] == ' ' || arguments[len - 2] == '\t'))
            {
                arguments[len - 2] = '\0';
                len--;
            }
        }

        // Check for file output modifier `>`
        char *redir = strchr(arguments, '>');
        if (redir != NULL)
        {
            *redir = '\0'; // Split the string at '>'
            redir++;       // Move past '>'

            // Skip any whitespace
            while (*redir == ' ' || *redir == '\t')
                redir++;

            if (*redir != '\0')
            {
                output_file = strdup(redir); // Duplicate the output file name
            }
            else
            {
                fprintf(stderr, ANSI_COLOR_RED "imcsh: No output file specified after '>'\n" ANSI_COLOR_RESET);
                return;
            }
        }
    }

    // Iterate through the function table to find a match
    for (int i = 0; function_table[i].name != NULL; i++)
    {
        if (strcmp(function, function_table[i].name) == 0)
        {
            // CHECK: There are arguments given to functions that require it
            if (function_table[i].expects_args)
            {
                if (arguments == NULL || strlen(arguments) == 0)
                {
                    fprintf(stderr, ANSI_COLOR_RED "imcsh: '%s' requires arguments to run\n" ANSI_COLOR_RESET, function_table[i].name);
                    return;
                }
            }
            else
            {
                if (arguments != NULL && strlen(arguments) > 0)
                {
                    fprintf(stderr, ANSI_COLOR_RED "imcsh: '%s' does not accept any arguments\n" ANSI_COLOR_RESET, function_table[i].name);
                    return;
                }
            }

            // CHECK: That function can be run in the background if needed
            if (!function_table[i].supports_background && background)
            {
                fprintf(stderr, ANSI_COLOR_RED "imcsh: '%s' does not support background execution\n" ANSI_COLOR_RESET, function_table[i].name);
                free(output_file);
                return;
            }

            // CHECK: That output can be caught into a file if needed
            if (!function_table[i].supports_output && output_file != NULL)
            {
                fprintf(stderr, ANSI_COLOR_RED "imcsh: '%s' does not support output redirection\n" ANSI_COLOR_RESET, function_table[i].name);
                free(output_file);
                return;
            }

            // Call the function with the arguments and background flag
            function_table[i].func(arguments, background, output_file);

            return;
        }
    }

    // If not found in `function_table` print error
    fprintf(stderr, ANSI_COLOR_RED "imcsh: invalid function '%s'\n" ANSI_COLOR_RESET, function);
}

// ----------------------
// |   Signal handler   |
// ----------------------
// Signal handler for SIGCHLD to reap background processes
void sigchld_handler(int sig)
{
    (void)sig;               // Marked as unused to suppress warning
    int saved_errno = errno; // Save errno, as it might be modified - interfering with main program's error state
    pid_t pid;               // Init storage of terminated PID
    int status;              // Init storage of status of the terminated process

    // Loop to reap all terminated child processes - avoinding zombies
    // - loop is apparently used because SIGCHIL signal might indicate more than 1 terminated children
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) // -1 = we wait for any process, WHOHANG means there is no blocking if no zombie child process found
    {
        if (WIFEXITED(status))
        {
            printf("\nBackground process %d terminated with exit status %d\n", pid, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("\nBackground process %d terminated due to signal %d\n", pid, WTERMSIG(status));
        }
        remove_running_process(pid);
        prompt_user();
        fflush(stdout); // Ensure the message is printed immediately - disregarding current input wait
    }
    errno = saved_errno; // Restore errno
}

// -----------------
// |   Main loop   |
// -----------------
int main()
{
    // Initialize shell - set username and host as global variables
    initialize_shell();
    display_title();

    // Set up the SIGCHLD handler to handle background process termination
    // - Neccessary because if we are simply forking processes and and don't wait for them and reap them, we create zombie processes
    // - Possible sources: https://docs.oracle.com/cd/E19455-01/806-4750/signals-7/index.html
    // - GNU docs: https://www.gnu.org/software/libc/manual/html_node/Signal-Handling.html
    struct sigaction sa;
    sa.sa_handler = &sigchld_handler;        // Handler to call when subprocess finishes
    sigemptyset(&sa.sa_mask);                // Initialize the signal set to exclude all signals
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; // Flags to restart interrupted system calls and ignore stopped children
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("imcsh: sigaction");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for storing the running processes
    running_processes = malloc(max_processes * sizeof(pid_t));
    if (running_processes == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Main loop
    while (1)
    {
        prompt_user();
        char *input = read_input();
        if (input == NULL)
        {
            // EOF encountered (e.g., Ctrl+D)
            printf("\n");
            break;
        }
        handle_input(input);
        free(input); // Free the allocated memory
    }

    // Clean up
    free(running_processes);
    return 0;
}