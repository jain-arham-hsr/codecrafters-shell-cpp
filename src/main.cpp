#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
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

    string path_env = getenv("PATH") ? getenv("PATH") : "";
    istringstream pathStream(path_env);
    string dir;
    while (getline(pathStream, dir, ':')) {
        string full_path = dir + "/" + args[0];
        if (access(full_path.c_str(), X_OK) == 0) {
            cout << args[0] << " is " << full_path;
            return 0;
        }
    }

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
            cout << command << ": command not found";
        }

        cout << "\n";
    }
}
