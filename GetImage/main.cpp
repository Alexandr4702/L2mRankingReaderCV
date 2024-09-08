#include <iostream>

#include "Tools.h"

void sendMouseClick(HWND hwnd, int x, int y)
{
    // Конвертировать координаты в координаты клиента окна
    LPARAM lParam = MAKELPARAM(x, y);

    // Отправить сигнал нажатия кнопки мыши (левая кнопка)
    PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
    PostMessage(hwnd, WM_LBUTTONUP, 0, lParam);
}

void sendMouseMove(HWND hwnd, int x, int y)
{
    // Конвертировать координаты в координаты клиента окна
    LPARAM lParam = MAKELPARAM(x, y);

    // Отправить сигнал движения мыши
    PostMessage(hwnd, WM_MOUSEMOVE, 0, lParam);
}

// Функция для получения клиентской области окна и преобразования координат
void GetClientCoordinates(HWND hwnd, int &x, int &y)
{
    POINT pt = {x, y};
    ScreenToClient(hwnd, &pt);
    x = pt.x;
    y = pt.y;
}

// Функция для перемещения мыши и клика с использованием SendInput
void SendMouseInput(int x, int y, bool click = false)
{
    INPUT input = {0};

    // Устанавливаем координаты курсора
    input.type = INPUT_MOUSE;
    input.mi.dx = (x * 65535) / GetSystemMetrics(SM_CXSCREEN);
    input.mi.dy = (y * 65535) / GetSystemMetrics(SM_CYSCREEN);
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(INPUT));

    if (click)
    {
        // Эмуляция нажатия левой кнопки мыши
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));

        // Эмуляция отпускания левой кнопки мыши
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }
}

HWND hwndLast = NULL;

void CALLBACK WinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG, LONG, DWORD, DWORD)
{
    if (event == EVENT_SYSTEM_FOREGROUND)
    {
        if (hwnd != hwndLast)
        {
            hwndLast = hwnd;
            wchar_t title[256];
            GetWindowTextW(hwnd, title, sizeof(title) / sizeof(title[0]));
            std::wcout << L"Active window changed to: " << title << std::endl;
        }
    }
}

int main()
{
    using namespace cv;
    using namespace std;

    const wstring winPrefix = L"Lineage2M l "s;

    wstring charName_u16 = L"KanunJarrus"s;

    wstring windowTitle = winPrefix + charName_u16;

    HWND hwnd = FindWindowW(NULL, windowTitle.c_str());
    std::cout << hwnd << std::endl;

    ImageGetter imageGetter;

    if (!hwnd)
    {
        std::cerr << "Window not found!" << std::endl;
        return false;
    }

    if (!imageGetter.initialize(hwnd))
    {
        return -1;
    }

    cv::Mat mat = imageGetter.captureImage();
    if (mat.empty())
    {
        std::cerr << "Failed to capture image!" << std::endl;
    }

    imshow("Image", mat);

    waitKey(0);

    return 0;
}
