#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_SIZE 1024

void prompt_user()
{
    printf("user@host> "); // From `stdio`
}

char *read_input()
{
    char *input = malloc(MAX_INPUT_SIZE); // Allocate memory for the input buffer, `stdlib`
    // CHECK: allocation successful
    if (!input)
    {
        fprintf(stderr, "imcsh: allocation error\n");
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
        fprintf(stderr, "imcsh: input too long\n");
        free(input);
        return NULL;
    }
    // Remove the trailing newline character
    input[strcspn(input, "\n")] = '\0';
    return input;
}

// Define possible functions for the shell to use
void help(char *args)
{
    if (args && strlen(args) > 0)
    {
        fprintf(stderr, "imcsh: 'help' does not accept any arguments\n");
        return;
    }
    printf("Available commands:\n");
    printf("  globalusage - Display global usage information\n");
    printf("  help        - Show this help message\n");
    printf("  echo        - Echos the user's input\n");
    printf("  exit        - Exit the shell\n");
}

void globalusage(char *args)
{
    if (args && strlen(args) > 0)
    {
        fprintf(stderr, "imcsh: 'globalusage' does not accept any arguments\n");
        return;
    }
    printf("Global usage display\n");
}

void echo(char *args)
{
    if (!args)
    {
        fprintf(stderr, "imcsh: 'echo' requires an argument\n");
        return;
    }
    printf("%s\n", args);
}

void exit_shell(char *args)
{
    if (args && strlen(args) > 0)
    {
        fprintf(stderr, "imcsh: 'exit' does not accept any arguments\n");
        return;
    }
    printf("Exiting shell...\n");
    exit(0);
}

// Create a lookup table of the possible functions
typedef void (*function_ptr)(char *);

typedef struct
{
    const char *name;
    function_ptr func;
    int expects_args; // 1 if the function expects arguments, 0 otherwise
} function_entry;

function_entry function_table[] = {
    {"globalusage", globalusage, 0},
    {"help", help, 0},
    {"echo", echo, 1},
    {"exit", exit_shell, 0},
    {NULL, NULL, 0} // Sentinel value to mark the end of the table
};

void handle_input(char *input_str)
{
    char *function;
    char *arguments;

    // Split the input into function and arguments
    function = strtok(input_str, " "); // From `string`
    arguments = strtok(NULL, "");      // Get the rest
    // printf("User wants to use function: %s\n", function);
    // printf("With arguments: %s\n", arguments ? arguments : "None");

    // CHECK: input is not empty
    if (function == NULL)
    {
        return;
    }

    // Iterate through the function table to find a match
    for (int i = 0; function_table[i].name != NULL; i++)
    {
        if (strcmp(function, function_table[i].name) == 0)
        {
            if (function_table[i].expects_args)
            {
                if (arguments == NULL || strlen(arguments) == 0)
                {
                    fprintf(stderr, "imcsh: '%s' requires an argument\n", function_table[i].name);
                    return;
                }
            }
            else
            {
                if (arguments != NULL && strlen(arguments) > 0)
                {
                    fprintf(stderr, "imcsh: '%s' does not accept any arguments\n", function_table[i].name);
                    return;
                }
            }

            // Call the function with the arguments
            function_table[i].func(arguments);
            return;
        }
    }

    fprintf(stderr, "imcsh: invalid function '%s'\n", function);
}

int main()
{
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
    return 0;
}