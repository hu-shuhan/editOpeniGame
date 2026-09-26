// Source: dayuwan77/igamevis at eccac729b57aeacbe9312d7d5189f6990bb4eebd.
// Integration gap: e7ec6571 imported the filter but omitted its example.
// Preserve the numerical/attribute checks below and reject invalid inputs;
// visual examples support --no-render so CI requires a real exit status.
// Integration commit: test: add examples for first-batch standard filters
// Find it: git log --diff-filter=A --format="%h %s" -- Examples/Filter/FeatureExtraction/OutlineCorners.cpp
#include "FeatureExtraction/iGameOutlineCornerFilter.h"
#include "iGameFileIO.h"
#include "iGamePointSet.h"

#include <exception>
#include <iostream>
#include <string>

namespace {
iGame::PointSet::Pointer CreateTestInput() {
    auto input = iGame::PointSet::New();
    input->AddPoint(iGame::Point(-1.0, -2.0, -3.0));
    input->AddPoint(iGame::Point(4.0, 5.0, 6.0));
    return input;
}
} // namespace

int main(int argc, char* argv[]) {
    if (argc > 3) {
        std::cerr << "Usage: testOutlineCorners <input-model> [corner-factor]\n";
        return 1;
    }

    const std::string inputFile = argc >= 2 ? argv[1] : "built-in bounding box";
    iGame::DataObject::Pointer input = argc >= 2
            ? iGame::FileIO::ReadFile(inputFile)
            : CreateTestInput();
    if (input.IsNull()) {
        std::cerr << "Failed to read input model: " << inputFile << '\n';
        return 2;
    }

    auto filter = iGame::OutlineCornerFilter::New();
    filter->SetInput(input);
    if (argc >= 3) {
        try {
            filter->SetCornerFactor(std::stof(argv[2]));
        } catch (const std::exception&) {
            std::cerr << "Invalid corner factor: " << argv[2] << '\n';
            return 3;
        }
    }

    if (!filter->Execute()) {
        std::cerr << "OutlineCornerFilter failed: " << filter->GetMessage() << '\n';
        return 4;
    }

    const auto result = filter->GetResult();
    if (result.IsNull()) {
        std::cerr << "OutlineCornerFilter returned no result.\n";
        return 5;
    }
    // Eight corners, each represented by a shared origin and three segments.
    if (result->GetNumberOfPoints() != 32 || result->GetNumberOfCells() != 24) {
        std::cerr << "Incorrect outline corner topology.\n";
        return 6;
    }

    std::cout << "Input: " << inputFile << '\n';
    std::cout << "Corner factor: " << filter->GetCornerFactor() << '\n';
    std::cout << "Number of points: " << result->GetNumberOfPoints() << '\n';
    std::cout << "Number of line cells: " << result->GetNumberOfCells() << '\n';

    for (IGsize cornerId = 0; cornerId < 8; ++cornerId) {
        const auto& point = result->GetPoint(cornerId * 4);
        std::cout << "corner[" << cornerId << "] = ("
                  << point[0] << ", " << point[1] << ", " << point[2] << ")\n";
    }

    return 0;
}
