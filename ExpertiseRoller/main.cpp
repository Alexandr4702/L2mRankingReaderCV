#include <Windows.h>
#include <iostream>
#include <string>
#include <thread>

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (IsWindowVisible(hwnd))
    {
        char windowTitle[256];
        GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
        HWND handler = FindWindow(NULL, windowTitle);
        std::cout << "Window Title: " << windowTitle << " PID: " << handler << std::endl;
    }
    return TRUE;
}

void MinimizeWindow(const std::string &windowTitle)
{
    HWND window = FindWindow(NULL, windowTitle.c_str());
    if (window != NULL)
    {
        ShowWindow(window, SW_MINIMIZE);
    }
    else
    {
        std::cerr << "Window not found." << std::endl;
    }
}

void MaximizeWindow(const std::string &windowTitle)
{
    HWND window = FindWindow(NULL, windowTitle.c_str());
    if (window != NULL)
    {
        ShowWindow(window, SW_SHOWMAXIMIZED);
    }
    else
    {
        std::cerr << "Window not found." << std::endl;
    }
}

void MouseClick(uint32_t x, uint32_t y)
{
    INPUT Inputs[3] = {0};
    ZeroMemory(Inputs, sizeof(Inputs));

    Inputs[0].type = INPUT_MOUSE;
    Inputs[0].mi.mouseData = 0;
    Inputs[0].mi.dx = 10; // desired X coordinate
    Inputs[0].mi.dy = 10; // desired Y coordinate
    Inputs[0].mi.dwFlags =  MOUSEEVENTF_MOVE;

    std::cout << 65536 / GetSystemMetrics(SM_CXSCREEN) << " " << 65536 / GetSystemMetrics(SM_CYSCREEN) << std::endl;

    Inputs[1].type = INPUT_MOUSE;
    Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    Inputs[2].type = INPUT_MOUSE;
    Inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(ARRAYSIZE(Inputs), Inputs, sizeof(INPUT));
}

int main()
{

    Sleep(1000); // Sleep for 2000 milliseconds (2 seconds)
    EnumWindows(EnumWindowsProc, 0);

    std::string windowTitle = "./ExpertiseRoller.exe";

    HWND window_handler = FindWindow(NULL, windowTitle.c_str());
    if (window_handler != NULL)
    {
        SetForegroundWindow(window_handler);
    }
    else
    {
        std::cerr << "Window not found." << std::endl;
        return -1;
    }
    // Minimize the window
    // MaximizeWindow(windowTitle);

    // SetActiveWindow(windowTitle);

    std::thread Example([=](){
    while (1)
    {
        Sleep(1000);
        SetForegroundWindow(window_handler);
        // ShowWindow(window_handler, SW_SHOW);
        MouseClick(1550, 580);
        SetForegroundWindow(window_handler);
    }
    }
    );
    Example.join();
    // MinimizeWindow(windowTitle);

    return 0;
}