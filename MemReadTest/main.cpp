#include "Memory.hpp"
#include <stdint.h>
#include <iostream>
#include <vector>
#include <fstream>

int main() {
    using namespace std;

    wstring TARGET_PROCESS_NAME = L"Lineage2M l Katzman"s;
    string TARGET_PROCESS_NAME_S = "Lineage2M.exe";

    int GAME_VERSION_MODULE_OFFSET = 0x2A1D738; // [Base address of 'League of Legends.exe']+0x2A1D738 (address of a string containing a version number)

    Memory Memory;
    Memory.GetDebugPrivileges();

    int processId = Memory.GetProcessId(TARGET_PROCESS_NAME);
    cout << processId << "\n";

    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, processId);

    HMODULE baseAddress = Memory.GetModuleBase(processHandle, TARGET_PROCESS_NAME_S);

    cout << reinterpret_cast<long long int> (baseAddress) << "\n";

    // int gameVersionAddress = baseAddress + GAME_VERSION_MODULE_OFFSET;
    long long int buffSize = 1024 * 1024 * 128;
    char* stringToRead = new char[buffSize];

    SIZE_T NumberOfBytesToRead = buffSize;
    SIZE_T NumberOfBytesActuallyRead = buffSize;
    
    BOOL success = ReadProcessMemory(processHandle, (LPCVOID)baseAddress, stringToRead, NumberOfBytesToRead, &NumberOfBytesActuallyRead);

    cout << "result: " << success << " " << NumberOfBytesActuallyRead << "\n";

    ofstream outfile;
    outfile.open("test_data.bin",ios::binary|ios::out);

    outfile.write(stringToRead, NumberOfBytesActuallyRead);

    outfile.close();

    delete[] stringToRead;
    // std::cout << "Game version: " << gameVersionAddress << std::endl;

    return 0;
}