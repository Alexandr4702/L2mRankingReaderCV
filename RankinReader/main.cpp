#include <Windows.h>

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#include <string>
#include <fstream>
#include <format>
#include <unordered_map>
#include <vector>

#include <future>
#include <thread>
#include <chrono>

#include <iomanip>
#include <ctime>
#include <sstream>

using namespace std;
using namespace cv;

class TimeMeasure
{
public:
    TimeMeasure(const std::ostream &out = std::cout, double *save = nullptr) : m_out(out), m_save_diff(save)
    {
        m_save_diff = save;
        m_start = std::chrono::system_clock::now();
    }
    ~TimeMeasure()
    {
        m_stop = std::chrono::system_clock::now();
        auto diff = m_stop - m_start;
        if (m_save_diff)
            *m_save_diff = std::chrono::duration_cast<std::chrono::microseconds>(diff).count();
        std::cout << "Time in microsec: " << std::chrono::duration_cast<std::chrono::microseconds>(diff).count() << std::endl;
    }

private:
    double *m_save_diff = nullptr;
    const std::ostream &m_out;
    std::chrono::system_clock::time_point m_start;
    std::chrono::system_clock::time_point m_stop;
};

struct tessData
{
    string str;
    float conf = 0;
    Rect box;
    void operator=(const tessData &data)
    {
        if (data.conf > conf)
        {
            str = data.str;
            conf = data.conf;
            box = data.box;
        }
    }
};

struct PersonProperties
{
    string serverName;
    tessData rank, clan, Union;
};

BITMAPINFOHEADER createBitmapHeader(int width, int height)
{
    BITMAPINFOHEADER bi;

    // create a bitmap
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // this is the line that makes it draw upside down or not
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    return bi;
}

Mat captureScreenMat(HWND hwnd)
{
    Mat src;

    // get handles to a device context (DC)
    HDC hwindowDC = GetDC(hwnd);
    HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
    SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

    // define scale, height and width
    int screenx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screeny = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // create mat object
    src.create(height, width, CV_8UC4);

    // create a bitmap
    HBITMAP hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
    BITMAPINFOHEADER bi = createBitmapHeader(width, height);

    // use the previously created device context with the bitmap
    SelectObject(hwindowCompatibleDC, hbwindow);

    // copy from the window device context to the bitmap device context
    StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, screenx, screeny, width, height, SRCCOPY); // change SRCCOPY to NOTSRCCOPY for wacky colors !
    GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);          // copy from hwindowCompatibleDC to hbwindow

    // avoid memory leak
    DeleteObject(hbwindow);
    DeleteDC(hwindowCompatibleDC);
    ReleaseDC(hwnd, hwindowDC);

    return src;
}

string getBoxWord(tesseract::TessBaseAPI *ocr, const Mat &im, const cv::Rect &roi)
{
    ocr->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    Mat window = im(roi);
    ocr->SetImage(window.data, window.cols, window.rows, im.elemSize1() * im.channels(), im.step);
    char *word = ocr->GetUTF8Text();
    string str = string(word);
    str.pop_back();
    delete[] word;
    return str;
}

bool checkIfBoxContainWord(tesseract::TessBaseAPI *ocr, const Mat &im, const string &str, const cv::Rect &roi)
{
    ocr->SetPageSegMode(tesseract::PSM_SINGLE_WORD);
    Mat window = im(roi);
    ocr->SetImage(window.data, window.cols, window.rows, im.elemSize1() * im.channels(), im.step);
    const char *text = ocr->GetUTF8Text();
    std::cout << text << "\n";
    bool ret = strncmp(text, str.c_str(), str.length()) == 0;
    delete[] text;
    return ret;
}

bool checkIfVertCrosses(const Rect &a, const Rect &b)
{
    int32_t a_min = a.y;
    int32_t a_max = a.y + a.height;
    int32_t b_min = b.y;
    int32_t b_max = b.y + b.height;

    return (a_min <= b_max && b_min <= a_max);

    return (a_min >= b_min && a_min <= b_max) ||
           (a_max >= b_min && a_max <= b_max) ||

           (b_min >= a_min && b_min <= a_max) ||
           (b_max >= a_min && b_max <= a_max);

    // return !(a_max < b_min || a_min > b_max);

    // return (interval1.start >= interval2.start && end1 <= end2);
}

char readKeyPress()
{
    char key;
    std::cin >> key;
    return key;
}

string getStringTime()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S");
    const string str = oss.str();
    return str;
}

int main()
{
    // capture image
    HWND hwnd = GetDesktopWindow();

    const Rect RankingRoi(307, 392, 1338 - 307, 804 - 392);      // Corners х1 (307, 392) х4 (1338, 392)
    const Rect WindowNameRoi(1553, 52, 1744 - 1553, 131 - 52);   // Corners х1 (1553, 52)  х4 (1744, 131)
    const Rect ServerNameRoi(1055, 260, 1254 - 1055, 306 - 260); // Corners х1 (1011, 179)  х4 (1295, 233)

    const Rect RankingRoiCurrentRank(400, 408, 485 - 400, 796 - 408);
    const Rect RankingRoiName(604, 408, 905 - 604, 796 - 408);
    const Rect RankingRoiClan(1005, 408, 1319 - 1005, 796 - 408);

    const string RankingString = "Ranking";

    tesseract::TessBaseAPI *ocr_eng = new tesseract::TessBaseAPI();
    ocr_eng->Init("C:/msys64/mingw64/share/tessdata/", "eng", tesseract::OEM_LSTM_ONLY);

    tesseract::TessBaseAPI *ocr_eng_rus = new tesseract::TessBaseAPI();
    ocr_eng_rus->Init("C:/msys64/mingw64/share/tessdata/", "eng+rus", tesseract::OEM_LSTM_ONLY);

    const string dateString = getStringTime();

    ofstream out_debug("./log_" + dateString + ".txt");
    ofstream out("./servers_list_" + dateString + ".txt");

    // Name, Rank, Clan, Alians
    unordered_map<string, PersonProperties> persons;

    std::future<char> future_key = std::async(std::launch::async, readKeyPress);

    while (true)
    {
        TimeMeasure ms;

        Mat im = captureScreenMat(hwnd);
        cv::cvtColor(im, im, COLOR_BGR2GRAY);
        cv::threshold(im, im, 80, 255, THRESH_BINARY_INV);

        static bool imw = true;

        if (checkIfBoxContainWord(ocr_eng, im, RankingString, WindowNameRoi))
        {
            string serverName = getBoxWord(ocr_eng, im, ServerNameRoi);
            cout << "serverName: " << serverName << "\n";
            {
                Mat windowCurrentRanks = im(RankingRoiCurrentRank);
                Mat windowNames = im(RankingRoiName);
                Mat windowClans = im(RankingRoiClan);
                vector<tessData> s_Ranks;
                vector<tessData> Names;
                vector<tessData> ClansAlians;

                auto handleWindow = [&](tesseract::TessBaseAPI *ocr, string Name, Mat &window, vector<tessData> &save, int confLvl = 50)
                {
                    out_debug << Name << "\n";
                    ocr->SetPageSegMode(tesseract::PSM_SINGLE_COLUMN);
                    ocr->SetImage(window.data, window.cols, window.rows, im.elemSize1() * im.channels(), im.step);
                    ocr->Recognize(0);
                    tesseract::ResultIterator *ri = ocr->GetIterator();
                    tesseract::PageIteratorLevel level = tesseract::RIL_WORD;

                    if (ri != 0)
                    {
                        do
                        {
                            tessData data;
                            const char *word = ri->GetUTF8Text(level);
                            if (word == nullptr)
                            {
                                cout << "NullData \n";
                                continue;
                            }
                            int x1, y1, x2, y2;
                            ri->BoundingBox(level, &x1, &y1, &x2, &y2);
                            data.conf = ri->Confidence(level);
                            data.str = string(word);
                            data.box = Rect(x1, y1, x2 - x1, y2 - y1);
                            // out_debug << format("conf: {:10.5f}; BoundingBox: {:5d},{:5d},{:5d},{:5d}; word: {:30.50s}", data.conf, x1, y1, x2, y2, word) << endl;
                            if (data.conf > confLvl)
                            {
                                save.push_back(move(data));
                            }

                            delete[] word;
                        } while (ri->Next(level));
                    }
                };

                handleWindow(ocr_eng, "Ranks---------", windowCurrentRanks, s_Ranks, 85);
                handleWindow(ocr_eng_rus, "Names---------", windowNames, Names);
                handleWindow(ocr_eng_rus, "Clans---------", windowClans, ClansAlians);

                for (const auto &name : Names)
                {
                    persons[name.str].serverName = serverName;

                    for (const auto &rank : s_Ranks)
                    {
                        if (checkIfVertCrosses(name.box, rank.box))
                        {
                            persons[name.str].rank = rank;
                            break;
                        }
                    }

                    for (const auto &clanAli : ClansAlians)
                    {
                        if (checkIfVertCrosses(name.box, clanAli.box))
                        {
                            out_debug << format("name.box y1 {:5d} y2 {:5d} size {:5d} clanAli.box y1 {:5d} y2 {:5d} size {:5d} {}  {}  y1n-y1c: {:5d} ",
                                                name.box.y, name.box.y + name.box.height, name.box.height,
                                                clanAli.box.y, clanAli.box.y + clanAli.box.height, clanAli.box.height,
                                                name.str, clanAli.str, name.box.y - clanAli.box.y)
                                      << endl;

                            int y1n_y1c = name.box.y - clanAli.box.y;
                            if (abs(y1n_y1c) < 10 || y1n_y1c < -10)
                            {
                                persons[name.str].clan = clanAli;
                            }
                            else if (y1n_y1c > 10)
                            {
                                persons[name.str].Union = clanAli;
                            }
                        }
                    }
                }
            }

            if (imw)
            {
                rectangle(im, RankingRoi, Scalar(255, 0, 0));
                rectangle(im, WindowNameRoi, Scalar(255, 0, 0));
                rectangle(im, ServerNameRoi, Scalar(255, 0, 0));

                rectangle(im, RankingRoiCurrentRank, Scalar(255, 0, 0));
                rectangle(im, RankingRoiName, Scalar(255, 0, 0));
                rectangle(im, RankingRoiClan, Scalar(255, 0, 0));

                imwrite("./boxes.jpg", im);
                imw = false;
            }
        }

        if (future_key.wait_for(chrono::system_clock::duration::min()) == future_status::ready)
        {
            cout << future_key.get() << "\n";
            break;
        }
    }

    cout << "stop\n";

    out << "rank;ran_c;name;clan;clan_c;union;union_c;serverName" << endl;

    for (const auto &person : persons)
    {
        out << person.second.rank.str << ";" << person.second.rank.conf << ";";
        out << person.first << ";";
        out << person.second.clan.str << ";" << person.second.clan.conf << ";";
        out << person.second.Union.str << ";" << person.second.Union.conf << ";";
        out << person.second.serverName;
        out << endl;
    }

    out.close();
    out_debug.close();

    ocr_eng->End();
    ocr_eng_rus->End();
    return 0;
}