// BUG (2026-09-19): the standalone exporter omitted PR #4's fixed color range.
// With extrema changing from 10 to 100, an unchanged value 5 changed color.
// Exercise the actual shared MP4/GIF capture path with cached animation frames,
// verify pixels and scalar buffers, preserve vector components and caller locks,
// and restore an unlocked mapper even when capture throws.
// 修复提交：与本测试首次加入的提交相同，主题为：
// fix: port remaining stable-sdk fixes before closing PR #4
// 查询提交号：git log --diff-filter=A --format="%h %s" -- Examples/Animation/AnimationExportRangeValidation.cpp
#include "SaveAnimation.h"
#include <iGameUnstructuredMesh.h>
#include <Log/iGameLogger.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace iGame;
constexpr int Width = 192, Height = 128;
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Frame : public UnstructuredMesh {
public:
    I_OBJECT(Frame);
    static Pointer New() { return new Frame; }
    FloatArray::Pointer Colors() { return m_Colors; }

protected:
    Frame() = default;
};

Frame::Pointer MakeFrame(float maximum) {
    auto frame = Frame::New();
    auto points = Points::New();
    points->AddPoint(-1, -1, 0);
    points->AddPoint(0, 0, 0);
    points->AddPoint(1, 1, 0);
    frame->SetPoints(points);
    for (igIndex id = 0; id < 3; ++id) frame->AddCell(&id, 1, IG_VERTEX);
    frame->SetShellRenderingOption(false);
    frame->SetViewStyle(IG_POINTS);
    frame->SetPointSize(13);
    auto values = FloatArray::New();
    values->SetName("velocity");
    values->SetDimension(2);
    values->AddElement2(1000, 0);
    values->AddElement2(2000, 5);
    values->AddElement2(3000, maximum);
    frame->GetAttributeSet()->AddScalar(IG_POINT, values);
    return frame;
}

void Run(Scene::Pointer scene, GLFWwindow* window) {
    scene->SetBackGround(0, 0, 0);
    auto root = DrawObject::New();
    root->SetAttributeSet(MakeFrame(10)->GetAttributeSet());
    root->SetViewStyle(IG_POINTS);
    auto first = MakeFrame(10);
    auto second = MakeFrame(100);
    auto frames = root->GetTimeFrames();
    frames->AddTimeStep(0, StringArray::New(), StreamingType::MultiSubFiles);
    frames->AddTimeStep(1, StringArray::New(), StreamingType::MultiSubFiles);
    frames->EnableCache(2);
    frames->GetTargetTimeFrame(0).SetCache({first});
    frames->GetTargetTimeFrame(1).SetCache({second});
    root->UpdateAnimation(0);
    scene->AddModel(root);
    root->ViewCloudPicture(scene, 0, 1);
    auto mapper = root->GetColorMapper();
    auto captured = CaptureAnimationFrames(scene, root, Width, Height);
    Require(captured.raw_image_data.size() == 2, "capture lost frames");
    Require(captured.bytes_per_line == Width * 4, "incorrect capture stride");
    for (const auto& pixels : captured.raw_image_data)
        Require(pixels.size() == Width * Height * 4, "incorrect frame size");
    Require(!mapper->GetStable(), "export left a previously automatic mapper locked");
    Require(std::abs(mapper->GetRange()[0]) < 1e-6 && std::abs(mapper->GetRange()[1] - 10) < 1e-6,
            "range changed to the second frame's extrema");
    Require(root->GetAttributeDimension() == 1 && second->GetAttributeDimension() == 1,
            "export changed selected component or failed to color new children");
    for (int c = 0; c < 4; ++c)
        Require(std::abs(first->Colors()->GetValue(4 + c) - second->Colors()->GetValue(4 + c)) < 1e-6,
                "unchanged scalar value changed color between frames");
    // The stationary central point must remain colored identically in actual captures.
    for (int y = Height / 2 - 2; y <= Height / 2 + 2; ++y)
        for (int x = Width / 2 - 2; x <= Width / 2 + 2; ++x)
            for (int c = 0; c < 3; ++c) {
                const int offset = (y * Width + x) * 4 + c;
                Require(captured.raw_image_data[0][offset] == captured.raw_image_data[1][offset],
                        "stationary point changed color in captured pixels");
            }
    const int center = ((Height / 2) * Width + Width / 2) * 4;
    const auto& pixels = captured.raw_image_data[0];
    // The default diverging palette has a gray midpoint, so compare against the
    // mapped color, not an assumption that valid scalar colors cannot be gray.
    Require(pixels[center] + pixels[center + 1] + pixels[center + 2] > 0, "central point is blank");
    for (int c = 0; c < 3; ++c)
        Require(std::abs(int(pixels[center + c]) - std::lround(first->Colors()->GetValue(4 + c) * 255)) <= 1,
                "captured pixel does not match the scalar color");

    mapper->SetRange(-10, 200);
    mapper->SetRangeStable(true);
    CaptureAnimationFrames(scene, root, Width, Height);
    Require(mapper->GetStable() && mapper->GetRange()[0] == -10 && mapper->GetRange()[1] == 200,
            "export changed an existing user range lock");
    mapper->SetRangeStable(false);
    scene->SetMakeCurrentFunctor([mapper, window]() {
        if (mapper->GetStable()) throw std::runtime_error("injected capture failure");
        glfwMakeContextCurrent(window);
    });
    bool threw = false;
    try { CaptureAnimationFrames(scene, root, Width, Height); }
    catch (const std::runtime_error&) { threw = true; }
    scene->SetMakeCurrentFunctor([window]() { glfwMakeContextCurrent(window); });
    Require(threw && !mapper->GetStable(), "exception did not restore the previous lock state");
    Require(CaptureAnimationFrames(nullptr, root).raw_image_data.empty(), "null scene accepted");
    Require(CaptureAnimationFrames(scene, nullptr).raw_image_data.empty(), "null object accepted");
    Require(CaptureAnimationFrames(scene, DrawObject::New()).raw_image_data.empty(), "empty sequence accepted");

    // Both encoders consume the same verified pixel sequence.
    auto writer = FFMPEGVideoWriter::New();
    auto encoding = captured;
    encoding.output_path = "export-range-validation.mp4";
    writer->SetVideoInputInfo(encoding); // transfers ownership of the input
    Require(writer->SaveMP4(), "MP4 encoder failed");
    Require(std::filesystem::file_size("export-range-validation.mp4") > 0, "MP4 output is empty");
    encoding = captured;
    encoding.output_path = "export-range-validation.gif";
    writer->SetVideoInputInfo(encoding);
    Require(writer->SaveGIF(), "GIF encoder failed");
    Require(std::filesystem::file_size("export-range-validation.gif") > 0, "GIF output is empty");
    scene->RemoveModel(scene->GetCurrentModel());
}
} // namespace

int main() {
    Log::Init();
    if (!glfwInit()) {
        std::cout << "SKIP: no GLFW display is available\n";
        return 77;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto* window = glfwCreateWindow(Width, Height, "Animation export validation", nullptr, nullptr);
    if (!window) {
        std::cout << "SKIP: an OpenGL 4.6 context is unavailable\n";
        glfwTerminate();
        return 77;
    }
    glfwMakeContextCurrent(window);
    auto scene = Scene::New();
    scene->SetMakeCurrentFunctor([&]() { glfwMakeContextCurrent(window); });
    scene->Initialize();
    scene->Resize(Width, Height, 1);
    scene->EnableFramePacing(false);
    int result = 0;
    try { Run(scene, window); std::cout << "PASS fixed range, pixels, component, lock restoration, MP4/GIF\n"; }
    catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; result = 1; }
    scene->Finalize();
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
