// Name(s): Collin Tucker
// Description: A  Unix-like shell that resembles a typically shell allowing 
// inputs and works like a bash.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // fork, execvp, chdir, getcwd
#include <sys/types.h>
#include <sys/wait.h>   // wait
#include <fcntl.h>      // open
#include <errno.h>      // errno
#include <string.h>     // strerror

#define MAX_INPUT 1024   // max ammount of inputs
#define MAX_ARGS 100     // Maximum number of arguments

/////////////////////// STRUCT ///////////////////////

/*
    This struct stores all important information about a parsed command.

*/

struct ShellCommand
{
    char *args[MAX_ARGS];
    char *inputFile;
    char *outputFile;
};

/////////////////////// FUNCTION DECLARATIONS ///////////////////////

void displayPrompt();
char* getInput();
struct ShellCommand parseInput(char *input);
void executeCommand(struct ShellCommand command);

/////////////////////// DISPLAY PROMPT ///////////////////////

/*
    Prints the current working directory followed by "$ "
    Example:
        /home/user$ 
*/
void displayPrompt()
{
    char cwd[1024];

    // getcwd retrieves current directory
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        printf("%s$ ", cwd);
    else
        perror("getcwd");
}

/////////////////////// GET INPUT ///////////////////////

/*
    Reads a full line of user input.
    Allocates memory dynamically and returns it.
*/
char* getInput()
{
    char *buffer = malloc(MAX_INPUT);

    if (buffer == NULL)
    {
        perror("malloc");
        exit(1);
    }

    // fgets reads input including newline
    if (fgets(buffer, MAX_INPUT, stdin) == NULL)
    {
        free(buffer);
        return NULL;
    }

    return buffer;
}

/////////////////////// PARSE INPUT ///////////////////////

/*
    Parses the user input string

    Uses strtok to split input by spaces and newline.
*/
struct ShellCommand parseInput(char *input)
{
    struct ShellCommand command;

    command.inputFile = NULL;
    command.outputFile = NULL;

    int i = 0;

    // Tokenize input by space and newline
    char *token = strtok(input, " \n");

    while (token != NULL)
    {
        // Detect input redirection
        if (strcmp(token, "<") == 0)
        {
            token = strtok(NULL, " \n");
            command.inputFile = token;
        }
        // Detect output redirection
        else if (strcmp(token, ">") == 0)
        {
            token = strtok(NULL, " \n");
            command.outputFile = token;
        }
        // Otherwise it's part of command/arguments
        else
        {
            command.args[i++] = token;
        }

        token = strtok(NULL, " \n");
    }

    // Important: execvp requires NULL-terminated argument array
    command.args[i] = NULL;

    return command;
}

/////////////////////// EXECUTE COMMAND ///////////////////////


 /// Executes a parsed shell command

void executeCommand(struct ShellCommand command)
{
    // If empty command, do nothing
    if (command.args[0] == NULL)
        return;

    ///////////////////////
    // BUILT-IN: exit
    ///////////////////////
    if (strcmp(command.args[0], "exit") == 0)
        exit(0);

    ///////////////////////
    // BUILT-IN: cd
    ///////////////////////
    if (strcmp(command.args[0], "cd") == 0)
    {
        if (command.args[1] == NULL)
        {
            fprintf(stderr, "cd: missing argument\n");
        }
        else if (chdir(command.args[1]) != 0)
        {
            // Print error in required format
            printf("Error %d (%s)\n", errno, strerror(errno));
        }
        return;
    }

    ///////////////////////
    // CREATE CHILD PROCESS
    ///////////////////////
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return;
    }

    ///////////////////////
    // CHILD PROCESS
    ///////////////////////
    if (pid == 0)
    {
        // Handle input redirection (<)
        if (command.inputFile != NULL)
        {
            int fd = open(command.inputFile, O_RDONLY);

            if (fd < 0)
            {
                printf("Error %d (%s)\n", errno, strerror(errno));
                exit(1);
            }

            // Replace standard input with file
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // Handle output redirection (>)
        if (command.outputFile != NULL)
        {
            int fd = open(command.outputFile,
                          O_WRONLY | O_CREAT | O_TRUNC,
                          0644);

            if (fd < 0)
            {
                printf("Error %d (%s)\n", errno, strerror(errno));
                exit(1);
            }

            // Replace standard output with file
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Replace child process with requested command
        execvp(command.args[0], command.args);

        // If execvp fails, print error
        printf("Error %d (%s)\n", errno, strerror(errno));
        exit(1);
    }
    ///////////////////////
    // PARENT PROCESS
    ///////////////////////
    else
    {
        // Wait for child to finish to prevent zombie process
        wait(NULL);
    }
}

/////////////////////// MAIN ///////////////////////

int main()
{
    char *input;
    struct ShellCommand command;

    // Infinite loop (shell runs until exit)
    for (;;)
    {
        // Display prompt
        displayPrompt();

        // Get user input
        input = getInput();
        if (input == NULL)
            break;

        // If only newline entered, skip
        if (strcmp(input, "\n") == 0)
        {
            free(input);
            continue;
        }

        // Parse command
        command = parseInput(input);

        // Execute command
        executeCommand(command);

        // Free dynamically allocated input
        free(input);
    }

    return 0;
}
