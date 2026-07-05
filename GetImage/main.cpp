#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

#include "Tools.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: GetImage <character-name>\n";
        return 1;
    }

    const std::wstring characterName = utf8_to_wstring(argv[1]);
    if (characterName.empty())
    {
        std::cerr << "Character name must be valid UTF-8.\n";
        return 1;
    }

    const std::wstring windowTitle = L"Lineage2M l " + characterName;
    const HWND hwnd = FindWindowW(nullptr, windowTitle.c_str());
    if (!hwnd)
    {
        std::cerr << "Window not found.\n";
        return 1;
    }

    ImageGetter imageGetter;
    if (!imageGetter.initialize(hwnd)) return 1;

    const cv::Mat image = imageGetter.captureImage();
    if (image.empty())
    {
        std::cerr << "Failed to capture image.\n";
        return 1;
    }

    cv::imshow("Captured image", image);
    cv::waitKey(0);
    return 0;
}
