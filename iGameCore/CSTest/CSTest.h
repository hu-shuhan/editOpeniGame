#pragma once
#include <string>

class OpenCmd {
public:
    int selected_idx;
    std::string filePath;

    std::string serialize() const { return std::to_string(selected_idx) + "|" + filePath; }

    void deserialize(const std::string& data) {
        size_t pos = data.find("|");
        if (pos != std::string::npos) {
            selected_idx = std::stoi(data.substr(0, pos));
            filePath = data.substr(pos + 1);
        } else {
            selected_idx = -1;
            filePath = "";
        }
    }
};
