#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

void executeCommand(const vector<string>& args) {
    pid_t pid = fork();
    if (pid == 0) { // Child process
        vector<char*> c_args;
        for (const auto& arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr); // Null-terminate the array

        execvp(c_args[0], c_args.data());
        perror("execvp failed"); // If execvp fails
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent process
        wait(nullptr); // Wait for child process to finish
    } else {
        perror("fork failed");
    }
}

int main() {
    string input;
    while (true) {
        cout << "Shaik> ";
        getline(cin, input);
        if (input == "exit") {
            cout << "Bye!" << endl;
            break;
        }

        stringstream ss(input);
        vector<string> args;
        string arg;
        while (ss >> arg) {
            args.push_back(arg);
        }

        executeCommand(args);
    }
    return 0;
}

