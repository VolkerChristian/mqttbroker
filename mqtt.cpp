
#include <cstdint>
#include <iostream>
#include <list>
#include <vector>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    std::vector<char> vec;
    vec.reserve(5);

    vec.push_back('a');
    vec.push_back('b');
    vec.push_back('c');

    std::list<int> li(vec.begin(), vec.end());

    std::string s(vec.begin(), vec.end());

    for (auto value : li) {
        std::cout << static_cast<uint16_t>(value) << ", ";
    }
    std::cout << std::endl;

    std::cout << s << std::endl;

    return 0;
}
