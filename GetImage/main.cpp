#include <iostream>

#include "Tools.h"

void sendMouseClick(HWND hwnd, int x, int y)
{
    LPARAM lParam = MAKELPARAM(x, y);

    PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
    PostMessage(hwnd, WM_LBUTTONUP, 0, lParam);
}

void sendMouseMove(HWND hwnd, int x, int y)
{
    LPARAM lParam = MAKELPARAM(x, y);

    PostMessage(hwnd, WM_MOUSEMOVE, 0, lParam);
}

int main()
{
    ImageGetter imageGetter;

    const char *windowTitle = "Lineage2M l KanunJarrus";
    HWND hwnd = FindWindowA(NULL, windowTitle);
    std::cout << hwnd << std::endl;

    if (!hwnd)
    {
        std::cerr << "Window not found!" << std::endl;
        return false;
    }

    if (!imageGetter.initialize(hwnd))
    {
        return -1;
    }

    while (true)
    {
        ConsoleEncodingSwitcher a(1251);

        cv::Mat mat = imageGetter.captureImage();
        if (mat.empty())
        {
            std::cerr << "Failed to capture image!" << std::endl;
            break;
        }
        // cv::imshow("Captured Image", mat);
        PrintActiveWindowTitle();
        PrintWindowUnderCursorTitle();

        // sendMouseMove(hwnd, 10, 10);
        sendKeystroke(hwnd, 0x49);
        Sleep(750);
        std::cout << "Sent\n";

        // // Проверка нажатия клавиши для выхода
        // if (cv::waitKey(30) >= 0)
        //     break;
    }

    return 0;
}
