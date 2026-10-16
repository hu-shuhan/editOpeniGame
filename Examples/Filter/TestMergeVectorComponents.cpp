// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestMergeVectorComponents.cpp
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
// Automatic test for MergeVectorComponentsFilter (no manual input required).
// Model: ./Models/MergeVectorComponents_Quad_Plane.vtk (hardcoded relative path; the CMake
//        assets step copies Examples/Models next to the executable)
// Checks (fully automatic):
//   1) point merge {u1, v1, w1} -> independent output node, IG_VECTOR dim=3 (point data)
//   2) cell merge {c_amp, c_amp, c_amp} -> independent output node, IG_VECTOR dim=3 (cell data)
//   3) the original model stays untouched; output node named <vector>_<model>_merged
// Return: 0 = all pass, 1 = any failure

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include <iGameFileIO.h>
#include <iGameDataObject.h>
#include <iGameAttributeSet.h>
#include <iGameFlatArray.h>
#include <MergeVectorComponents/iGameMergeVectorComponentsFilter.h>

namespace {

int g_passes = 0;
int g_failures = 0;

// 通过时静默计数, 失败时才打印, 避免输出冗余
void Check(bool cond, const std::string& what) {
    if (cond) { ++g_passes; return; }
    ++g_failures;
    std::cout << "  [FAIL] " << what << "\n" << std::flush;
}

void PrintScalarPreview(iGame::ArrayObject* arr, IGsize count) {
    if (!arr) { std::cout << "(null)\n"; return; }
    const IGsize n = arr->GetNumberOfElements();
    const IGsize m = count < n ? count : n;
    for (IGsize i = 0; i < m; ++i) {
        std::cout << arr->GetValue(i);
        if (i + 1 < m) std::cout << ", ";
    }
    std::cout << "\n" << std::flush;
}

void PrintVectorPreview(iGame::ArrayObject* arr, IGsize count) {
    if (!arr) return;
    const int dim = arr->GetDimension();
    const IGsize n = arr->GetNumberOfElements();
    const IGsize m = count < n ? count : n;
    for (IGsize i = 0; i < m; ++i) {
        std::cout << "    [" << i << "] = (";
        for (int d = 0; d < dim; ++d) {
            std::cout << arr->GetElementValue(i, d);
            if (d + 1 < dim) std::cout << ", ";
        }
        std::cout << ")\n";
    }
    std::cout << std::flush;
}

iGame::DataObject::Pointer RunMerge(iGame::DataObject::Pointer input,
                                    const std::vector<std::string>& names,
                                    IGenum attach, const std::string& outName) {
    auto filter = iGame::MergeVectorComponentsFilter::New();
    filter->SetInput(input);
    filter->SetComponentArrayNames(names);
    filter->SetAttachmentType(attach);
    filter->SetOutputVectorName(outName);
    if (!filter->Execute()) {
        std::cout << "  [FAIL] Execute: " << filter->GetMessage() << "\n" << std::flush;
        ++g_failures;
        return nullptr;
    }
    return filter->GetOutput();
}

// Verify an independent output node for a merge of `names` (same order) under `attach`
void VerifyOutput(iGame::DataObject::Pointer input, iGame::DataObject::Pointer output,
                  IGenum attach, const std::vector<std::string>& names) {
    Check(output != nullptr, "output node exists");
    if (!output) return;
    Check(output.GetPointer() != input.GetPointer(),
          "output is an independent node (not the input)");
    Check(input->GetAttributeSet()->GetAttributeIndex("vector") < 0,
          "original model untouched (no merged vector on input)");
    Check(output->GetName().find("_merged") != std::string::npos,
          "output node named <vector>_<model>_merged");

    auto inAttrs = input->GetAttributeSet();
    auto outAttrs = output->GetAttributeSet();
    if (!outAttrs) { Check(false, "output has an AttributeSet"); return; }

    auto& merged = outAttrs->GetAttribute("vector");
    Check(!merged.IsNone() && merged.type == IG_VECTOR, "vector attribute registered as IG_VECTOR");
    Check(merged.attachmentType == attach, "vector attached to the requested data type");
    Check(merged.pointer && merged.pointer->GetDimension() == 3, "vector dimension == 3");

    std::vector<iGame::ArrayObject::Pointer> src;
    for (const auto& name : names) {
        src.push_back(inAttrs->GetAttribute(name).pointer);
        Check(src.back() != nullptr, "input scalar \"" + name + "\" present");
    }
    if (!src[0] || !src[1] || !src[2]) return;
    const IGsize n = src[0]->GetNumberOfElements();
    Check(merged.pointer->GetNumberOfElements() == n, "vector element count matches scalars");

    bool valuesOk = true;
    for (IGsize i = 0; valuesOk && i < n; ++i) {
        for (int d = 0; d < 3; ++d) {
            if (merged.pointer->GetElementValue(i, d) != src[static_cast<size_t>(d)]->GetValue(i)) {
                valuesOk = false;
                break;
            }
        }
    }
    Check(valuesOk, "component values match the input scalars");

    // 打印结果: 所选标量前 10 值 + 合并向量前 10 元素
    const char* axis[3] = {"X", "Y", "Z"};
    for (int d = 0; d < 3; ++d) {
        std::cout << "  [" << axis[d] << " " << names[d] << "] first 10 values: ";
        PrintScalarPreview(src[d], 10);
    }
    std::cout << "  [vector] first 10 elements:\n";
    PrintVectorPreview(merged.pointer, 10);
}

} // namespace

int main(int argc, char** argv) {
    // Hardcoded relative model path (no manual input required)
    std::string modelPath = "./Models/MergeVectorComponents_Quad_Plane.vtk";
    if (argc >= 2) modelPath = argv[1];

    std::cout << "[testMergeVectorComponents] model: " << modelPath << "\n" << std::flush;
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(modelPath);
    Check(obj != nullptr, "model loaded");
    if (!obj) return 1;
    Check(obj->GetAttributeSet() != nullptr, "model has an AttributeSet");
    if (!obj->GetAttributeSet()) return 1;

    std::cout << "--- Point merge {u1, v1, w1} ---\n";
    const int fail0 = g_failures;
    auto outP = RunMerge(obj, {"u1", "v1", "w1"}, IG_POINT, "vector");
    VerifyOutput(obj, outP, IG_POINT, {"u1", "v1", "w1"});
    std::cout << "  --> point merge: " << (g_failures == fail0 ? "OK" : "FAILED") << "\n";

    std::cout << "--- Cell merge {c_amp, c_amp, c_amp} ---\n";
    const int fail1 = g_failures;
    auto outC = RunMerge(obj, {"c_amp", "c_amp", "c_amp"}, IG_CELL, "vector");
    VerifyOutput(obj, outC, IG_CELL, {"c_amp", "c_amp", "c_amp"});
    std::cout << "  --> cell merge: " << (g_failures == fail1 ? "OK" : "FAILED") << "\n";

    std::cout << "\n[testMergeVectorComponents] "
              << (g_failures == 0 ? std::string("ALL PASS") : "FAILED")
              << "  (" << g_passes << " passed, " << g_failures << " failed)\n" << std::flush;
    return g_failures == 0 ? 0 : 1;
}
