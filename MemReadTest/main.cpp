#include "Memory.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <psapi.h>


int main1() {
    try {
        using namespace std;

        std::wstring toFind = L"OrbF";

        vector<uint8_t> toFind1 = {0x00, 0x4F, 0x00, 0x20, 0x00, 0x72, 0x00, 0x20, 0x00, 0x62, 0x00, 0x20};

        Memory memory(L"Lineage2M l OrbFighter");

        auto moduleMemory = memory.ReadModuleToVector("Lineage2M.exe");

        auto it = std::search(moduleMemory.begin(), moduleMemory.end(), toFind.begin(), toFind.end());

        if (it != moduleMemory.end()) 
        {
            size_t position = std::distance(moduleMemory.begin(), it);
            std::cout << "String found " << position << "\n";
        } else
        {

        }

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

int main2() {
    try {
        // Имя процесса
        std::wstring processName = L"Lineage2M l OrbFighter";
        std::string moduleName = "Lineage2M.exe";

        // Открытие процесса
        HWND hwnd = FindWindowW(nullptr, processName.c_str());
        if (!hwnd) {
            throw std::runtime_error("Failed to find process window");
        }

        DWORD processId;
        GetWindowThreadProcessId(hwnd, &processId);

        HANDLE processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
        if (!processHandle) {
            throw std::runtime_error("Failed to open process");
        }

        // Получение базового адреса модуля
        HMODULE hModules[1024];
        DWORD cbNeeded;
        if (EnumProcessModules(processHandle, hModules, sizeof(hModules), &cbNeeded)) {
            for (size_t i = 0; i < cbNeeded / sizeof(HMODULE); ++i) {
                char moduleFileName[MAX_PATH];
                if (GetModuleBaseNameA(processHandle, hModules[i], moduleFileName, sizeof(moduleFileName))) {
                    // if (moduleName == moduleFileName) {
                        std::cout << "ModuleName: " << moduleFileName << "\n";
                        uintptr_t moduleBase = reinterpret_cast<uintptr_t>(hModules[i]);
                        AnalyzeModuleSections(processHandle, moduleBase);
                        // break;
                    // }
                }
            }
        }

        CloseHandle(processHandle);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}

#include <iostream>
#include <Windows.h>
#include <vector>

bool ReadMemorySafely(HANDLE processHandle, LPCVOID address, LPVOID buffer, SIZE_T size) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(processHandle, address, &mbi, sizeof(mbi)) == 0) {
        std::cerr << "Error: VirtualQueryEx failed for address: " << address << std::endl;
        return false;
    }

    bool skip_check = true;
    if ((mbi.State != MEM_COMMIT) && skip_check) {
        std::cerr << "Skipped: Memory region is not committed (MEM_COMMIT) at address: " << address << std::endl;
        return false;
    }

    if ((mbi.Protect & PAGE_NOACCESS) && skip_check) {
        std::cerr << "Skipped: Memory region is protected (PAGE_NOACCESS) at address: " << address << std::endl;
        return false;
    }

    if ((mbi.Protect & PAGE_GUARD) && skip_check) {
        std::cerr << "Skipped: Memory region is protected (PAGE_GUARD) at address: " << address << std::endl;
        return false;
    }

    SIZE_T bytesRead;
    if (ReadProcessMemory(processHandle, address, buffer, size, &bytesRead)) {
        return true; // Successfully read memory
    } else {
        std::cerr << "Error: Failed to read memory at address: " << address
                  << " (error code: " << GetLastError() << ")" << std::endl;
        return false;
    }
}

int main() {

    std::wstring windowsName = L"Lineage2M l OrbFighter";
    
    HWND hwnd = FindWindowW(nullptr, windowsName.c_str());
    if (!hwnd) {
        throw std::runtime_error("Failed to find process window");
    }

    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);

    DWORD processID = processId; // Target process ID
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processID);
    if (!hProcess) {
        std::cerr << "Error: Failed to open process! Error code: " << GetLastError() << std::endl;
        return 1;
    }

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    LPCVOID address = si.lpMinimumApplicationAddress;
    std::vector<BYTE> buffer(4096);

    while (address < si.lpMaximumApplicationAddress) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProcess, address, &mbi, sizeof(mbi)) == 0) {
            std::cerr << "Error: VirtualQueryEx failed for address: " << address
                      << " (error code: " << GetLastError() << ")" << std::endl;
            break;
        }

        if (mbi.State == MEM_COMMIT && !(mbi.Protect & PAGE_NOACCESS) && !((mbi.Protect & PAGE_GUARD) == PAGE_GUARD) || 1) {
            if (ReadMemorySafely(hProcess, address, buffer.data(), buffer.size())) {
                // std::cout << "Success: Memory read successfully at address: " << address << std::endl;
            }
        } else {
            std::cerr << "Skipped: Memory region at address: " << address 
            << " mbi.State == MEM_COMMIT: " << (mbi.State == MEM_COMMIT)
            << " (mbi.Protect & PAGE_NOACCESS): " << (mbi.Protect & PAGE_NOACCESS)
            << " (mbi.Protect & PAGE_GUARD): " << ((mbi.Protect & PAGE_GUARD) == PAGE_GUARD)
            << std::endl;
        }

        address = static_cast<LPCVOID>(static_cast<const char*>(mbi.BaseAddress) + mbi.RegionSize);
    }

    CloseHandle(hProcess);
    return 0;
}
