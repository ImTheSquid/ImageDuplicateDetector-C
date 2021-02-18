#include <iostream>
#include <fstream>
#include <optional>
#include <opencv2/opencv.hpp>
#include <thread>
#include <string>
#include <vector>
#include <set>
#include <filesystem>
#include <future>

#include "argparse.hpp"
#include "indicators.hpp"

#if defined(WINDOWS)
#define strcasecmp _strcmpi
#endif

// Returns NULL optional if image load failed otherwise percentage similarity (different sizes of image automatically return 0)
const std::optional<const double> compareImages(const std::string& imagePath1, const std::string& imagePath2) {
    cv::Mat image1Mat = cv::imread(imagePath1, cv::IMREAD_UNCHANGED);
    cv::Mat image2Mat = cv::imread(imagePath2, cv::IMREAD_UNCHANGED);
    if (image1Mat.data == nullptr || image2Mat.data == nullptr) {
        return std::nullopt;
    }

    if (image1Mat.rows != image2Mat.rows || image1Mat.cols != image2Mat.cols) {
        return 0;
    }

    cv::Mat diff;
    cv::absdiff(image1Mat, image2Mat, diff);
    if (!diff.isContinuous()) {
        return 0;
    }

    std::vector<uchar> diffVec(diff.data, diff.data + diff.total() * diff.channels());
    return (double)std::count_if(diffVec.begin(), diffVec.end(), [](uchar uc){ return uc == 0; }) / diffVec.size();
}

void addDuplicate(std::vector<std::vector<std::filesystem::path>>& vec, const std::filesystem::path& path1, const std::filesystem::path& path2) {
    for (auto& group : vec) {
        // Check if either path exists in a duplicate group already
        if (std::find(group.begin(), group.end(), path1) != group.end()) {
            // Path 1 is in a group
            group.push_back(path2);
            return;
        }
        else if (std::find(group.begin(), group.end(), path2) != group.end()) {
            // Path 2 is in a group
            group.push_back(path1);
            return;
        }
    }

    // Neither do, so create a new group
    vec.push_back(std::vector{path1, path2});
}

void compareImages(const std::vector<std::filesystem::path>& paths, const int largestDimension) {
    std::vector<cv::Mat> images;
    for (const auto& path : paths) {
        images.push_back(cv::imread(path.string(), cv::IMREAD_UNCHANGED));
    }

    int height = images[0].rows, width = images[0].cols;
    double ratio = (double)width / height;
    std::cout << "Image size: " << width << "x" << height << "\n";
    if (height > width) {
        height = largestDimension;
        width = int(largestDimension / pow(ratio, -1));
    }
    else if (width > height) {
        width = largestDimension;
        height = int(largestDimension / ratio);
    }
    else {
        width = largestDimension;
        height = largestDimension;
    }
    std::cout << "Resized size: " << width << "x" << height << "\n";

    try {
        for (int i = 0; i < images.size(); ++i) {
            const std::string name = "Compare " + std::to_string(i) + " [Press any key to close]";
            cv::namedWindow(name, cv::WINDOW_NORMAL);
            cv::resizeWindow(name, width, height);
            cv::imshow(name, images[i]);
        }

        cv::waitKey(0);
        for (int i = 0; i < images.size(); ++i) {
            const std::string name = "Compare " + std::to_string(i) + " [Press any key to close]";
            cv::destroyWindow(name);
        }
    }
    catch (const cv::Exception& e) {
        std::cout << e.what() << "\n";
        std::cout << "Press ENTER to continue";
        std::cin.get();
    }
}

void clearTerminal() {
#if defined(WINDOWS)
    system("cls");
#elif defined(UNIX)
    system("clear");
#endif
}

bool fileIsValid(const std::filesystem::path& path) {
    const std::vector<std::string> validTypes = {"bmp", "dib", "jpeg", "jpg", "jpe", "jp2", "png", "webp", "pbm", "pgm", "ppm", "pxm", "pnm", "sr", "ras", "tiff", "tif", "exr", "hdr", "pic"};
    for (const auto& type : validTypes) {
        if (strcasecmp(path.extension().string().c_str(), ("." + type).c_str()) == 0) {
            return true;
        }
    }
    return false;
};

const std::set<std::filesystem::path> countFiles(const std::string& path, const bool recurse = false) {
    std::set<std::filesystem::path> toSearch;
    if (recurse) {
        for (const auto& file : std::filesystem::recursive_directory_iterator(path)) {
            if (!fileIsValid(file)) {
                continue;
            }
            toSearch.insert(file);
        }
    }
    else {
        for (const auto& file : std::filesystem::directory_iterator(path)) {
            if (!fileIsValid(file)) {
                continue;
            }
            toSearch.insert(file);
        }
    }
    return toSearch;
}

int main(int argc, char** argv) {
    argparse::ArgumentParser program("ImageDuplicateDetector");

    program.add_argument("path")
        .help("Sets path to search (program will exit if slash is at end of path)");

    program.add_argument("-r", "--recurse")
        .help("Recurses through parent directory")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-t", "--threshold")
        .help("Value from 0.1-1.0 (default 0.9) that sets how similar an image has to be to another to be flagged as a duplicate")
        .default_value(0.9)
        .action([](const std::string& value) { return std::stod(value); });

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        const std::string errStr = err.what();
        if (errStr.find("help called") == std::string::npos) {
            std::cout << "Error" << errStr << std::endl;
            std::cout << program;
            exit(1);
        }
        else {
            std::cout << program;
            exit(0);
        }
    }

    clearTerminal();
    std::cout << "=== Image Duplicate Detector (C++ Edition) | Jack Hogan 2021 ===\n";
    std::string path = program.get("path");
    if (!std::filesystem::exists(path)) {
        std::cout << "Directory \"" << path << "\" does not exist\n";
        exit(2);
    }
    if (program.get<bool>("-r")) {
        std::cout << "Recursion enabled\n";
    }
    double threshold = std::clamp(program.get<double>("-t"), 0.1, 1.0);
    if (threshold != 0.9) {
        std::cout << "Threshold set to " << threshold << "\n";
    }
    std::cout << "Counting files... this might take a while!\n";
    auto paths = countFiles(path, program.get<bool>("-r"));
    std::cout << "Found " << paths.size() << " file" << (paths.size() == 1 ? "" : "s") << "\n";
    if (paths.size() <= 1) {
        std::cout << "Didn't find enough files to compare\nExiting...\n";
        exit(0);
    }
    std::cout << "Starting file comparison\n";
    using namespace indicators;
    show_console_cursor(false);
    ProgressBar currentFile{
        option::BarWidth{50},
        option::Start{"["},
        option::Fill{"="},
        option::Lead{">"},
        option::Remainder{" "},
        option::End{"]"},
        option::ShowPercentage{true},
        option::PrefixText{"Parent Progress "},
        option::ShowElapsedTime{true}
    };
    ProgressBar compareFile{
        option::BarWidth{50},
        option::Start{"["},
        option::Fill{"="},
        option::Lead{">"},
        option::Remainder{" "},
        option::End{"]"},
        option::ShowPercentage{true},
        option::PrefixText{"Child Progress  "}
    };
    compareFile.set_progress(50);
    compareFile.set_progress(0);

    MultiProgress<ProgressBar, 2> bars(currentFile, compareFile);

    auto findDuplicates = [&bars, &compareFile, path, &paths](const bool recurse, const double threshold) -> std::vector<std::vector<std::filesystem::path>> {
        std::vector<std::vector<std::filesystem::path>> duplicates;
        std::set<std::filesystem::path> toCompare(paths);
        size_t outerCount = 0;
        for (const auto& path : paths) {
            bars.set_progress<0>(100 * outerCount / paths.size());
            toCompare.erase(toCompare.begin());
            size_t innerCount = 0;
            for (const auto& compare : toCompare) {
                bars.set_progress<1>(100 * innerCount / toCompare.size());
                if (compareImages(path.string(), compare.string()) >= threshold) {
                    addDuplicate(duplicates, path, compare);
                }
                innerCount++;
            }
            outerCount++;
        }
        bars.set_progress<0>(size_t(100));
        bars.set_progress<1>(size_t(100));
        return duplicates;
    };
    
    std::future<std::vector<std::vector<std::filesystem::path>>> ret = std::async(findDuplicates, program.get<bool>("-r"), threshold);
    auto duplicates = ret.get();

    show_console_cursor(true);
    if (duplicates.size() == 0) {
        std::cout << "No duplicates found\n";
        exit(0);
    }

    int selectedGroup = -1, largestDimension = 1000;
    std::string stringFlag = "";
    while (true) {
        clearTerminal();
        std::cout << "=== Image Duplicate Detector (C++ Edition) | Jack Hogan 2021 ===\n";
        if (stringFlag.size() > 0) {
            std::cout << stringFlag << "\n\n";
        }
        if (selectedGroup == -1) {
            if (duplicates.size() == 0) {
                std::cout << "No duplicates found\n";
                exit(0);
            }
            std::cout << "Found " << duplicates.size() << " group" << (duplicates.size() == 1 ? "" : "s") << " of duplicates\n";
            std::cout << "[Group Number] Group Info\n";
            for (int i = 0; i < duplicates.size(); ++i) {
                std::cout << "[" << std::to_string(i) << "] " << std::to_string(duplicates[i].size()) << " item" << (duplicates[i].size() == 1 ? "" : "s") << "\n";
            }

            std::cout << "\nView group: [Group Number], Export to file: e [Path], Set compare window's largest dimension (min 200, default 1000): s [Largest Dimension] Quit: q\n";
            std::cout << "Enter command:";
        }
        else {
            auto targetGroup = duplicates[selectedGroup];
            std::cout << "Group " << std::to_string(selectedGroup) << " (" << std::to_string(targetGroup.size()) << " member" << (targetGroup.size() == 1 ? "" : "s") << ")\n";
            for (int i = 0 ; i < targetGroup.size(); ++i) {
                std::cout << "[" << std::to_string(i) << "] " << std::next(targetGroup.begin(), i)->string() << "\n";
            }

            std::cout << "\nDelete item: d [Item Number], Delete all duplicates (leaves first item in group): d a, Mark as non-duplicate: n [Item Number], Compare items: c [Item Numbers (space delimited)], Compare all items: c a, Go back: q\n";
            std::cout << "Enter command:";
        }

        stringFlag = "";
        std::string command;
        std::getline(std::cin, command);
        if (command == "q") {
            if (selectedGroup == -1) {
                exit(0);
            }
            else {
                selectedGroup = -1;
                continue;
            }
        }
        if (selectedGroup == -1) {
            if (command[0] == 'e') {
                if (command.size() < 3) {
                    stringFlag = "Invalid command";
                }
                else {
                    const std::string logPath = command.substr(2, command.size());
                    if (std::filesystem::exists(logPath)) {
                        stringFlag = "File already exists";
                    }
                    else {
                        std::cout << "Writing file...\n";
                        std::ofstream logFile(logPath);
                        logFile << "=== Image Duplicate Detector (C++ Edition) | Jack Hogan 2021 ===\n";
                        for (int i = 0; i < duplicates.size(); ++i) {
                            logFile << "=== GROUP " << i << " ===\n";
                            for (int j = 0; j < duplicates[i].size(); ++j) {
                                logFile << duplicates[i][j].string() << "\n";
                            }
                        }
                        logFile.close();
                        stringFlag = "File written";
                    }
                }
            }
            else if (command[0] == 's') {
                std::istringstream iss(command);
                std::vector<std::string> commandParts(std::istream_iterator<std::string>{iss},
                                                std::istream_iterator<std::string>());
                if (commandParts.size() != 2) {
                    stringFlag = "Invalid arguments";
                }
                else {
                    largestDimension = std::max(std::stoi(commandParts[1]), 250);
                    stringFlag = "Largest dimension set to " + std::to_string(largestDimension);
                }
            }
            else {
                int choice = std::atoi(command.c_str());
                if (choice < 0 || choice > duplicates.size() - 1) {
                    stringFlag = "Invalid selection";
                } else {
                    selectedGroup = choice;
                }
            }
        }
        else {
            const std::string validCommands = "dcn";
            std::istringstream iss(command);
            std::vector<std::string> commandParts(std::istream_iterator<std::string>{iss},
                                            std::istream_iterator<std::string>());
            if (commandParts.size() == 0 || validCommands.find(commandParts[0]) == std::string::npos || commandParts[0].size() > 1) {
                stringFlag = "Invalid command or selection";
            }
            else {
                switch (commandParts[0].c_str()[0]) {
                    case 'd':
                        if (commandParts[1] == "a") {
                            for (int i = 1; i < duplicates[selectedGroup].size(); ++i) {
                                std::filesystem::remove(duplicates[selectedGroup][i]);
                            }
                            duplicates.erase(duplicates.begin() + selectedGroup);
                            selectedGroup = -1;
                        }
                        else {
                            int targetIndex = std::stoi(commandParts[1]);
                            if (targetIndex < 0 || targetIndex >= duplicates[selectedGroup].size()) {
                                stringFlag = "Invalid selection";
                            }
                            else {
                                std::filesystem::remove(duplicates[selectedGroup][targetIndex]);
                                duplicates[selectedGroup].erase(duplicates[selectedGroup].begin() + targetIndex);
                            }

                            // Remove group if no more duplicates
                            if (duplicates[selectedGroup].size() == 1) {
                                duplicates.erase(duplicates.begin() + selectedGroup);
                                selectedGroup = -1;
                            }
                        }
                    break;
                    case 'c':
                    if (commandParts[1] == "a") {
                        compareImages(duplicates[selectedGroup], largestDimension);
                    }
                    else {
                        std::set<std::filesystem::path> paths;
                        for (int i = 1; i < commandParts.size(); ++i) {
                            int targetIndex = std::stoi(commandParts[i]);
                            if (targetIndex < 0 || targetIndex >= duplicates[selectedGroup].size()) {
                                stringFlag = "Invalid selection";
                                break;
                            }
                            paths.insert(duplicates[selectedGroup][targetIndex]);
                        }
                        if (stringFlag.size() == 0) {
                            compareImages(std::vector(paths.begin(), paths.end()), largestDimension);
                        }
                    }
                    break;
                    case 'n':
                        int targetIndex = std::stoi(commandParts[1]);
                        if (targetIndex < 0 || targetIndex >= duplicates[selectedGroup].size()) {
                            stringFlag = "Invalid selection";
                        }
                        else {
                            duplicates[selectedGroup].erase(duplicates[selectedGroup].begin() + targetIndex);
                        }

                        // Remove group if no more duplicates
                        if (duplicates[selectedGroup].size() == 1) {
                            duplicates.erase(duplicates.begin() + selectedGroup);
                            selectedGroup = -1;
                        }
                    break;
                }
            }
        }
    }
}
