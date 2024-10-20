/*
Name: Abul Hasan Shaik
Course: CSC 456-S01
Profrssors Name: Dr. Jun Huang
Programming Assignment: 1
Due: 10/20/2024 11:59 PM
*/
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <dirent.h> // For directory listing
#include <pwd.h>    // For getting home directory
using namespace std;

string currentDir = ""; // Stores the current directory

//Function prototypes or declarations
int shell_cd(vector<string> &args);
int shell_exit(vector<string> &args);

//List of built-in commands followed by their corresponding functions.
vector<string> built_in_cmds = 
    {
    "cd",
    "exit"
    };

int (*built_in_funcs[])(vector<string> &) = {
    &shell_cd,
    &shell_exit};

int get_num_built_in_cmds()
{
    return built_in_cmds.size();
}

/*
  Built-in function implementations.
*/

/**
   @brief Built-in command: change directory.
   @param args List of arguments. args[0] is "cd", args[1] is the directory.
   @return Always returns 1 to continue execution.
 */
int shell_cd(vector<string> &args)
{
    if (args.size() < 2)
    {
        cerr << "shell: expected argument to \"cd\"" << endl;
    }
    else
    {
        if (chdir(args[1].c_str()) != 0)
        {
            perror("shell");
        }
        else
        {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != nullptr)
            {
                currentDir = string(cwd);
            }
        }
    }
    return 1;
}

/**
   @brief Built-in command: exit.
   @param args List of arguments. Not used.
   @return Always returns 0 to terminate execution.
 */
int shell_exit(vector<string> &args)
{
    cout << "\nBye!\n\n";
    return 0;
}

/**
  @brief Launch a program and wait for it to terminate.
  @param args List of arguments (including the program to execute).
  @return Always returns 1 to continue execution.
 */
int launch_process(vector<string> &args)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0)
    {
        // Child process
        vector<char *> c_args;
        for (const string &arg : args)
        {
            c_args.push_back(const_cast<char *>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        if (execvp(c_args[0], c_args.data()) == -1) //Using execvp() system call which is a part of exec familt to to run the user command with an arbitrary number of arguments
        {
            perror("shell");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        // Error forking
        perror("shell");
    }
    else
    {
        // Parent process
        do
        {
            wpid = waitpid(pid, &status, WUNTRACED);    //Using waitpid() to wait for the child process(the user command) to finish before continuing as required
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

/**
   @brief Execute shell built-in command or launch a program.
   @param args List of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate.
 */
int execute_command(vector<string> &args)
{
    if (args.empty())
    {
        // No command entered
        return 1;
    }

    // Check if the first argument is "exit" (case-insensitive)
    if (strcasecmp(args[0].c_str(), "exit") == 0)
    {
        return shell_exit(args);
    }

    for (int i = 0; i < get_num_built_in_cmds(); i++)
    {
        if (strcasecmp(args[0].c_str(), built_in_cmds[i].c_str()) == 0)
        {
            return (*built_in_funcs[i])(args);
        }
    }

    return launch_process(args);
}

#define BUFFER_SIZE 1024
/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
string read_line()
{
    int bufsize = BUFFER_SIZE;
    int position = 0;
    string buffer;
    int c;

    buffer.resize(bufsize);

    while (1)
    {
        // Read a character
        c = getchar();

        // End input on EOF or newline
        if (c == EOF || c == '\n')
        {
            buffer[position] = '\0';
            return buffer;
        }
        else
        {
            buffer[position] = static_cast<char>(c);
        }
        position++;

        // Resize buffer if it exceeds initial size
        if (position >= bufsize)
        {
            bufsize += BUFFER_SIZE;
            buffer.resize(bufsize);
        }
    }
}

#define TOKEN_BUFFER_SIZE 64
#define TOKEN_DELIMITERS " \t\r\n\a"
/**
   @brief Split a line into tokens.
   @param line The line to be split.
   @return Vector of tokens (arguments).
 */
vector<string> split_line(const string &line)
{
    istringstream iss(line);
    vector<string> tokens;
    string token;

    while (iss >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

/**
   @brief Get the list of subdirectories in the current working directory.
   @return A vector of subdirectories.
 */
vector<string> list_subdirectories()
{
    vector<string> subdirs;
    DIR *dir = opendir(".");
    struct dirent *entry;

    if (dir == nullptr)
    {
        perror("shell");
        return subdirs;
    }

    while ((entry = readdir(dir)) != nullptr)
    {
        if (entry->d_type == DT_DIR)
        {
            if (string(entry->d_name) != "." && string(entry->d_name) != "..")
            {
                subdirs.push_back(entry->d_name);
            }
        }
    }

    closedir(dir);
    return subdirs;
}

/**
   @brief Get the current working directory.
   @return The current directory as a string.
 */
string get_curr_dir()
{
    if (!currentDir.empty())
    {
        string home_dir = getenv("HOME");
        size_t pos = currentDir.find(home_dir);
        if (pos != string::npos)
        {
            string subdir = currentDir.substr(pos + home_dir.length() + 1);
            if (!subdir.empty())
            {
                size_t lastSlash = subdir.find_last_of('/');
                if (lastSlash != string::npos)
                {
                    return subdir.substr(lastSlash + 1);
                }
                else
                {
                    return subdir;
                }
            }
        }
    }
    return currentDir;
}

/**
   @brief Main loop: Get input and execute commands.
 */
void main_loop()
{
    string line;
    vector<string> args;
    int status;

    do
    {
        vector<string> subdirs = list_subdirectories();
        string curr_dir = get_curr_dir();

        cout << "Shaik";
        if (!curr_dir.empty())
        {
            cout << "/" << curr_dir;
        }
        cout << "> ";
        line = read_line();
        args = split_line(line);
        status = execute_command(args);
    } while (status);
}

/**
   @brief Entry point of the program.
   @param argc Argument count.
   @param argv Argument vector.
   @return Exit status.
 */
int main(int argc, char **argv)
{
    // Load configuration files if any.

    // Start the command loop.
    main_loop();

    // Perform cleanup if necessary.

    return EXIT_SUCCESS;
}
