#include <climits>
#include <cstdlib>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>

using namespace std;

struct Redirects {
    string file[3];
    bool append[3] = {false, false, false};
    bool set[3] = {false, false, false};
    int saved[3] = {-1, -1, -1};
};

Redirects parse_redirections(vector<string> &tokens) {
    Redirects r;
    vector<string> clean;

    for (size_t i = 0; i < tokens.size(); i++) {
        string &t = tokens[i];
        int fd = -1;
        bool app = false;
        if (t == ">" || t == "1>")
            fd = 1;
        else if (t == ">>" || t == "1>>") {
            fd = 1;
            app = true;
        } else if (t == "2>")
            fd = 2;
        else if (t == "2>>") {
            fd = 2;
            app = true;
        }

        if (fd != -1 && i + 1 < tokens.size()) {
            r.set[fd] = true;
            r.append[fd] = app;
            r.file[fd] = tokens[++i];
        } else {
            clean.push_back(t);
        }
    }

    tokens = clean;
    return r;
}

void apply_redirections(Redirects &r) {
    for (int fd = 1; fd <= 2; fd++) {
        if (!r.set[fd])
            continue;
        int flags = O_WRONLY | O_CREAT | (r.append[fd] ? O_APPEND : O_TRUNC);
        int file_fd = open(r.file[fd].c_str(), flags, 0644);
        if (file_fd < 0) {
            perror(r.file[fd].c_str());
            continue;
        }
        r.saved[fd] = dup(fd);
        dup2(file_fd, fd);
        close(file_fd);
    }
}

void restore_redirections(Redirects &r) {
    cout.flush();
    cerr.flush();
    for (int fd = 1; fd <= 2; fd++) {
        if (r.saved[fd] != -1) {
            dup2(r.saved[fd], fd);
            close(r.saved[fd]);
        }
    }
}

using CommandFunc = function<int(const vector<string> &)>;

vector<string> split_input(const string &input) {
    vector<string> tokens;
    string token;
    char quote = 0;
    bool in_token = false;

    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        bool escape = c == '\\' && i + 1 < input.size() &&
                      (quote == 0 ||
                       (quote == '"' &&
                        string("\\$\"\n").find(input[i + 1]) != string::npos));

        if (!quote && isspace((unsigned char)c)) {
            if (in_token) {
                tokens.push_back(token);
                token.clear();
                in_token = false;
            }
        } else if (!quote && (c == '\'' || c == '"')) {
            quote = c;
            in_token = true;
        } else if (quote && c == quote) {
            quote = 0;
        } else if (escape) {
            token += input[++i];
            in_token = true;
        } else {
            token += c;
            in_token = true;
        }
    }

    if (in_token)
        tokens.push_back(token);
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
    cout << "\n";
    return 0;
}

int builtin_type(const vector<string> &args) {
    if (builtins.count(args[0])) {
        cout << args[0] << " is a shell builtin" << "\n";
        return 0;
    }

    string full_path = find_in_path(args[0]);
    if (!full_path.empty())
        cout << args[0] << " is " << full_path;
    else
        cout << args[0] << ": not found";
    cout << "\n";
    return 0;
}

int builtin_pwd(const vector<string> &args) {
    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf)) != nullptr)
        cout << buf;
    cout << "\n";
    return 0;
}

int builtin_cd(const vector<string> &args) {
    string path = args[0];
    if (path == "~") {
        const char *home = getenv("HOME");
        if (home)
            path = home;
    }

    if (chdir(path.c_str()) != 0)
        cout << "cd: " << args[0] << ": No such file or directory" << "\n";
    return 0;
}

int builtin_jobs(const vector<string> &args) { return 0; }

int next_job_id = 1;

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    builtins = {{"exit", builtin_exit}, {"echo", builtin_echo},
                {"type", builtin_type}, {"pwd", builtin_pwd},
                {"cd", builtin_cd},     {"jobs", builtin_jobs}};

    while (true) {

        while (waitpid(-1, nullptr, WNOHANG) > 0) {
        }

        std::cout << "$ ";

        string inp;

        if (!getline(cin, inp)) {
            break;
        }

        vector<string> tokens = split_input(inp);

        if (tokens.empty())
            continue;

        Redirects redirs = parse_redirections(tokens);
        if (tokens.empty())
            continue;

        bool background = false;
        if (tokens.back() == "&") {
            background = true;
            tokens.pop_back();
        }
        if (tokens.empty())
            continue;

        string command = tokens[0];
        vector<string> args(tokens.begin() + 1, tokens.end());

        apply_redirections(redirs);

        if (builtins.find(command) != builtins.end()) {
            builtins[command](args);
        } else {
            string full_path = find_in_path(command);
            if (full_path.empty()) {
                cout << command << ": command not found" << "\n";
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
                    if (background) {
                        cout << "[" << next_job_id++ << "] " << pid << "\n";
                    } else {
                        int status;
                        waitpid(pid, &status, 0);
                    }
                }
            }
        }

        restore_redirections(redirs);
    }
}
