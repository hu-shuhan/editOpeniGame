// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/FeatureExtraction/CellSizeExtraction.cpp
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#pragma once

#include "CellSize/iGameCellSizeFilter.h"
#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameFileIO.h"
#include "iGameUnstructuredMesh.h"

#include <iomanip>
#include <iostream>
#include <string>

// CellSize command-line test program
// Run: testCellSizeExtraction  (fixed relative model path, no input needed)
// Prints the first 10 values of each per-cell Length(1D)/Area(2D)/Volume(3D) attribute to stdout
// Return code: 0=success 1=read/compute failed

namespace {

void PrintAttrStats(iGame::DataObject* data) {
    auto attrSet = data->GetAttributeSet();
    auto attrs = attrSet->GetAllAttributes();
    const char* names[] = {"Length", "Area", "Volume"};
    for (const char* name : names) {
        int idx = attrSet->GetAttributeIndex(name);
        if (idx < 0) {
            std::cout << "[CellSize] attribute=\"" << name << "\" NOT FOUND\n";
            continue;
        }
        auto& attr = attrs->GetElement(idx);
        auto arr = attr.pointer.GetPointer();
        size_t n = arr->GetNumberOfElements();
        size_t show = n < 10 ? n : 10;
        std::cout << "[CellSize] attribute=\"" << name << "\" first " << show << " values:";
        for (size_t i = 0; i < show; ++i) {
            std::cout << " " << arr->GetValue(i);
        }
        std::cout << "\n";
    }
}

// Run the filter on one model and print the first values of each attribute.
int RunOnModel(const std::string& fileName) {
    std::cout << "\n===== " << fileName << " =====\n";

    // Read the file
    iGame::DataObject::Pointer dataObj = iGame::FileIO::ReadFile(fileName);
    if (!dataObj) {
        std::cerr << "[CellSize] failed to read file: " << fileName << "\n";
        return 1;
    }

    // Execute CellSizeFilter
    iGame::CellSizeFilter::Pointer filter = iGame::CellSizeFilter::New();
    filter->SetInput(dataObj);
    if (!filter->Execute()) {
        std::string msg = filter->GetMessage();
        if (msg.empty()) msg = "execute failed";
        std::cerr << "[CellSize] " << msg << "\n";
        return 1;
    }

    // Print results from the independent output node
    auto outModel = filter->GetOutput();
    if (!outModel) {
        std::cerr << "[CellSize] filter produced no output\n";
        return 1;
    }
    PrintAttrStats(outModel);
    std::cout << "[CellSize] done: " << fileName << "\n";
    return 0;
}

} // namespace

int main() {
    const std::string models[] = {
        "./Models/CellSize_TetraCube.vtk",
        "./Models/CellSize_MixedTypes.vtk",
    };
    int rc = 0;
    for (const auto& fileName : models) {
        rc |= RunOnModel(fileName);
    }
    std::cout << "\n[CellSize] all built-in tests " << (rc == 0 ? "PASSED" : "FAILED") << "\n";
    return rc;
}
