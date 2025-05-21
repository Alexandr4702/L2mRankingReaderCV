#include <algorithm>
#include <cstdint>
#include <iostream>
#include "Tools.h"
#include <tesseract/baseapi.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <string>
#include <array>
#include <vector>

const int MIN_CONFIDENCE = 75;

// For 1600x900
// const cv::Rect propName1Rct(590, 419, 280, 24);
// const cv::Rect propName2Rct(590, 446, 280, 24);
// const cv::Rect propName3Rct(590, 472, 280, 24);
// const cv::Rect propValue1Rct(891, 421, 150, 24);
// const cv::Rect propValue2Rct(891, 446, 150, 24);
// const cv::Rect propValue3Rct(891, 472, 150, 24);
// std::pair<uint32_t, uint32_t> REQUIRED_RESOLUTION = {1600, 900};

// For 1920x1120 HUD 100
const cv::Rect propName1Rct(645, 513, 437, 35);
const cv::Rect propName2Rct(645, 559, 437, 35);
const cv::Rect propName3Rct(645, 602, 437, 35);
const cv::Rect propValue1Rct(1082, 513, 227, 35);
const cv::Rect propValue2Rct(1082, 559, 227, 35);
const cv::Rect propValue3Rct(1082, 602, 227, 35);

// Width x Height
std::pair<uint32_t, uint32_t> REQUIRED_RESOLUTION = {1920, 1120};

const std::wstring CHAR_NAME = L"ВсемПривет";

std::vector<std::array<std::pair<std::string, double>, 3>> REQUIRED_VALUES = 
{
{
        {
            {"", 0},
            {"", 0}, 
            {"weapon damage boost", 15}
        }
    }
};

bool CheckProperties(const std::vector<std::array<std::pair<std::string, double>, 3>>& requiredValues,
                     const std::array<std::pair<std::string, double>, 3>& props) 
{
    for (const auto& reqArray : requiredValues) {
        bool allMatch = true;
        
        for (size_t i = 0; i < 3; ++i) {
            const auto& req = reqArray[i];
            const auto& prop = props[i];
            
            if (req.first.empty()) {
                continue;
            }
            
            if (prop.first != req.first || prop.second < req.second) {
                allMatch = false;
                break;
            }
        }
        
        if (allMatch) {
            return true;
        }
    }
    
    return false;
}

int main()
{
    using namespace cv;
    using namespace std;

    auto windowTitle = L"Lineage2M l " + CHAR_NAME;
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

    const string dateString = getStringTime();
    ofstream logFile("log_" + dateString + ".csv");

    logFile << "ParamName1;ParamVal1;ParamName2;ParamVal2;ParamName3;ParamVal3\n";

    while (true)
    {
        Mat screen = imageGetter.captureImage();
        if (screen.empty() or screen.cols != REQUIRED_RESOLUTION.first or screen.rows != REQUIRED_RESOLUTION.second)
        {
            std::cerr << "Failed to capture image or image size is not " << REQUIRED_RESOLUTION.first << "x" << REQUIRED_RESOLUTION.second << "!\n";
            break;
        }

        cv::cvtColor(screen, screen, COLOR_BGR2GRAY);
        // cv::threshold(screen, screen, 40, 255, THRESH_BINARY_INV);
        // cv::blur(screen, screen, Size(2, 2));

        threshold(screen, screen, 50, 255, THRESH_BINARY);

        auto recognizeImage = [&](const Rect &propNameRect, const Rect &propValueRect, const char *winName, pair<string, double> &result) -> bool
        {
            Mat propNameImage = screen(propNameRect);
            Mat propValueImage = screen(propValueRect);

            const auto decoded_props_name = blockToString(ocr_eng, propNameImage);
            ocr_eng.SetVariable("tessedit_char_whitelist", "+%0123456789");
            const auto decoded_props_val = blockToString(ocr_eng, propValueImage);
            ocr_eng.SetVariable("tessedit_char_whitelist", "");

            result.first = decoded_props_name.first;
            string propValStr = decoded_props_val.first;
            int confidence = std::min(decoded_props_name.second, decoded_props_val.second);

            string propNameWinName = string("propName") + winName;
            string propValueWinName = string("propValue") + winName;

            imshow(propNameWinName, propNameImage);
            imshow(propValueWinName, propValueImage);
            boost::algorithm::to_lower(result.first);
            boost::algorithm::to_lower(propValStr);
    
            cout << result.first << " " << propValStr << "\n";

            propValStr.erase(std::remove_if(propValStr.begin(), propValStr.end(),
                        [](char c) { return !std::isdigit(static_cast<unsigned char>(c)); }),
                        propValStr.end());

            bool ret = true;
            if (confidence < MIN_CONFIDENCE)
            {
                cout << "Low confidence: " << confidence << "\n";
                ret = false;
            }
            try
            {
                result.second = std::stof(propValStr);
            }
            catch (...)
            {
                ret = false;
            }

            cout << result.first << " " << propValStr << "\n";
            cout << result.first << " val: " << result.second << "\n";
            cout << "Confidence: " << decoded_props_name.second << " "
                 << decoded_props_val.second << " MinConf: " << confidence
                 << "\n";

            return ret;
        };

        cout << "New attempt\n \n";
        // pair<string, double> props[3];
        std::array<std::pair<std::string, double>, 3> props;

        bool result = true;

        result &= recognizeImage(propName1Rct, propValue1Rct, "1", props[0]);
        result &= recognizeImage(propName2Rct, propValue2Rct, "2", props[1]);
        result &= recognizeImage(propName3Rct, propValue3Rct, "3", props[2]);

        if (result)
        {
            if (CheckProperties(REQUIRED_VALUES, props))
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
