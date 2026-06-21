#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>

using namespace std;

using CommandFunc = function<int(const vector<string> &)>;

vector<string> split_input(const string &input) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(input);
    while (tokenStream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

string find_in_path(const string &command) {
    string path_env = getenv("PATH") ? getenv("PATH") : "";
    istringstream pathStream(path_env);
    string dir;
    while (getline(pathStream, dir, ':')) {
        string full_path = dir + "/" + command;
        if (access(full_path.c_str(), X_OK) == 0)
            return full_path;
    }
    return "";
}

unordered_map<string, CommandFunc> builtins;

int builtin_exit(const vector<string> &args) {
    exit(0);
    return 0;
}

int builtin_echo(const vector<string> &args) {
    for (auto arg : args)
        cout << arg << " ";
    return 0;
}

int builtin_type(const vector<string> &args) {
    if (builtins.count(args[0])) {
        cout << args[0] << " is a shell builtin";
        return 0;
    }

    string full_path = find_in_path(args[0]);
    if (!full_path.empty())
        cout << args[0] << " is " << full_path;
    else
        cout << args[0] << ": not found";

    return 0;
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    builtins = {
        {"exit", builtin_exit}, {"echo", builtin_echo}, {"type", builtin_type}};

    while (true) {
        std::cout << "$ ";

        string inp;

        if (!getline(cin, inp)) {
            break;
        }

        vector<string> tokens = split_input(inp);

        if (tokens.empty())
            continue;

        string command = tokens[0];
        vector<string> args(tokens.begin() + 1, tokens.end());

        if (builtins.find(command) != builtins.end()) {
            builtins[command](args);
        } else {
            string full_path = find_in_path(command);
            if (full_path.empty()) {
                cout << command << ": command not found";
            } else {
                vector<char *> argv;
                argv.push_back(const_cast<char *>(command.c_str()));
                for (auto &a : args)
                    argv.push_back(const_cast<char *>(a.c_str()));
                argv.push_back(nullptr);

                pid_t pid = fork();
                if (pid == 0) {
                    execv(full_path.c_str(), argv.data());
                    _exit(127);
                } else if (pid > 0) {
                    int status;
                    waitpid(pid, &status, 0);
                }
            }
        }

        cout << "\n";
    }
}
