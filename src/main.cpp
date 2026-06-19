#include <iostream>
#include <string>

using namespace std;

int main() {
    // Flush after every std::cout / std:cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // TODO: Uncomment the code below to pass the first stage
    while (true) {
        std::cout << "$ ";

        string command;
        getline(cin, command);

        cout << command << ": command not found";
    }
}
