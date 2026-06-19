#include <iostream>
#include <string>

using namespace std;

int main() {
    // Flush after every std::cout / std:cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    while (true) {
        std::cout << "$ ";

        string command;
        getline(cin, command);

        if (command == "exit")
            break;

        cout << command << ": command not found";

        cout << "\n";
    }
}
