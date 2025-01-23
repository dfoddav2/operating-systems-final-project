# David Fodor - operating-systems-final-project
The final project solution for WS24/25 Operating Systems course. IMCSH, an alternative Linux shell written in C.

## How to run
Simply compile the `imcsh.c` file via your desired method, then run `./imcsh` in the terminal.

To compile you may use the included [makefile](./makefile) by running `make`. Additionally `make clean` deletes the created binary and .o files and `make run` can be used as a shorthand for `./imcsh`.

Once you have started the application, you should see a "custom shell" appear in your terminal with a welcome message. If you ever feel stuck you can always run in the `help` command once inside the `imcsh` shell.

## Capabilities and usage
The app functions like any regular shell would, it continously prompts the user, asking for an input command to be executed. We can separate internal commands and external invokable programs in our application. 

### Commands
Here is a full list of possible commands one might give to the imcsh shell:

- globalusage - Displays information about the shell
- help        - Show a help message
- echo        - Echoes the user's input - expects a string to echo
- quit        - Quit the shell
- exec        - Execute a program like a regular shell would do - expects a command to run

### Modifiers
Additionally, there are modifiers that can change how the previously mentioned functions work when added to the end of command:

> [!NOTE]
> In case of the `exec` function you may chain `>` and `&` after each other, meaning that not only is the execution not awaited on and done in the background, but it's stdout also gets redirected into a file.
> Example: `exec ps > example.txt &`

#### **&** - modifier
Adding the ampersand to the end of a command makes the given function run in the background, meaning new inputs are not blocked until the started process is finished.

Can be used on the `exec` commands.

#### **>** - modifier
Adding it to the end of a command in the form of `[command] > [filename]`, redirects the output of the invoked function to a file instead named as the given filename. Example usage: `exec ps > example.txt`.

Can be user on the `globalusage`, `help`, `echo`, `exec` commands.

## Interesting design choices
Throughout development I have noticed there are several different ways to approach the problem and I have even ran into some curious things, which I will explain here. Of course I will only explain some things on the high level, things that are not already explained by comments in the code, which I have left a lot of because C is new to me. (Also left some sources to possibly visit once I open this repository again in the future.)

### General structure - Functions setup
My logic was to create the separate functionalities of the shell as functions, that get invoked by the `handle_input` after matching the user's input to a valid function.

I found it logical to then "map" the usable function names to the actual functions' pointers via a `function_entry` struct. This struct would hold additional information about the function, like whether it can be used with modifiers and arguments or not, and get validated in the `handle_input` accordingly. Here is the relevant part of the code:

```C
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
```

### `posix_spawn` VS `fork` and `execv` - Handling subprocesses
The common approach for creating the `exec` function is to `fork` the parent and then run some version of `execvp` inside of the child process to execute the command, however while researching the topic on Stackoverflow I found `posix_spawn` in the comments.

Obviously I haven't heard of `posix_spawn` before as a complete C beginner, but neither many of the Stackoverflow users it seems like. From what I gathered, it is pretty much the perfect solution for executing a command in our case where we don't need any special setup except for maybe writing to a file, but that too can be done with `file_actions`.

Apparently, the reason it is better is because it's more memory efficient as it doesn't copy the memory space of the parent process for the creation of the child process and simpler to manage. (Of course this doesn't make much difference in such a small and simple application.) Being this unique and seemingly more efficient in some ways, I chose to implement `exec` via `posix_spawn`.

### `>` modifier and the issue with redirecting all output
My initial idea was that I would call a helper function that redirects the `stdout` using `dup2` as was suggested in the assignment description. However, I decided to not do this afterall, as it could potentially cause issues with how background and foreground processes may get out of sync and redirect the `stdout` in an unexpected way when using combination of `&` modified and regular commands.

Instead I decided to write the output of all the "native" functions of the shell directly to a file, circumventing the need for redirection of the `stdout`, and only the `exec` command would get redirected which themselves are redirected in the child process, (leaving the parent process's output unaffected) which behind the scenes did use `dup` just via the `posix_spawn` wrapper's `file_actions`.

### Signals VS Pipes - Parent-Child communication
When researching the problem I found that child processes send a signal to their parents when finishing, which then could be used to act upon.

In my case I am using the `sigaction` to set a handler before the main loop starts, defining what to do when the parent process encounters the `SIGCHILD` signal. This part was pretty hard to implement as someone who hasn't touched C before even with some AI guidance and explanation, but I did also find many sources about the function, just not in the exact context I wanted to use it.

The function `sigchld_handler` then reaps all hanging zombie processes, printing their `pid` as requested in the specifications of the assignment.

Once I have implemented my application this way I realized that in the course we spent more time on pipes and didn't do so much with signals / interrupts, so my guess would be that the expected solutions is to set up an array of pipes dynamically that can communicate with the parent process directly, or they themselves are the ones printing once execution finished.

### Miscellaneous
Here I also wanted to quickly mention how surprisingly easy it was in the end to colour the output with ANSI colours and how relatively unpainful it was to get the username and hostname. With the hostname and username I created a global variable that gets initialized with the start of the shell before the main loop.

## Conclusion
I found that this was a great exercise and I am glad that we had it, but doing even the simplest things in C is so painful once you are used to higher level programming languages.

I try and use AI when learning as little as possible (or use it as a step-by-step explainer instead of letting it write instead of me), but it definitely makes things a lot more easier in a case like this when you know what you would do in a higher level programming language, but it is just not as simple in C, e.g.: taking input from the user, then validating it is frustratingly complex.