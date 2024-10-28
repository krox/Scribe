#include <iostream>

#include "mystruct.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <file.json>\n";
        return 1;
    }
    mystruct data;
    read_file(data, argv[1]);
    std::cout << "field1: " << data.field1 << "\n";
    if (data.field2)
        std::cout << "field2: " << *data.field2 << "\n";
    else
        std::cout << "field2: null\n";
    std::cout << "field3: ";
    for (auto &elem : data.field3)
        std::cout << elem << " ";
    std::cout << "\n";
    std::cout << "field4.a: " << data.field4.a << "\n";
}