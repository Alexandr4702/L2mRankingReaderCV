#include <windows.h>
#include <vector>
#include <string>
#include <stdint.h>
#include <iostream>
#include "Tools.h"

int main()
{
    Beep(1000, 500);

    WindowLister windowLister;

    windowLister.RefreshWindowList();
    const auto &windows = windowLister.GetWindows();

    {
        ConsoleEncodingSwitcher switcher(1251);
        // Вывести информацию обо всех окнах
        for (const auto &window : windows)
        {
            std::cout
                << " HWNDx16: " << window.hwnd
                << " HWNDx10: " << reinterpret_cast<uint64_t>(window.hwnd)
                << " - Title: " << window.title
                << std::endl;
        }
    }

    const char *windowTitle = "Lineage2M l KanunJarrus";

    // Найти окно по его заголовку
    HWND hwnd = FindWindowA(NULL, windowTitle);

    if (hwnd == NULL)
    {
        std::cerr << "Window is not found" << std::endl;
        return 1;
    }

    // Sleep(100); // Небольшая задержка

    // sendKeystroke(hwnd, 'I');
    // Sleep(1000);
    // sendKeystroke(hwnd, 'I');
    // Sleep(1000);
    // sendKeystroke(hwnd, 'I');

    while (1)
    {
        MoveMouseSendInput(hwnd, 100, 100);
        MoveMouseSendMessage(hwnd, 100, 100);
        Sleep(1000);
        MoveMouseSendInput(hwnd, 500, 500);
        MoveMouseSendMessage(hwnd, 500, 500);
        Sleep(1000);
    }
    std::cout << "Signal was sent" << std::endl;

    return 0;
}
