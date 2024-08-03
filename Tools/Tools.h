#include <chrono>
#include <iostream>
#include <vector>
#include <windows.h>
#include <opencv2/opencv.hpp>

class TimeMeasure
{
public:
    TimeMeasure(const std::ostream &out = std::cout, double *save = nullptr);
    ~TimeMeasure();

private:
    double *m_save_diff = nullptr;
    const std::ostream &m_out;
    std::chrono::system_clock::time_point m_start;
    std::chrono::system_clock::time_point m_stop;
};

class ImageGetter
{
public:
    ImageGetter(); // Default constructor
    ~ImageGetter();
    bool initialize(const char *windowTitle); // Initialize with window title
    bool initialize(HWND m_hwnd);             // Initialize with window title
    cv::Mat captureImage();                   // Capture image from the window

private:
    HWND m_hwnd;
    HDC m_hWindowDC;
    HDC m_hMemDC;
    HBITMAP m_hBitmap;
    RECT m_rect;
    int m_prevWidth;
    int m_prevHeight;

    void updateSize();                     // Update window size and bitmap if necessary
    cv::Mat HBitmapToMat(HBITMAP hBitmap); // Convert HBITMAP to cv::Mat
};

class ConsoleEncodingSwitcher
{
public:
    ConsoleEncodingSwitcher(UINT newCP);
    ~ConsoleEncodingSwitcher();

private:
    UINT originalCP;
    UINT originalInputCP;
};

class WindowLister
{
public:
    struct WindowInfo
    {
        HWND hwnd;
        std::string title;
    };

    WindowLister() = default;

    void RefreshWindowList()
    {
        m_windows.clear();
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(this));
    }

    const std::vector<WindowInfo> &GetWindows() const
    {
        return m_windows;
    }

private:
    std::vector<WindowInfo> m_windows;
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
};

void sendKeystroke(HWND hwnd, char key);
void PrintActiveWindowTitle();
void PrintWindowUnderCursorTitle();

void MoveMouseSendInput(HWND hwnd, int x, int y);
void MoveMousePostMessage(HWND hwnd, int x, int y);
void MoveMouseSendMessage(HWND hwnd, int x, int y);

void ClickMouseSendInput(HWND hwnd, int x, int y);
void ClickMousePostMessage(HWND hwnd, int x, int y);
void ClickMouseSendMessage(HWND hwnd, int x, int y);

