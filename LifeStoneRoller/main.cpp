#include <iostream>
#include "Tools.h"
#include <tesseract/baseapi.h>

int main()
{
    using namespace cv;
    using namespace std;

    const char *windowTitle = "Lineage2M l KanunJarrus";
    HWND hwnd = FindWindowA(NULL, windowTitle);
    // HWND hwnd = reinterpret_cast<HWND>(1642232);
    std::cout << hwnd << std::endl;

    if (!hwnd)
    {
        std::cerr << "Window not found!" << std::endl;
        return false;
    }

    ImageGetter imageGetter;

    if (!imageGetter.initialize(hwnd))
    {
        return -1;
    }

    tesseract::TessBaseAPI ocr_eng = tesseract::TessBaseAPI();
    ocr_eng.Init("C:/msys64/mingw64/share/tessdata/", "eng", tesseract::OEM_LSTM_ONLY);

    tesseract::TessBaseAPI ocr_eng_rus = tesseract::TessBaseAPI();
    ocr_eng_rus.Init("C:/msys64/mingw64/share/tessdata/", "eng+rus", tesseract::OEM_LSTM_ONLY);

    tesseract::TessBaseAPI ocr_rus = tesseract::TessBaseAPI();
    ocr_rus.Init("C:/msys64/mingw64/share/tessdata/", "rus", tesseract::OEM_LSTM_ONLY);

    const Rect propName1Rct(611, 421, 280, 24);
    const Rect propName2Rct(611, 446, 280, 24);
    const Rect propName3Rct(611, 472, 280, 24);

    const Rect propValue1Rct(891, 421, 105, 24);
    const Rect propValue2Rct(891, 446, 105, 24);
    const Rect propValue3Rct(891, 472, 105, 24);

    ConsoleEncodingSwitcher a(CP_UTF8);

    while (true)
    {
        Mat screen = imageGetter.captureImage();
        if (screen.empty() && screen.cols != 1600 && screen.rows != 900)
        {
            std::cerr << "Failed to capture image!" << std::endl;
            break;
        }

        cv::cvtColor(screen, screen, COLOR_BGR2GRAY);
        cv::threshold(screen, screen, 50, 255, THRESH_BINARY_INV);

        auto recognizeImage = [&](const Rect &propNameRect, const Rect &propValueRect, const char *winName) -> pair<string, double>
        {
            Mat propNameImage = screen(propNameRect);
            Mat propValueImage = screen(propValueRect);

            auto blockToString = [&screen](tesseract::TessBaseAPI &ocr, const Mat &img) -> string
            {
                ocr.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
                ocr.SetImage(img.data, img.cols, img.rows, img.elemSize1() * img.channels(), img.step);

                char *reconizedStringPtr = ocr.GetUTF8Text();
                string reconizedString = string(reconizedStringPtr);
                reconizedString.pop_back();
                delete[] reconizedStringPtr;

                return reconizedString;
            };

            string propName = blockToString(ocr_rus, propNameImage);
            string propValStr = blockToString(ocr_eng, propValueImage);
            cout << propName << " " << propValStr << "\n";

            string propNameWinName = string("propName") + winName;
            string propValueWinName = string("propValue") + winName;

            imshow(propNameWinName, propNameImage);
            imshow(propValueWinName, propValueImage);

            return make_pair<string, double>("ad", 0);
        };

        cout << "New attempt\n \n";
        recognizeImage(propName1Rct, propValue1Rct, "1");
        recognizeImage(propName2Rct, propValue2Rct, "2");
        recognizeImage(propName3Rct, propValue3Rct, "3");
        // sendKeystroke(hwnd, 'Y');

        if (cv::waitKey(1500) >= 0)
            break;
    }

    ocr_eng.End();
    ocr_eng_rus.End();
    ocr_rus.End();

    return 0;
}
