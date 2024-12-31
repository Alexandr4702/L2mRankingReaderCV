#include "Memory.hpp"
#include <iostream>
#include <vector>

int main() {
    using namespace std;

    wstring TARGET_PROCESS_NAME = L"Lineage2M l OrbFighter"s;

    Memory memory;
    memory.EnableDebugPrivileges();

    int processId = memory.GetProcessId(TARGET_PROCESS_NAME);
    cout << "Process ID: " << processId << endl;

    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, processId);
    if (processHandle == 0) {
        cerr << "Failed to open process" << endl;
        return 1;
    }

    uintptr_t testAddress = 0x123456; // Пример тестового адреса
    try {
        int value = memory.ReadMemory<int>(processHandle, testAddress);
        cout << "Read int value: " << value << endl;

        float floatValue = memory.ReadMemory<float>(processHandle, testAddress + 4);
        cout << "Read float value: " << floatValue << endl;
    } catch (const std::exception &e) {
        cerr << e.what() << endl;
    }

    CloseHandle(processHandle);
    return 0;
}
