#include <Windows.h>

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#include <string>
#include <fstream>
#include <format>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace cv;

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

    cout << a_min << " " << a_max << " " << b_min << " " << b_max << "  " << static_cast<int>((a_min >= b_min && a_min <= b_max) || (a_max >= b_min && a_max <= b_max)) << "\n";

    return (a_min <= b_max && b_min <= a_max);

    return (a_min >= b_min && a_min <= b_max) ||
           (a_max >= b_min && a_max <= b_max) ||

           (b_min >= a_min && b_min <= a_max) ||
           (b_max >= a_min && b_max <= a_max)
           ;

    // return !(a_max < b_min || a_min > b_max);

    // return (interval1.start >= interval2.start && end1 <= end2);
}

int main()
{
    // capture image
    HWND hwnd = GetDesktopWindow();

    const Rect RankingRoi(307, 392, 1338 - 307, 804 - 392);      // Corners х1 (307, 392) х4 (1338, 392)
    const Rect WindowNameRoi(1553, 52, 1744 - 1553, 131 - 52);   // Corners х1 (1553, 52)  х4 (1744, 131)
    const Rect ServerNameRoi(1055, 260, 1254 - 1055, 306 - 260); // Corners х1 (1011, 179)  х4 (1295, 233)

    const Rect RankingRoiCurrentRank(404, 408, 480 - 404, 796 - 408);
    const Rect RankingRoiName(604, 408, 905 - 604, 796 - 408);
    const Rect RankingRoiClan(1005, 408, 1319 - 1005, 796 - 408);

    const string RankingString = "Ranking";

    tesseract::TessBaseAPI *ocr_eng = new tesseract::TessBaseAPI();
    ocr_eng->Init("C:/msys64/mingw64/share/tessdata/", "eng", tesseract::OEM_LSTM_ONLY);

    tesseract::TessBaseAPI *ocr_eng_rus = new tesseract::TessBaseAPI();
    ocr_eng_rus->Init("C:/msys64/mingw64/share/tessdata/", "eng+rus", tesseract::OEM_LSTM_ONLY);

    ofstream out_debug("./test.txt");
    ofstream out("./out.csv");

    // Name, Rank, Clan
    unordered_map<string, pair<string, string>> persons;

    while (true)
    {
        Mat im = captureScreenMat(hwnd);
        cv::cvtColor(im, im, COLOR_BGR2GRAY);
        static bool imw = true;

        if (checkIfBoxContainWord(ocr_eng, im, RankingString, WindowNameRoi))
        {
            string serverName = getBoxWord(ocr_eng, im, ServerNameRoi);
            cout << "serverName: " << serverName << "\n";
            {
                tesseract::TessBaseAPI *ocr = ocr_eng_rus;
                Mat windowCurrentRanks = im(RankingRoiCurrentRank);
                Mat windowNames = im(RankingRoiName);
                Mat windowClans = im(RankingRoiClan);
                vector<pair<string, Rect>> s_Ranks;
                vector<pair<string, Rect>> Names;
                vector<pair<string, Rect>> ClansAlians;

                auto handleWindow = [&](string Name, Mat &window, vector<pair<string, Rect>> &save)
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
                            pair<string, Rect> data;
                            const char *word = ri->GetUTF8Text(level);
                            float conf = ri->Confidence(level);
                            // if (conf < 85)
                                // continue;
                            int x1, y1, x2, y2;
                            ri->BoundingBox(level, &x1, &y1, &x2, &y2);
                            data.first = string(word);
                            data.second = Rect(x1, y1, x2 - x1, y2 - y1);
                            save.push_back(move(data));

                            out_debug << format("conf: {:10.5f}; BoundingBox: {:5d},{:5d},{:5d},{:5d}; word: '{:30.50s}'\n", conf, x1, y1, x2, y2, word) << endl;
                            delete[] word;
                        } while (ri->Next(level));
                    }
                };

                handleWindow("Ranks---------", windowCurrentRanks, s_Ranks);
                handleWindow("Names---------", windowNames, Names);
                handleWindow("Clans---------", windowClans, ClansAlians);

                for (const auto &name : Names)
                {
                    auto person = persons.find(name.first);
                    if (person == persons.end())
                    {
                        for (const auto &rank : s_Ranks)
                        {
                            if (checkIfVertCrosses(name.second, rank.second))
                            {
                                persons[name.first].first = rank.first;
                                break;
                            }
                        }

                        for (const auto &clanAli : ClansAlians)
                        {
                            if (checkIfVertCrosses(name.second, clanAli.second))
                            {
                                persons[name.first].second += ((persons[name.first].second.length())? ";":"") + clanAli.first;
                            }
                        }
                        out << persons[name.first].first << ";" << name.first << ";" << persons[name.first].second << endl;
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
    }

    ocr_eng->End();
    ocr_eng_rus->End();
    return 0;
}