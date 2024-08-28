#include <iostream>
#include "Tools.h"
#include <tesseract/baseapi.h>
#include <boost/algorithm/string.hpp>
#include <fstream>

int main()
{
    using namespace cv;
    using namespace std;

    auto windowTitle = L"Lineage2M l KanunJarrus"s;
    HWND hwnd = FindWindowW(NULL, windowTitle.c_str());
    // hwnd = reinterpret_cast<HWND>(3213236);
    std::cout << hwnd << std::endl;

    if (!hwnd)
    {
        std::cerr << "Window not found!" << std::endl;
        return -1;
    }

    ImageGetter imageGetter;

    if (!imageGetter.initialize(hwnd))
    {
        return -1;
    }

    tesseract::TessBaseAPI ocr_eng = tesseract::TessBaseAPI();
    ocr_eng.Init("C:/msys64/mingw64/share/tessdata/", "eng", tesseract::OEM_LSTM_ONLY);

    const Rect propName1Rct(611, 421, 280, 24);
    const Rect propName2Rct(611, 446, 280, 24);
    const Rect propName3Rct(611, 472, 280, 24);

    const Rect propValue1Rct(891, 421, 105, 24);
    const Rect propValue2Rct(891, 446, 105, 24);
    const Rect propValue3Rct(891, 472, 105, 24);

    const string dateString = getStringTime();
    ofstream logFile("log_" + dateString + ".csv");

    logFile << "ParamName1;ParamVal1;ParamName2;ParamVal2;ParamName3;ParamVal3\n";

    while (true)
    {
        Mat screen = imageGetter.captureImage();
        if (screen.empty() or screen.cols != 1600 or screen.rows != 900)
        {
            std::cerr << "Failed to capture image or image size is not 1600x900!" << std::endl;
            break;
        }

        cv::cvtColor(screen, screen, COLOR_BGR2GRAY);
        // cv::threshold(screen, screen, 40, 255, THRESH_BINARY_INV);
        // cv::blur(screen, screen, Size(2, 2));

        auto recognizeImage = [&](const Rect &propNameRect, const Rect &propValueRect, const char *winName, pair<string, double> &result) -> bool
        {
            Mat propNameImage = screen(propNameRect);
            Mat propValueImage = screen(propValueRect);

            auto blockToString = [&screen](tesseract::TessBaseAPI &ocr, const Mat &img) -> string
            {
                ocr.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
                ocr.SetImage(img.data, img.cols, img.rows, img.elemSize1() * img.channels(), img.step);

                char *reconizedStringPtr = ocr.GetUTF8Text();
                string reconizedString = string(reconizedStringPtr);

                if (reconizedString.size() > 0)
                    reconizedString.pop_back();

                delete[] reconizedStringPtr;
                return reconizedString;
            };

            result.first = blockToString(ocr_eng, propNameImage);
            string propValStr = blockToString(ocr_eng, propValueImage);

            string propNameWinName = string("propName") + winName;
            string propValueWinName = string("propValue") + winName;

            imshow(propNameWinName, propNameImage);
            imshow(propValueWinName, propValueImage);
            boost::algorithm::to_lower(result.first);
            boost::algorithm::to_lower(propValStr);

            try
            {
                result.second = std::stof(propValStr);
            }
            catch (...)
            {
                return false;
            }

            cout << result.first << " " << propValStr << "\n";
            cout << result.first << " val: " << result.second << "\n";

            return true;
        };

        cout << "New attempt\n \n";
        pair<string, double> props[3];

        bool result = recognizeImage(propName1Rct, propValue1Rct, "1", props[0]) and
                      recognizeImage(propName2Rct, propValue2Rct, "2", props[1]) and
                      recognizeImage(propName3Rct, propValue3Rct, "3", props[2]);

        if (result)
        {
            // string reqParam = "skill damage boost";
            string reqParam = "defense";
            int reqVal = 2;
            int val = 0;

            val += props[0].first == reqParam ? props[0].second : 0;
            val += props[1].first == reqParam ? props[1].second : 0;
            val += props[2].first == reqParam ? props[2].second : 0;

            logFile << props[0].first << ";" << props[0].second << ";";
            logFile << props[1].first << ";" << props[1].second << ";";
            logFile << props[2].first << ";" << props[2].second;
            logFile << endl;

            if (val >= reqVal)
            {
                Beep(1000, 1000);
            }
            else
            {
                sendKeystroke(hwnd, 'Y');
            }
        }
        else
        {
            cout << "Image is not recognized \n";
        }

        if (cv::waitKey(1500) >= 0)
            break;
    }

    logFile.close();
    ocr_eng.End();

    return 0;
}
