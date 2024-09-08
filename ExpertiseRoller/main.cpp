#include "Tools.h"
#include <boost/algorithm/string.hpp>
#include <fstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/detail/file_parser_error.hpp>

namespace pt = boost::property_tree;

class ColorDetector
{
private:
    static const std::array<cv::Scalar, 3> colors;
    static const std::array<std::string, 4> colorNames;

    static inline double ColorDistance(const cv::Scalar &a, const cv::Scalar &b)
    {
        return cv::norm(a - b);
    }

public:
    enum ColorIndex
    {
        PURPLE,
        GREEN,
        BLUE,
        UNDEFINED
    };

    static inline std::string IdxToStr(ColorIndex ind)
    {
        return colorNames[ind];
    }

    static ColorIndex detectColorIndex(const cv::Scalar &a)
    {
        // pair<ColorIndex, double>  |  ColorIndex, color distance between a and Colors[ColorIndex]
        auto cmp = [](const std::pair<ColorIndex, double> &a, const std::pair<ColorIndex, double> &b)
        { return a.second < b.second; };

        std::set<std::pair<ColorIndex, double>, decltype(cmp)> colorIndexSet;

        colorIndexSet.insert(std::pair<ColorIndex, double>(ColorIndex::PURPLE, ColorDistance(colors[ColorIndex::PURPLE], a)));
        colorIndexSet.insert(std::pair<ColorIndex, double>(ColorIndex::GREEN, ColorDistance(colors[ColorIndex::GREEN], a)));
        colorIndexSet.insert(std::pair<ColorIndex, double>(ColorIndex::BLUE, ColorDistance(colors[ColorIndex::BLUE], a)));

        return colorIndexSet.begin()->first;
    }
};

const std::array<cv::Scalar, 3> ColorDetector::colors = {
    cv::Scalar(80.1043, 18.2386, 38.3279, 0),
    cv::Scalar(34.7525, 54.0646, 12.7087, 0),
    cv::Scalar(84.3319, 36.4075, 16.8168, 0)};

const std::array<std::string, 4> ColorDetector::colorNames = {
    "PURPLE",
    "GREEN",
    "BLUE",
    "UNDEFINED"};

struct SquareProps
{
    ColorDetector::ColorIndex colorIdx;
    std::string propName;
    int propVal;
};

// Property rectangles row by row
const uint16_t SQUARE_SIZE_X = 198, SQUARE_SIZE_Y = 121;
const cv::Rect PROP_RECTS[] = {
    {314, 167, SQUARE_SIZE_X, SQUARE_SIZE_Y},
    {534, 167, SQUARE_SIZE_X, SQUARE_SIZE_Y},
    {754, 167, SQUARE_SIZE_X, SQUARE_SIZE_Y},

    {314, 310, SQUARE_SIZE_X, SQUARE_SIZE_Y},
    {534, 310, SQUARE_SIZE_X, SQUARE_SIZE_Y},
    {754, 310, SQUARE_SIZE_X, SQUARE_SIZE_Y},

    {314, 452, SQUARE_SIZE_X, SQUARE_SIZE_Y},
    {534, 452, SQUARE_SIZE_X, SQUARE_SIZE_Y},
    {754, 452, SQUARE_SIZE_X, SQUARE_SIZE_Y}};

std::vector<std::array<std::tuple<ColorDetector::ColorIndex, std::string, int>, 9>> DESIRED_RESULT;

/*
    std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),             // 0 upper left corner
    std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),             // 1 upper middle
    std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),             // 2 upper right corner
    std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),             // 3 middle left
    std::make_tuple(ColorDetector::ColorIndex::PURPLE, "stun resistance", 4), // 4 central stun resistance
    std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),             // 5 middle right
    std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),             // 6 lower left corner
    std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),             // 7 lower middle corner
    std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0)              // 8 lower right corner
*/

bool extractPropVal(const std::string &input, SquareProps &ret)
{
    std::string part1, part2;
    char delimiter = '+';
    size_t pos = input.find(delimiter);

    if (pos != std::string::npos)
    {
        ret.propName = input.substr(0, pos - 1);
        part2 = input.substr(pos + 1);
    }
    else
    {
        return false;
    }

    std::string numberStr;
    for (char c : part2)
    {
        if (std::isdigit(c))
        {
            numberStr += c;
        }
    }

    if (!numberStr.empty())
    {
        ret.propVal = std::stoi(numberStr);
    }

    return true;
}

bool getDesiredResultFromJson(const std::string &json,
                              std::vector<std::array<std::tuple<ColorDetector::ColorIndex, std::string, int>, 9>> &ret,
                              std::string &r_charName)
{
    pt::ptree SettingsTree;

    try
    {
        read_json(json, SettingsTree);
    }
    catch (...)
    {
        return false;
    }

    auto charName = SettingsTree.find("charName");
    auto expertisParametrs = SettingsTree.find("expertisParametrs");

    if (charName == SettingsTree.not_found() or expertisParametrs == SettingsTree.not_found())
        return false;

    r_charName = charName->second.get_value<std::string>();

    const std::vector<std::string> corners = {"UpLeft",
                                              "UpMid",
                                              "UpRight",
                                              "MidLeft",
                                              "MidMid",
                                              "MidRight",
                                              "DwnLeft",
                                              "DwnMid",
                                              "DwnRight"};

    auto expertisParametrsIt = expertisParametrs->second.begin();

    while (expertisParametrsIt != expertisParametrs->second.end())
    {
        std::array<std::tuple<ColorDetector::ColorIndex, std::string, int>, 9> data =
            {
                std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),
                std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),
                std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),
                std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),
                std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),
                std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),
                std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),
                std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0),
                std::make_tuple(ColorDetector::ColorIndex::UNDEFINED, "", 0)};

        for (int i = 0; i < 9; i++)
        {
            auto itCorner = expertisParametrsIt->second.find(corners[i]);

            if (itCorner == expertisParametrsIt->second.not_found())
                continue;

            auto color = itCorner->second.find("color");
            auto propName = itCorner->second.find("propertyName");
            auto val = itCorner->second.find("val");

            if (color == itCorner->second.not_found() or propName == itCorner->second.not_found() or val == itCorner->second.not_found())
                return false;

            std::get<0>(data[i]) = static_cast<ColorDetector::ColorIndex>(color->second.get_value<int>());
            std::get<1>(data[i]) = propName->second.get_value<std::string>();
            std::get<2>(data[i]) = val->second.get_value<int>();
        }

        ret.push_back(data);
        expertisParametrsIt = std::next(expertisParametrsIt, 1);
    }

    return true;
}

int main()
{
    using namespace cv;
    using namespace std;

    std::string charName_u8;

    bool jsonSucc = getDesiredResultFromJson("settings_ExpertiseRoller.json", DESIRED_RESULT, charName_u8);

    if (!jsonSucc)
    {
        cout << "Couldn't read json file \n";
        return -1;
    }
    const wstring winPrefix = L"Lineage2M l "s;

    wstring charName_u16 = utf8_to_wstring(charName_u8);

    wstring windowTitle = winPrefix + charName_u16;

    HWND hwnd = FindWindowW(NULL, windowTitle.c_str());
    // HWND hwnd = reinterpret_cast<HWND>(1642232);
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

    const string dateString = getStringTime();
    ofstream logFile(string("log_") + "_exp_roller_" + dateString + ".csv");

    tesseract::TessBaseAPI ocr_eng = tesseract::TessBaseAPI();
    ocr_eng.Init("./", "eng", tesseract::OEM_LSTM_ONLY);

    while (1)
    {
        // Will be switched to false in case of the fail
        bool recognitionSuccess = true;
        struct SquareProps Props[sizeof(PROP_RECTS) / sizeof(PROP_RECTS[0])];

        Mat screen = imageGetter.captureImage(); // CV_8UC3
        if (screen.empty() or screen.cols != 1600 or screen.rows != 900)
        {
            Beep(2000, 500);
            std::cerr << "Failed to capture image or image size is not 1600x900!" << std::endl;
            continue;
        }
        else
        {
            for (int i = 0; i < 9; i++)
            {
                Mat propSquare = screen(PROP_RECTS[i]);
                Scalar color = mean(propSquare);
                Props[i].colorIdx = ColorDetector::detectColorIndex(color);

                cvtColor(propSquare, propSquare, COLOR_BGR2GRAY);
                threshold(propSquare, propSquare, 70, 255, THRESH_BINARY);
                // blur(propSquare, propSquare, Size(2, 2));

                // Mask lock to avoid false recognition
                const Rect lockRect = {167, 90, 31, 31};
                rectangle(propSquare, lockRect, Scalar(0, 0, 0), FILLED);

                auto [propStr, confidence] = blockToString(ocr_eng, propSquare);

                replace(propStr.begin(), propStr.end(), '\n', ' ');
                boost::algorithm::to_lower(propStr);

                extractPropVal(propStr, Props[i]);

                // cout << ColorDetector::IdxToStr(Props[i].colorIdx) << " " << i << " ";
                cout << i << " ";
                cout << ColorDetector::IdxToStr(Props[i].colorIdx) << " ";
                cout << Props[i].propName << " ";
                cout << confidence << " ";
                cout << Props[i].propVal << " ";
                cout << endl;

                // imshow(string("prop_") + to_string(i), propSquare);
                if (confidence < 80)
                {
                    recognitionSuccess = false;
                    // break;
                }
            }
            // imshow("screen", screen);
        }

        if (recognitionSuccess)
        {
            bool needRoll = true;

            for (int i = 0; i < DESIRED_RESULT.size(); i++)
            {
                bool requiredRoll = false;
                const auto desiredProps = DESIRED_RESULT[i];

                for (int j = 0; j < 9; j++)
                {
                    const auto &desiredColorIdx = get<0>(desiredProps[j]);
                    const auto &desiredPropName = get<1>(desiredProps[j]);
                    const auto &desiredPropVal = get<2>(desiredProps[j]);

                    if ((desiredColorIdx != ColorDetector::ColorIndex::UNDEFINED && desiredColorIdx != Props[j].colorIdx) ||
                        (!desiredPropName.empty() && (desiredPropName != Props[j].propName or desiredPropVal > Props[j].propVal)))
                    {
                        requiredRoll = true;
                        break;
                    }
                }
                needRoll &= requiredRoll;
            }

            for (int i = 0; i < 9; i++)
            {
                logFile << i << ";";
                logFile << ColorDetector::IdxToStr(Props[i].colorIdx) << ";";
                logFile << Props[i].propName << ";";
                logFile << Props[i].propVal << ";";
            }
            logFile << endl;

            if (needRoll)
            {
                sendKeystroke(hwnd, 'Y');
                Sleep(200);
                sendKeystroke(hwnd, 'Y');
            }
            else
            {
                Beep(1000, 500);
                return 0;
            }
        }
        else
        {
            cout << "Recognition failed\n";
        }

        cout << "---------------------------------------------------------------------------" << endl;
        Sleep(1500);
    }

    return 0;
}
