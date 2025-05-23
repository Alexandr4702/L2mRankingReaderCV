#include "Tools.h"
#include <string>
#include <thread>

TimeMeasure::TimeMeasure(const std::ostream &out, double *save) : m_out(out), m_save_diff(save)
{
    m_save_diff = save;
    m_start = std::chrono::system_clock::now();
}

TimeMeasure::~TimeMeasure()
{
    m_stop = std::chrono::system_clock::now();
    auto diff = m_stop - m_start;
    if (m_save_diff)
        *m_save_diff = std::chrono::duration_cast<std::chrono::microseconds>(diff).count();
    std::cout << "Time in microsec: " << std::chrono::duration_cast<std::chrono::microseconds>(diff).count() << std::endl;
}

ImageGetter::ImageGetter()
    : m_hwnd(NULL), m_hWindowDC(NULL), m_hMemDC(NULL), m_hBitmap(NULL), m_prevWidth(0), m_prevHeight(0) {}

ImageGetter::~ImageGetter()
{
    if (m_hBitmap)
    {
        DeleteObject(m_hBitmap);
    }
    if (m_hMemDC)
    {
        DeleteDC(m_hMemDC);
    }
    if (m_hWindowDC)
    {
        ReleaseDC(m_hwnd, m_hWindowDC);
    }
}

bool ImageGetter::initialize(const char *windowTitle)
{
    m_hwnd = FindWindow(NULL, windowTitle);
    if (!m_hwnd)
    {
        std::cerr << "Window not found!" << std::endl;
        return false;
    }

    return initialize(m_hwnd);
}

bool ImageGetter::initialize(HWND hwnd)
{
    m_hwnd = hwnd;

    m_hWindowDC = GetDC(m_hwnd);
    if (!m_hWindowDC)
    {
        std::cerr << "Failed to get window DC!" << std::endl;
        return false;
    }

    updateSize(); // Initialize window size and bitmap

    m_hMemDC = CreateCompatibleDC(m_hWindowDC);
    if (!m_hMemDC)
    {
        ReleaseDC(m_hwnd, m_hWindowDC);
        std::cerr << "Failed to create memory DC!" << std::endl;
        return false;
    }

    m_hBitmap = CreateCompatibleBitmap(m_hWindowDC, m_rect.right - m_rect.left, m_rect.bottom - m_rect.top);
    if (!m_hBitmap)
    {
        DeleteDC(m_hMemDC);
        ReleaseDC(m_hwnd, m_hWindowDC);
        std::cerr << "Failed to create bitmap!" << std::endl;
        return false;
    }

    return true;
}

void ImageGetter::updateSize()
{
    GetClientRect(m_hwnd, &m_rect);

    int newWidth = m_rect.right - m_rect.left;
    int newHeight = m_rect.bottom - m_rect.top;

    // Recreate the bitmap only if the size has changed
    if (newWidth != m_prevWidth || newHeight != m_prevHeight)
    {
        if (m_hBitmap)
        {
            DeleteObject(m_hBitmap);
        }

        m_hBitmap = CreateCompatibleBitmap(m_hWindowDC, newWidth, newHeight);
        if (!m_hBitmap)
        {
            std::cerr << "Failed to create new bitmap!" << std::endl;
            return;
        }

        m_prevWidth = newWidth;
        m_prevHeight = newHeight;
    }
}

cv::Mat ImageGetter::HBitmapToMat(HBITMAP hBitmap)
{
    BITMAP bmp;
    BITMAPINFOHEADER bi;

    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = -bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;
    HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
    char *lpbitmap = (char *)GlobalLock(hDIB);

    HDC hdc = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hdc);
    SelectObject(hMemDC, hBitmap);

    GetDIBits(hMemDC, hBitmap, 0, (UINT)bmp.bmHeight, lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    cv::Mat mat(bmp.bmHeight, bmp.bmWidth, CV_8UC3, lpbitmap);
    cv::Mat result;
    mat.copyTo(result);

    GlobalUnlock(hDIB);
    GlobalFree(hDIB);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hdc);

    return result;
}

cv::Mat ImageGetter::captureImage()
{
    if (!m_hwnd || !m_hWindowDC || !m_hMemDC || !m_hBitmap)
    {
        std::cerr << "Initialization failed!" << std::endl;
        return cv::Mat();
    }

    updateSize(); // Update size before capturing

    SelectObject(m_hMemDC, m_hBitmap);
    BitBlt(m_hMemDC, 0, 0, m_rect.right - m_rect.left, m_rect.bottom - m_rect.top, m_hWindowDC, 0, 0, SRCCOPY);
    return HBitmapToMat(m_hBitmap);
}

ConsoleEncodingSwitcher::ConsoleEncodingSwitcher(UINT newCP)
{
    // Сохранить текущую кодировку консоли
    originalCP = GetConsoleOutputCP();
    originalInputCP = GetConsoleCP();

    // Установить новую кодировку консоли
    SetConsoleOutputCP(newCP);
    SetConsoleCP(newCP);
}

ConsoleEncodingSwitcher::~ConsoleEncodingSwitcher()
{
    // Восстановить исходную кодировку консоли
    SetConsoleOutputCP(originalCP);
    SetConsoleCP(originalInputCP);
}

BOOL CALLBACK WindowLister::EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    WindowLister *pThis = reinterpret_cast<WindowLister *>(lParam);

    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));

    if (strlen(title) > 0)
    {
        pThis->m_windows.emplace_back(WindowInfo{hwnd, title});
    }
    return TRUE;
}

void checkError()
{
    DWORD error = GetLastError();
    // LPVOID errorMessage;
    // FormatMessage(
    //     FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
    //     FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL,
    //     SUBLANG_DEFAULT), (LPSTR)&errorMessage, 0, NULL);
    std::cerr << "Failed to send WM_KEYDOWN to window. Error: " << error << std::endl;
}
HWND handlePostMessageError(const char* phase) {
    DWORD err = GetLastError();
    std::cerr << "[" << phase << "] PostMessage failed. Error code: " << err << std::endl;
    return nullptr;
}

void sendKeystroke(HWND hwnd, char key)
{
    UINT scanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);
    LPARAM lParamDown = 1 | (scanCode << 16);
    LPARAM lParamUp = 1 | (scanCode << 16) | (1 << 30) | (1 << 31);

    std::cout << "KeyDOWN \n";
    if (!PostMessageA (hwnd, WM_KEYDOWN, key, lParamDown))
    {
        std::cerr << "PostMessage failed. Error: " << GetLastError() << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(30)); // Задержка

    std::cout << "KeyUP \n";
    if (!PostMessageA (hwnd, WM_KEYUP, key, lParamUp))
    {
        std::cerr << "PostMessage failed. Error: " << GetLastError() << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(30)); // Задержка
}

// void sendKeystroke(HWND hwnd, char key)
// {
//     if (!IsWindow(hwnd)) {
//         std::cerr << "[!] Invalid window handle." << std::endl;
//         return;
//     }

//     // Логирование валидности окна
//     char title[256];
//     GetWindowTextA(hwnd, title, sizeof(title));
//     std::cout << "[*] Target window title: " << title << std::endl;

//     DWORD targetThread = GetWindowThreadProcessId(hwnd, NULL);
//     DWORD currentThread = GetCurrentThreadId();

//     // Логируем поток текущего окна
//     std::cout << "[*] Target thread: " << targetThread << ", Current thread: " << currentThread << std::endl;

//     // Проверка, находится ли окно в фокусе
//     HWND foregroundWindow = GetForegroundWindow();
//     std::cout << "[*] Foreground window: " << foregroundWindow << ", Target window: " << hwnd << std::endl;

//     if (foregroundWindow != hwnd) {
//         std::cout << "[!] Warning: Target window is NOT in foreground" << std::endl;
//     }

//     // Привязка потоков, если не в фокусе
//     AttachThreadInput(currentThread, targetThread, TRUE);

//     // Попробуем активировать окно
//     if (!SetForegroundWindow(hwnd)) {
//         std::cerr << "[!] Failed to bring window to foreground." << std::endl;
//     }

//     SetActiveWindow(hwnd);
//     SetFocus(hwnd);

//     // Получим scanCode
//     UINT scanCode = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);
//     LPARAM lParamDown = 1 | (scanCode << 16);
//     LPARAM lParamUp   = 1 | (scanCode << 16) | (1 << 30) | (1 << 31);

//     // Логируем перед отправкой сообщения
//     std::cout << "[*] Posting KeyDOWN: " << key << std::endl;
//     BOOL downOK = PostMessageA(hwnd, WM_KEYDOWN, key, lParamDown);
//     if (!downOK) {
//         std::cerr << "[!] PostMessage failed for WM_KEYDOWN. Error: " << GetLastError() << std::endl;
//     }

//     std::cout << "[*] Posting KeyUP: " << key << std::endl;
//     BOOL upOK = PostMessageA(hwnd, WM_KEYUP, key, lParamUp);
//     if (!upOK) {
//         std::cerr << "[!] PostMessage failed for WM_KEYUP. Error: " << GetLastError() << std::endl;
//     }

//     // Отключаем привязку потоков
//     AttachThreadInput(currentThread, targetThread, FALSE);
// }

// void sendKeystroke(HWND hwnd, char key)
// {
//     // 1) validate window handle
//     if (!IsWindow(hwnd)) {
//         std::cerr << "sendKeystroke: invalid window handle\n";
//         return;
//     }

//     // 2) get target process ID
//     DWORD pid = 0;
//     GetWindowThreadProcessId(hwnd, &pid);
//     if (pid == 0) {
//         std::cerr << "sendKeystroke: failed to get process ID\n";
//         return;
//     }

//     // 3) open the target process with rights for remote thread & memory ops
//     HANDLE hProc = OpenProcess(
//         PROCESS_CREATE_THREAD     |
//         PROCESS_QUERY_INFORMATION |
//         PROCESS_VM_OPERATION      |
//         PROCESS_VM_WRITE          |
//         PROCESS_VM_READ,
//         FALSE, pid);
//     if (!hProc) {
//         std::cerr << "sendKeystroke: OpenProcess failed, error " 
//                   << GetLastError() << "\n";
//         return;
//     }

//     // 4) locate keybd_event in user32.dll
//     HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
//     if (!hUser32) {
//         std::cerr << "sendKeystroke: GetModuleHandle(user32.dll) failed\n";
//         CloseHandle(hProc);
//         return;
//     }
//     auto pKeybdEvent = 
//         (LPTHREAD_START_ROUTINE)GetProcAddress(hUser32, "keybd_event");
//     if (!pKeybdEvent) {
//         std::cerr << "sendKeystroke: GetProcAddress(keybd_event) failed\n";
//         CloseHandle(hProc);
//         return;
//     }

//     // 5) prepare parameter structs for KEYDOWN and KEYUP
//     struct KEYBD_ARG { BYTE bVk, bScan; DWORD dwFlags; ULONG_PTR dwExtra; };
//     BYTE vk = static_cast<BYTE>(key);
//     BYTE scan = static_cast<BYTE>(MapVirtualKeyA(vk, MAPVK_VK_TO_VSC));
//     KEYBD_ARG argsDown = { vk, scan, 0,       0 };
//     KEYBD_ARG argsUp   = { vk, scan, KEYEVENTF_KEYUP, 0 };

//     SIZE_T argSize = sizeof(KEYBD_ARG);
//     // allocate space for both structs
//     LPVOID remoteArgs = VirtualAllocEx(
//         hProc, nullptr, argSize * 2, MEM_COMMIT, PAGE_READWRITE);
//     if (!remoteArgs) {
//         std::cerr << "sendKeystroke: VirtualAllocEx failed, error " 
//                   << GetLastError() << "\n";
//         CloseHandle(hProc);
//         return;
//     }

//     // write argsDown then argsUp
//     if (!WriteProcessMemory(hProc, remoteArgs, &argsDown, argSize, nullptr) ||
//         !WriteProcessMemory(hProc, (BYTE*)remoteArgs + argSize, &argsUp, argSize, nullptr))
//     {
//         std::cerr << "sendKeystroke: WriteProcessMemory failed, error " 
//                   << GetLastError() << "\n";
//         VirtualFreeEx(hProc, remoteArgs, 0, MEM_RELEASE);
//         CloseHandle(hProc);
//         return;
//     }

//     // 6) create remote thread for KEYDOWN
//     HANDLE hThread = CreateRemoteThread(
//         hProc, nullptr, 0,
//         pKeybdEvent,
//         remoteArgs,
//         0, nullptr);
//     if (!hThread) {
//         std::cerr << "sendKeystroke: CreateRemoteThread(down) failed, error " 
//                   << GetLastError() << "\n";
//         VirtualFreeEx(hProc, remoteArgs, 0, MEM_RELEASE);
//         CloseHandle(hProc);
//         return;
//     }
//     WaitForSingleObject(hThread, INFINITE);
//     CloseHandle(hThread);

//     // slight pause to ensure the target processes the DOWN event
//     Sleep(20);

//     // 7) create remote thread for KEYUP
//     hThread = CreateRemoteThread(
//         hProc, nullptr, 0,
//         pKeybdEvent,
//         (BYTE*)remoteArgs + argSize,
//         0, nullptr);
//     if (!hThread) {
//         std::cerr << "sendKeystroke: CreateRemoteThread(up) failed, error " 
//                   << GetLastError() << "\n";
//         VirtualFreeEx(hProc, remoteArgs, 0, MEM_RELEASE);
//         CloseHandle(hProc);
//         return;
//     }
//     WaitForSingleObject(hThread, INFINITE);
//     CloseHandle(hThread);

//     // 8) cleanup
//     VirtualFreeEx(hProc, remoteArgs, 0, MEM_RELEASE);
//     CloseHandle(hProc);
// }

bool SendKeyToWindow(HWND hwnd, WPARAM character)
{
    // // Преобразуем символ в виртуальный код клавиши
    // int vkCode = VkKeyScan(character);
    
    // // Проверяем, успешно ли преобразован символ
    // if (vkCode == -1) {
    //     std::cerr << "Не удалось преобразовать символ в виртуальный код клавиши." << std::endl;
    //     return;
    // }

    // // Получаем текущее время в миллисекундах
    // DWORD timestamp = GetTickCount();
    
    // // Преобразуем временную метку в формат для передачи через lParam
    // LPARAM lParam = (timestamp & 0xFFFFFFFF); // Можно использовать только младшие 32 бита для примера

    // // Отправляем сообщение нажатия клавиши
    // SendMessage(hwnd, WM_KEYDOWN, vkCode, lParam);

    // // Задержка перед отпусканием клавиши
    // Sleep(pressDelayMs);

    // // Обновляем временную метку перед отправкой сообщения об отпускании
    // timestamp = GetTickCount();
    // lParam = (timestamp & 0xFFFFFFFF); // Обновляем lParam с новой временной меткой

    // // Отправляем сообщение отпускания клавиши
    // SendMessage(hwnd, WM_KEYUP, vkCode, lParam);
}

// Function to move the mouse using SendInput relative to the window
void MoveMouseSendInput(HWND hwnd, int x, int y)
{
    // Convert window coordinates to screen coordinates
    POINT pt = {x, y};
    ClientToScreen(hwnd, &pt);

    // Convert coordinates to absolute values
    int absX = pt.x * (65536 / GetSystemMetrics(SM_CXSCREEN));
    int absY = pt.y * (65536 / GetSystemMetrics(SM_CYSCREEN));

    // Fill the INPUT structure
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = absX;
    input.mi.dy = absY;
    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

    // Send the mouse event
    SendInput(1, &input, sizeof(INPUT));
}

// Function to move the mouse using PostMessage
void MoveMousePostMessage(HWND hwnd, int x, int y)
{
    // Send the mouse move message
    PostMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
}

// Function to move the mouse using SendMessage
void MoveMouseSendMessage(HWND hwnd, int x, int y)
{
    // Send the mouse move message
    SendMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
}

// Function to click the mouse using SendInput relative to the window
void ClickMouseSendInput(HWND hwnd, int x, int y)
{
    // Convert window coordinates to screen coordinates
    POINT pt = {x, y};
    ClientToScreen(hwnd, &pt);

    // Convert coordinates to absolute values
    int absX = pt.x * (65536 / GetSystemMetrics(SM_CXSCREEN));
    int absY = pt.y * (65536 / GetSystemMetrics(SM_CYSCREEN));

    // Fill the INPUT structure for mouse down
    INPUT inputDown = {0};
    inputDown.type = INPUT_MOUSE;
    inputDown.mi.dx = absX;
    inputDown.mi.dy = absY;
    inputDown.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN;

    // Fill the INPUT structure for mouse up
    INPUT inputUp = {0};
    inputUp.type = INPUT_MOUSE;
    inputUp.mi.dx = absX;
    inputUp.mi.dy = absY;
    inputUp.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP;

    // Send the mouse events
    SendInput(1, &inputDown, sizeof(INPUT));
    SendInput(1, &inputUp, sizeof(INPUT));
}

// Function to click the mouse using PostMessage
void ClickMousePostMessage(HWND hwnd, int x, int y)
{
    // Send the mouse button down and up messages
    PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
    PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
}

// Function to click the mouse using SendMessage
void ClickMouseSendMessage(HWND hwnd, int x, int y)
{
    // Send the mouse button down and up messages
    SendMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
    SendMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
}

void PrintActiveWindowTitle() {
    HWND hwndActive = GetForegroundWindow(); // Получение дескриптора активного окна

    if (hwndActive) {
        char title[256];
        int length = GetWindowTextA(hwndActive, title, sizeof(title));
        
        if (length > 0) {
            title[length] = '\0'; // Обеспечение завершения строки нулем
            std::cout << "Active window title: hwnd "<< hwndActive << " name: " << title << std::endl;
        }
    }
}

void PrintWindowUnderCursorTitle() {
    // Получение текущих координат курсора
    POINT cursorPos;
    if (GetCursorPos(&cursorPos)) {
        // Получение дескриптора окна под курсором
        HWND hwndUnderCursor = WindowFromPoint(cursorPos);
        
        if (hwndUnderCursor) {
            char title[256];
            GetWindowTextA(hwndUnderCursor, title, sizeof(title));
            
            // Обеспечение завершения строки нулем
            title[sizeof(title) - 1] = '\0';
            std::cout << "window under cursor hwnd: " << hwndUnderCursor << " name: " << title << std::endl;
        }
    } else {
        std::cerr << "Failed to get cursor position." << std::endl;
    }
}

std::string getStringTime()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S");
    const std::string str = oss.str();
    return str;
}

std::pair<std::string, int> blockToString(tesseract::TessBaseAPI &ocr, const cv::Mat &img)
{
    ocr.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    ocr.SetImage(img.data, img.cols, img.rows, img.elemSize1() * img.channels(), img.step);

    char *reconizedStringPtr = ocr.GetUTF8Text();
    std::string reconizedString = std::string(reconizedStringPtr);

    int confidence = ocr.MeanTextConf();

    if (reconizedString.size() > 0)
        reconizedString.pop_back();

    delete[] reconizedStringPtr;
    return std::make_pair(reconizedString, confidence);
};