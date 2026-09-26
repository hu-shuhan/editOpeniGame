#include <IQCore/igQtAnimationFilterAdapters.h>
#include <IQCore/igQtAnimationFilterManager.h>
#include <IQCore/igQtAnimationPipeline.h>

#include <Convert/iGameConvertToPointDataFilter.h>
#include <iGameDataObject.h>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace iGame;

namespace {

void Require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

DataObject::Pointer ReadFrame(const fs::path& directory, int frame) {
    const auto file =
            directory / ("VTKfile-" + std::to_string(frame) + ".vtk");
    auto object = FileIO::ReadFile(file.generic_string());
    Require(object != nullptr, "failed to read " + file.string());
    return object;
}

AttributeSet::Attribute& FindAnyAttribute(DataObject::Pointer object,
                                          const std::string& name) {
    Require(object && object->GetAttributeSet(), "frame has no attributes");
    auto& attribute = object->GetAttributeSet()->GetAttribute(name);
    Require(!attribute.IsNone(), "missing attribute: " + name);
    return attribute;
}

bool HasPointAttribute(DataObject::Pointer object, const std::string& name) {
    if (!object || !object->GetAttributeSet()) return false;
    auto attributes = object->GetAttributeSet()->GetAllPointAttributes();
    if (!attributes) return false;
    for (int i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto array = attributes->GetElement(i).pointer;
        if (array && array->GetName() == name) return true;
    }
    return false;
}

int CountOutputCells(DataObject::Pointer object) {
    if (auto mesh = DynamicCast<UnstructuredMesh>(object)) {
        return static_cast<int>(mesh->GetNumberOfCells());
    }
    int total = 0;
    for (auto it = object->SubDataObjectIteratorBegin();
         it != object->SubDataObjectIteratorEnd(); ++it) {
        total += CountOutputCells(DynamicCast<DataObject>(it->second));
    }
    return total;
}

igQtAnimationFilterManager CreateManager() {
    igQtAnimationFilterManager manager;
    QString error;
    Require(igQtRegisterBuiltinAnimationFilters(manager, &error),
            "register builtin filters: " + error.toStdString());
    return manager;
}

void ValidateModifyOnly(const fs::path& directory, int frame) {
    std::cout << "[frame " << frame << "] modify-only: reading frame\n";
    auto object = ReadFrame(directory, frame);
    std::cout << "[frame " << frame << "] modify-only: registering filters\n";
    auto manager = CreateManager();

    igQtAnimationPipelineSteps steps;
    igQtAnimationPipelineStep convert;
    convert.filterId = QStringLiteral("convertToPointData");
    steps.push_back(convert);

    igQtAnimationFrameContext context;
    context.input = object;
    context.sourceFrameIndex = frame;
    context.outputFrameIndex = frame;

    igQtAnimationFilterResult result;
    QString error;
    std::cout << "[frame " << frame << "] modify-only: executing pipeline\n";
    Require(igQtExecuteAnimationPipeline(manager, steps, context, result,
                                         &error),
            "modify-only pipeline failed: " + error.toStdString());
    std::cout << "[frame " << frame << "] modify-only: checking result\n";
    Require(result.output != nullptr, "modify-only pipeline returned null");
    Require(result.output.GetPointer() == object.GetPointer(),
            "modify-only filter must keep the same model");
    Require(HasPointAttribute(result.output, "Pressure"),
            "Pressure was not converted to point data");

    std::cout << "[frame " << frame
              << "] modify-only kept model and converted Pressure\n";
}

void ValidateDirectConvert(const fs::path& directory, int frame) {
    std::cout << "[frame " << frame << "] direct convert: reading frame\n";
    auto object = ReadFrame(directory, frame);
    std::cout << "[frame " << frame << "] direct convert: executing filter\n";
    auto filter = iGame::ConvertToPointDataFilter::New();
    filter->SetInput(object);
    Require(filter->Execute(), "direct ConvertToPointDataFilter failed");
    std::cout << "[frame " << frame << "] direct convert: checking result\n";
    Require(HasPointAttribute(object, "Pressure"),
            "direct convert did not produce point Pressure");
    std::cout << "[frame " << frame << "] direct convert: ok\n";
}

bool ContainsName(const std::vector<std::string>& names,
                  const std::string& name) {
    return std::find(names.begin(), names.end(), name) != names.end();
}

void CollectAttributeNames(DataObject::Pointer object, bool cell,
                           std::vector<std::string>& names) {
    if (!object || !object->GetAttributeSet()) return;
    auto attributes = cell ? object->GetAttributeSet()->GetAllCellAttributes()
                           : object->GetAttributeSet()->GetAllPointAttributes();
    if (!attributes) return;
    for (int i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto array = attributes->GetElement(i).pointer;
        if (array) names.push_back(array->GetName());
    }
}

void ValidateAttributeSideEffects(const fs::path& directory, int frame) {
    std::cout << "[frame " << frame << "] side effects: reading frame\n";
    auto object = ReadFrame(directory, frame);

    std::vector<std::string> cellBefore;
    std::vector<std::string> pointBefore;
    CollectAttributeNames(object, true, cellBefore);
    CollectAttributeNames(object, false, pointBefore);
    Require(!cellBefore.empty(), "frame has no cell attributes to convert");

    auto manager = CreateManager();
    igQtAnimationPipelineSteps steps;
    igQtAnimationPipelineStep convert;
    convert.filterId = QStringLiteral("convertToPointData");
    steps.push_back(convert);

    igQtAnimationFrameContext context;
    context.input = object;
    context.sourceFrameIndex = frame;
    context.outputFrameIndex = frame;
    igQtAnimationFilterResult result;
    QString error;
    Require(igQtExecuteAnimationPipeline(manager, steps, context, result,
                                         &error),
            "convert pipeline failed: " + error.toStdString());

    std::vector<std::string> cellAfter;
    std::vector<std::string> pointAfter;
    CollectAttributeNames(result.output, true, cellAfter);
    CollectAttributeNames(result.output, false, pointAfter);
    for (const auto& name : cellBefore) {
        Require(ContainsName(pointAfter, name),
                "converted attribute disappeared from point data: " + name);
        Require(!ContainsName(cellAfter, name),
                "converted attribute still present in cell data: " + name);
    }
    std::cout << "[frame " << frame << "] side effects ok: cell="
              << cellBefore.size() << " -> point=" << pointAfter.size()
              << "\n";
}

void ValidatePipeline(const fs::path& directory, int frame) {
    std::cout << "[frame " << frame << "] pipeline: reading frame\n";
    auto object = ReadFrame(directory, frame);
    std::cout << "[frame " << frame << "] pipeline: locating Pressure\n";
    auto& pressure = FindAnyAttribute(object, "Pressure");
    Require(pressure.pointer != nullptr, "Pressure has no data array");
    std::cout << "[frame " << frame << "] pipeline: computing range\n";
    double minimum = std::numeric_limits<double>::max();
    double maximum = -std::numeric_limits<double>::max();
    for (size_t i = 0; i < pressure.pointer->GetNumberOfValues(); ++i) {
        const double value = pressure.pointer->GetValue(i);
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
    }
    Require(std::isfinite(minimum) && std::isfinite(maximum) &&
                    minimum <= maximum,
            "Pressure data range is invalid");

    auto manager = CreateManager();
    struct IsoCandidate {
        double lower;
        double upper;
    };
    std::vector<IsoCandidate> candidates;
    const double span = maximum - minimum;
    if (span > 0.0) {
        candidates.push_back({minimum + span / 3.0,
                              minimum + span * 2.0 / 3.0});
    }
    const double pad = span > 0.0 ? span * 0.5 : 1.0;
    candidates.push_back({minimum - pad, maximum + pad});

    QString lastError;
    for (size_t candidateIndex = 0; candidateIndex < candidates.size();
         ++candidateIndex) {
        const auto& candidate = candidates[candidateIndex];
        igQtAnimationPipelineSteps steps;

        igQtAnimationPipelineStep convert;
        convert.filterId = QStringLiteral("convertToPointData");
        steps.push_back(convert);

        igQtAnimationPipelineStep iso;
        iso.filterId = QStringLiteral("isoVolume");
        iso.parameters.insert(QStringLiteral("scalarName"),
                              QStringLiteral("Pressure"));
        iso.parameters.insert(QStringLiteral("scalarDimension"), 0);
        iso.parameters.insert(QStringLiteral("lowerValue"), candidate.lower);
        iso.parameters.insert(QStringLiteral("upperValue"), candidate.upper);
        steps.push_back(iso);

        igQtAnimationFrameContext context;
        context.input = object;
        context.sourceFrameIndex = frame;
        context.outputFrameIndex = frame;

        igQtAnimationFilterResult result;
        QString error;
        std::cout << "[frame " << frame << "] pipeline: executing candidate "
                  << (candidateIndex + 1) << " isoRange=["
                  << candidate.lower << ", " << candidate.upper << "]\n";
        if (!igQtExecuteAnimationPipeline(manager, steps, context, result,
                                          &error)) {
            lastError = error;
            std::cout << "[frame " << frame << "] pipeline candidate failed: "
                      << error.toStdString() << "\n";
            continue;
        }
        Require(result.output != nullptr, "pipeline returned null output");
        Require(result.output.GetPointer() != object.GetPointer(),
                "isoVolume must create a new model");
        const int cells = CountOutputCells(result.output);
        if (cells > 0) {
            std::cout << "[frame " << frame << "] pipeline cells=" << cells
                      << " pressureRange=[" << minimum << ", " << maximum << "]"
                      << " isoRange=[" << candidate.lower << ", "
                      << candidate.upper << "]\n";
            return;
        }
        std::cout << "[frame " << frame << "] pipeline candidate empty\n";
        lastError = QStringLiteral("isoVolume produced an empty output");
    }

    throw std::runtime_error(
            "no isoVolume candidate produced output: " +
            lastError.toStdString());
}

bool ScanPressureRange(DataObject::Pointer object, double& minimum,
                       double& maximum) {
    auto& pressure = FindAnyAttribute(object, "Pressure");
    if (!pressure.pointer || pressure.pointer->GetNumberOfValues() == 0) {
        return false;
    }
    minimum = std::numeric_limits<double>::max();
    maximum = -std::numeric_limits<double>::max();
    for (size_t i = 0; i < pressure.pointer->GetNumberOfValues(); ++i) {
        const double value = pressure.pointer->GetValue(i);
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
    }
    return minimum <= maximum;
}

void TestRangeAcrossFrames(const fs::path& directory, double lower,
                           double upper, const std::string& label) {
    auto manager = CreateManager();
    int total = 0;
    int ok = 0;
    int empty = 0;
    int failed = 0;
    std::vector<int> emptyFrames;
    std::vector<int> failedFrames;

    for (int frame = 0; frame <= 400; ++frame) {
        auto object = ReadFrame(directory, frame);
        ++total;

        igQtAnimationPipelineSteps steps;
        igQtAnimationPipelineStep convert;
        convert.filterId = QStringLiteral("convertToPointData");
        steps.push_back(convert);
        igQtAnimationPipelineStep iso;
        iso.filterId = QStringLiteral("isoVolume");
        iso.parameters.insert(QStringLiteral("scalarName"),
                              QStringLiteral("Pressure"));
        iso.parameters.insert(QStringLiteral("scalarDimension"), 0);
        iso.parameters.insert(QStringLiteral("lowerValue"), lower);
        iso.parameters.insert(QStringLiteral("upperValue"), upper);
        steps.push_back(iso);

        igQtAnimationFrameContext context;
        context.input = object;
        context.sourceFrameIndex = frame;
        context.outputFrameIndex = frame;
        igQtAnimationFilterResult result;
        QString error;
        if (!igQtExecuteAnimationPipeline(manager, steps, context, result,
                                          &error)) {
            ++failed;
            failedFrames.push_back(frame);
            continue;
        }
        if (CountOutputCells(result.output) == 0) {
            ++empty;
            emptyFrames.push_back(frame);
            continue;
        }
        ++ok;
    }

    std::cout << "[range " << label << "] [" << lower << ", " << upper << "]"
              << " ok=" << ok << "/" << total << " empty=" << empty
              << " failed=" << failed << "\n";
    if (!emptyFrames.empty()) {
        std::cout << "[range " << label << "] empty frames:";
        for (size_t i = 0; i < emptyFrames.size() && i < 20; ++i) {
            std::cout << " " << emptyFrames[i];
        }
        if (emptyFrames.size() > 20) {
            std::cout << " ...(" << emptyFrames.size() << ")";
        }
        std::cout << "\n";
    }
    if (!failedFrames.empty()) {
        std::cout << "[range " << label << "] failed frames:";
        for (size_t i = 0; i < failedFrames.size() && i < 20; ++i) {
            std::cout << " " << failedFrames[i];
        }
        if (failedFrames.size() > 20) {
            std::cout << " ...(" << failedFrames.size() << ")";
        }
        std::cout << "\n";
    }
}

void RunRangeScan(const fs::path& directory) {
    double globalMinimum = std::numeric_limits<double>::max();
    double globalMaximum = -std::numeric_limits<double>::max();
    int scanned = 0;
    for (int frame = 0; frame <= 400; ++frame) {
        auto object = ReadFrame(directory, frame);
        double minimum = 0.0;
        double maximum = 0.0;
        if (ScanPressureRange(object, minimum, maximum)) {
            globalMinimum = std::min(globalMinimum, minimum);
            globalMaximum = std::max(globalMaximum, maximum);
            ++scanned;
        }
    }
    std::cout << "[scan] scanned frames=" << scanned
              << " globalPressureRange=[" << globalMinimum << ", "
              << globalMaximum << "]\n";

    const double span = globalMaximum - globalMinimum;
    const double pad = std::max(1.0, std::abs(span) * 0.1);
    struct RangeCandidate {
        std::string label;
        double lower;
        double upper;
    };
    const std::vector<RangeCandidate> candidates = {
            {"full", globalMinimum, globalMaximum},
            {"thirds", globalMinimum + span / 3.0,
             globalMinimum + span * 2.0 / 3.0},
            {"padded", globalMinimum - pad, globalMaximum + pad},
    };
    for (const auto& candidate : candidates) {
        TestRangeAcrossFrames(directory, candidate.lower, candidate.upper,
                              candidate.label);
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::cout << std::unitbuf;
        std::cerr << std::unitbuf;
        const fs::path directory =
                argc > 1 ? fs::path(argv[1])
                         : fs::path("test") / "Animation Test" / "result-case1";
        Require(fs::exists(directory), "animation directory does not exist: " +
                                           directory.string());

        if (argc > 2 && std::string(argv[2]) == "scan") {
            RunRangeScan(directory);
            return 0;
        }

        const std::vector<int> frames = {0, 200, 400};
        for (const int frame : frames) {
            ValidateAttributeSideEffects(directory, frame);
            ValidateDirectConvert(directory, frame);
            ValidateModifyOnly(directory, frame);
            ValidatePipeline(directory, frame);
        }

        std::cout << "[SUMMARY] animation pipeline validation passed for "
                  << frames.size() << " frames\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "[FAIL] unknown exception\n";
        return 1;
    }
}
