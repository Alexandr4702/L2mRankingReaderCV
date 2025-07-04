#include <algorithm>
#include <array>
#include <atomic>
#include <conio.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <ranges>
#include <string>
#include <synchapi.h>
#include <thread>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <tesseract/baseapi.h>

#include "Tools.h"

const bool SHOW_WINDOWS = false;
const uint32_t PAUSE_MS = 1500;
const int MIN_CONFIDENCE = 75;

// For 1600x900 HUD 75?
// const cv::Rect propName1Rct(590, 419, 280, 24);
// const cv::Rect propName2Rct(590, 446, 280, 24);
// const cv::Rect propName3Rct(590, 472, 280, 24);
// const cv::Rect propValue1Rct(891, 421, 150, 24);
// const cv::Rect propValue2Rct(891, 446, 150, 24);
// const cv::Rect propValue3Rct(891, 472, 150, 24);
// std::pair<uint32_t, uint32_t> REQUIRED_RESOLUTION = {1600, 900};

// For 1600x900 HUD 100
const cv::Rect propName1Rct(530, 410, 411, 32);
const cv::Rect propName2Rct(530, 447, 411, 32);
const cv::Rect propName3Rct(530, 482, 411, 32);
const cv::Rect propValue1Rct(941, 410, 153, 32);
const cv::Rect propValue2Rct(941, 447, 153, 32);
const cv::Rect propValue3Rct(941, 482, 153, 32);
std::pair<uint32_t, uint32_t> REQUIRED_RESOLUTION = {1600, 900};

// For 1920x1120 HUD 100
// const cv::Rect propName1Rct(645, 513, 437, 35);
// const cv::Rect propName2Rct(645, 559, 437, 35);
// const cv::Rect propName3Rct(645, 602, 437, 35);
// const cv::Rect propValue1Rct(1082, 513, 227, 35);
// const cv::Rect propValue2Rct(1082, 559, 227, 35);
// const cv::Rect propValue3Rct(1082, 602, 227, 35);
// std::pair<uint32_t, uint32_t> REQUIRED_RESOLUTION = {1920, 1120}; // Width x Height

const std::string TESSARACT_LOCATION = "./";

namespace pt = boost::property_tree;

class ExpertiseConfigLoader
{
  public:
    using Property = std::pair<std::string, double>;
    using PropertySet = std::array<Property, 3>;
    using ConfigList = std::vector<PropertySet>;

    struct ConfigData
    {
        std::wstring characterName;
        ConfigList configurations;
    };

    enum class LoadStatus
    {
        Success,
        FileNotFound,
        InvalidFormat,
        InvalidData
    };

    LoadStatus load(const std::string &filePath)
    {
        std::error_code ec;
        pt::ptree tree;

        try
        {
            pt::read_json(filePath, tree);
        }
        catch (const pt::json_parser_error &e)
        {
            m_lastError = "JSON parse error: " + std::string(e.what());
            return LoadStatus::InvalidFormat;
        }

        try
        {
            m_config = parseConfig(tree);
            return LoadStatus::Success;
        }
        catch (const std::exception &e)
        {
            m_lastError = "Config error: " + std::string(e.what());
            return LoadStatus::InvalidData;
        }
    }

    const std::wstring &getCharacterName() const noexcept
    {
        return m_config.characterName;
    }

    const ConfigList &getConfigurations() const noexcept
    {
        return m_config.configurations;
    }

    const std::string &getLastError() const noexcept
    {
        return m_lastError;
    }

    bool isLoaded() const noexcept
    {
        return !m_config.characterName.empty();
    }

  private:
    ConfigData m_config;
    std::string m_lastError;

    ConfigData parseConfig(const pt::ptree &tree)
    {
        ConfigData result;

        if (auto charName = tree.get_optional<std::string>("charName"))
        {
            result.characterName = utf8_to_wstring(*charName);
        }
        else
        {
            throw std::runtime_error("Missing 'charName' field");
        }

        if (auto lifeStoneParams = tree.get_child_optional("LifeStoneParametrs"))
        {
            for (const auto &lsArray : *lifeStoneParams)
            {
                PropertySet config;
                size_t i = 0;

                for (const auto &prop : lsArray.second)
                {
                    if (i >= 3)
                        break;
                    config[i++] = {prop.second.get<std::string>("propeName", ""),
                                   prop.second.get<double>("propVal", 0.0)};
                }
                result.configurations.push_back(config);
            }
        }

        return result;
    }

    static std::wstring utf8_to_wstring(const std::string &str)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
    }
};

std::ostream &operator<<(std::ostream &os, const ExpertiseConfigLoader::PropertySet &props)
{
    for (const auto &[name, value] : props)
    {
        os << "\"" << name << "\": " << value << "\n";
    }
    return os;
}

/**
 * @brief Checks if current properties match any required configuration
 * @param requiredValues List of allowed {propertyName, minValue} configurations
 * @param props Current properties to check {propertyName, value} pairs
 * @return true if props meet ALL non-empty requirements of ANY configuration,
 *         false otherwise
 * @note - Empty propertyNames in config are ignored (wildcards)
 *       - PropertyNames are case-sensitive
 *       - Values must be >= required minimum
 */
bool CheckProperties(const ExpertiseConfigLoader::ConfigList &requiredValues,
                     const ExpertiseConfigLoader::PropertySet &props)
{
    for (const auto &reqConfig : requiredValues)
    {
        bool allMatch = true;

        for (const auto &[req, prop] : std::views::zip(reqConfig, props))
        {
            if (!req.first.empty())
            { // Only check non-wildcard properties
                if (prop.first != req.first || prop.second < req.second)
                {
                    allMatch = false;
                    break;
                }
            }
        }

        if (allMatch)
        {
            return true; // All non-empty properties in this config matched
        }
    }

    return false; // No matching config found
}

int main()
{
    using namespace cv;
    using namespace std;

    ExpertiseConfigLoader config;
    auto config_load_result = config.load("settings_lifeStoneRoller.json");
    if (config_load_result != ExpertiseConfigLoader::LoadStatus::Success)
    {
        cout << config.getLastError() << endl;
        return 0;
    }

    const auto &charName = config.getCharacterName();
    const auto &requiredValues = config.getConfigurations();

    wcout << L"Char name: " << charName << endl;

    cout << "Required config: \n";
    for_each(requiredValues.begin(), requiredValues.end(), [](const auto &req) {
        cout << req;
        cout << "-------------------------------------------------------------------\n";
    });

    auto windowTitle = L"Lineage2M l " + charName;
    HWND hwnd = FindWindowW(NULL, windowTitle.c_str());
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
    ocr_eng.Init(TESSARACT_LOCATION.c_str(), "eng", tesseract::OEM_LSTM_ONLY);

    const string dateString = getStringTime();
    ofstream logFile("log_" + dateString + ".csv");

    logFile << "ParamName1;ParamVal1;ParamName2;ParamVal2;ParamName3;ParamVal3\n";

    atomic<bool> stop_flag{true};
    thread exit_thread([&stop_flag]() {
        while (stop_flag.load(memory_order_relaxed))
        {
            if (_kbhit())
            {
                _getch();
                stop_flag.store(false);
                break;
            }
            this_thread::sleep_for(10ms);
        }
    });

    while (stop_flag.load(memory_order_relaxed))
    {
        Mat screen = imageGetter.captureImage();
        if (screen.empty() or screen.cols != REQUIRED_RESOLUTION.first or screen.rows != REQUIRED_RESOLUTION.second)
        {
            std::cerr << "Failed to capture image or image size is not " << REQUIRED_RESOLUTION.first << "x"
                      << REQUIRED_RESOLUTION.second << "!\n";
            break;
        }

        cv::cvtColor(screen, screen, COLOR_BGR2GRAY);
        // cv::threshold(screen, screen, 40, 255, THRESH_BINARY_INV);
        // cv::blur(screen, screen, Size(2, 2));

        cv::GaussianBlur(screen, screen, cv::Size(1, 1), 0);
        cv::threshold(screen, screen, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
        // cv::adaptiveThreshold(screen, screen, 255,
        //                     ADAPTIVE_THRESH_GAUSSIAN_C,
        //                     THRESH_BINARY, 11, 2);

        auto recognizeImage = [&](const Rect &propNameRect, const Rect &propValueRect, const char *winName,
                                  pair<string, double> &result) -> bool {
            Mat propNameImage = screen(propNameRect);
            Mat propValueImage = screen(propValueRect);

            resize(propNameImage, propNameImage, Size(), 1.5, 1.5, INTER_CUBIC);
            resize(propValueImage, propValueImage, Size(), 2, 2, INTER_CUBIC);

            cv::morphologyEx(propNameImage, propNameImage, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size(2, 2)));

            const auto decoded_props_name = blockToString(ocr_eng, propNameImage);
            ocr_eng.SetVariable("tessedit_char_whitelist", "+%0123456789");
            const auto decoded_props_val = blockToString(ocr_eng, propValueImage);
            ocr_eng.SetVariable("tessedit_char_whitelist", "");

            result.first = decoded_props_name.first;
            string propValStr = decoded_props_val.first;
            int confidence = std::min(decoded_props_name.second, decoded_props_val.second);

            string propNameWinName = string("propName") + winName;
            string propValueWinName = string("propValue") + winName;

            if constexpr (SHOW_WINDOWS)
            {
                imshow(propNameWinName, propNameImage);
                imshow(propValueWinName, propValueImage);
            }
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

            // cout << result.first << " " << propValStr << "\n";
            // cout << result.first << " val: " << result.second << "\n";
            cout << "Confidence: " << decoded_props_name.second << " " << decoded_props_val.second
                 << " MinConf: " << confidence << "\n";

            return ret;
        };

        cout << "New attempt\n \n";
        ExpertiseConfigLoader::PropertySet props;

        bool result = true;

        result &= recognizeImage(propName1Rct, propValue1Rct, "1", props[0]);
        result &= recognizeImage(propName2Rct, propValue2Rct, "2", props[1]);
        result &= recognizeImage(propName3Rct, propValue3Rct, "3", props[2]);

        cout << props;
        logFile << props;

        if (result)
        {
            if (CheckProperties(requiredValues, props))
            {
                Beep(1000, 1000);
                break;
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

        if constexpr (SHOW_WINDOWS)
        {
            if (cv::waitKey(PAUSE_MS) >= 0)
                break;
        }
        else
        {
            Sleep(PAUSE_MS);
        }
    }

    logFile.close();
    ocr_eng.End();
    stop_flag.store(false, std::memory_order::relaxed);
    exit_thread.join();

    return 0;
}
