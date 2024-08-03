#include "Tools.h"

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

    updateSize();

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

    updateSize();

    SelectObject(m_hMemDC, m_hBitmap);
    BitBlt(m_hMemDC, 0, 0, m_rect.right - m_rect.left, m_rect.bottom - m_rect.top, m_hWindowDC, 0, 0, SRCCOPY);
    return HBitmapToMat(m_hBitmap);
}

ConsoleEncodingSwitcher::ConsoleEncodingSwitcher(UINT newCP)
{
    originalCP = GetConsoleOutputCP();
    originalInputCP = GetConsoleCP();

    SetConsoleOutputCP(newCP);
    SetConsoleCP(newCP);
}

ConsoleEncodingSwitcher::~ConsoleEncodingSwitcher()
{
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
    //     FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    //     NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    //     (LPSTR)&errorMessage, 0, NULL);
    std::cerr << "Failed to send WM_KEYDOWN to window. Error: " << error << std::endl;
}

void sendKeystroke(HWND hwnd, char key)
{
    UINT scanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);

    LPARAM lParamDown = 1 | (scanCode << 16);
    LPARAM lParamUp = 1 | (scanCode << 16) | (1 << 30) | (1 << 31);
    DWORD currentTime = GetTickCount();

    std::cout << "KeyDOWN \n";
    if (!PostMessageA (hwnd, WM_KEYDOWN, key, lParamDown))
    {
        std::cerr << "PostMessage failed. Error: " << GetLastError() << std::endl;
    }

    std::cout << "KeyUP \n";
    if (!PostMessageA (hwnd, WM_KEYUP, key, lParamUp))
    {
        std::cerr << "PostMessage failed. Error: " << GetLastError() << std::endl;
    }
}

void MoveMouseSendInput(HWND hwnd, int x, int y)
{
    POINT pt = {x, y};
    ClientToScreen(hwnd, &pt);

    int absX = pt.x * (65536 / GetSystemMetrics(SM_CXSCREEN));
    int absY = pt.y * (65536 / GetSystemMetrics(SM_CYSCREEN));

    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = absX;
    input.mi.dy = absY;
    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

    SendInput(1, &input, sizeof(INPUT));
}

void MoveMousePostMessage(HWND hwnd, int x, int y)
{
    PostMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
}

void MoveMouseSendMessage(HWND hwnd, int x, int y)
{
    SendMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
}

void ClickMouseSendInput(HWND hwnd, int x, int y)
{
    POINT pt = {x, y};
    ClientToScreen(hwnd, &pt);

    int absX = pt.x * (65536 / GetSystemMetrics(SM_CXSCREEN));
    int absY = pt.y * (65536 / GetSystemMetrics(SM_CYSCREEN));

    INPUT inputDown = {0};
    inputDown.type = INPUT_MOUSE;
    inputDown.mi.dx = absX;
    inputDown.mi.dy = absY;
    inputDown.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN;

    INPUT inputUp = {0};
    inputUp.type = INPUT_MOUSE;
    inputUp.mi.dx = absX;
    inputUp.mi.dy = absY;
    inputUp.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP;

    SendInput(1, &inputDown, sizeof(INPUT));
    SendInput(1, &inputUp, sizeof(INPUT));
}

void ClickMousePostMessage(HWND hwnd, int x, int y)
{
    PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
    PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
}

void ClickMouseSendMessage(HWND hwnd, int x, int y)
{
    SendMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
    SendMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
}

void PrintActiveWindowTitle() {
    HWND hwndActive = GetForegroundWindow();

    if (hwndActive) {
        char title[256];
        int length = GetWindowTextA(hwndActive, title, sizeof(title));
        
        if (length > 0) {
            title[length] = '\0';
            std::cout << "Active window title: hwnd "<< hwndActive << " name: " << title << std::endl;
        }
    }
}

void PrintWindowUnderCursorTitle() {

    POINT cursorPos;
    if (GetCursorPos(&cursorPos)) {
        HWND hwndUnderCursor = WindowFromPoint(cursorPos);
        
        if (hwndUnderCursor) {
            char title[256];
            GetWindowTextA(hwndUnderCursor, title, sizeof(title));
            
            title[sizeof(title) - 1] = '\0';
            std::cout << "window under cursor hwnd: " << hwndUnderCursor << " name: " << title << std::endl;
        }
    } else {
        std::cerr << "Failed to get cursor position." << std::endl;
    }
}