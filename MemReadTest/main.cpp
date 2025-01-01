#include "Memory.hpp"
#include <iostream>
#include <fstream>

int main() {
    try {
        Memory memory(L"Lineage2M l OrbFighter");

        memory.ReadModuleToVector("Lineage2M.exe");
        const auto& moduleMemory = memory.GetModuleMemory();

        std::ofstream outfile;
        outfile.open("test_data.bin", std::ios::binary | std::ios::out);

        outfile.write(
            reinterpret_cast<const char*> (moduleMemory.data()), moduleMemory.size());

        outfile.close();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
