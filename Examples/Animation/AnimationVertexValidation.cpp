// BUG (2026-09-19): Bird_impact VERTEX files displayed individually, but multi-file
// animation/PVD containers kept IG_SURFACE. Playback and movie export propagated
// that parent style to every new frame, hiding the points again.
// 修复提交：与本测试首次加入的提交相同，主题为：
// fix(animation): inherit initial view style for vertex sequences
// 查询提交号：git log --diff-filter=A --format="%h %s" -- Examples/Animation/AnimationVertexValidation.cpp
// Exercise actual OpenFiles/PVD readers and Qt playback slots, including cache
// hits, disk reloads, interpolation and the frame capture used by export. Surface
// and mixed-block cases guard against fixing points by forcing all animations to
// point mode; manual overrides must survive subsequent frames.
#include <IQCore/igQtFileLoader.h>
#include <IQWidgets/igQtAnimationWidget.h>
#include <Log/iGameLogger.h>
#include <iGameFileIO.h>
#include <iGameSceneManager.h>
#include <QApplication>
#include <QImage>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace iGame;
namespace fs = std::filesystem;
namespace {
constexpr int Width = 320, Height = 240, Frames = 3, PointsPerFrame = 16;
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void WriteVtu(const fs::path& path, int frame, bool vertices) {
    std::ofstream out(path);
    out << "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid>"
           "<Piece NumberOfPoints=\"16\" NumberOfCells=\"" << (vertices ? 16 : 1) << "\">"
           "<PointData><DataArray Name=\"temperature\" type=\"Float32\" format=\"ascii\">";
    for (int i = 0; i < PointsPerFrame; ++i) out << i + frame << ' ';
    out << "</DataArray></PointData><Points><DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">";
    for (int i = 0; i < PointsPerFrame; ++i) out << (i % 4) + frame * .2 << ' ' << i / 4 << " 0 ";
    out << "</DataArray></Points><Cells><DataArray Name=\"connectivity\" type=\"Int32\" format=\"ascii\">";
    if (vertices) { for (int i = 0; i < PointsPerFrame; ++i) out << i << ' '; }
    else out << "0 3 12";
    out << "</DataArray><DataArray Name=\"offsets\" type=\"Int32\" format=\"ascii\">";
    if (vertices) { for (int i = 1; i <= PointsPerFrame; ++i) out << i << ' '; }
    else out << '3';
    out << "</DataArray><DataArray Name=\"types\" type=\"UInt8\" format=\"ascii\">";
    if (vertices) { for (int i = 0; i < PointsPerFrame; ++i) out << "1 "; }
    else out << '5';
    out << "</DataArray></Cells></Piece></UnstructuredGrid></VTKFile>";
    Require(bool(out), "could not write VTU fixture");
}

void WriteFixtures(const fs::path& dir) {
    fs::create_directories(dir);
    for (int f = 0; f < Frames; ++f) {
        WriteVtu(dir / ("points" + std::to_string(f) + ".vtu"), f, true);
        WriteVtu(dir / ("surface" + std::to_string(f) + ".vtu"), f, false);
    }
    for (const std::string kind : {"points", "surface", "mixed"}) {
        std::ofstream out(dir / (kind + ".pvd"));
        out << "<VTKFile type=\"Collection\"><Collection>";
        for (int f = 0; f < Frames; ++f) {
            const std::string prefix = kind == "mixed" ? "points" : kind;
            out << "<DataSet timestep=\"" << f << "\" file=\"" << prefix << f << ".vtu\"/>";
            if (kind == "mixed") out << "<DataSet timestep=\"" << f << "\" file=\"surface" << f << ".vtu\"/>";
        }
        out << "</Collection></VTKFile>";
        Require(bool(out), "could not write PVD fixture");
    }
}

DrawObject::Pointer Load(const fs::path& dir, const std::string& kind, bool multiSelect) {
    if (!multiSelect) return DynamicCast<DrawObject>(FileIO::ReadFile((dir / (kind + ".pvd")).generic_string()));
    igQtFileLoader loader;
    DrawObject::Pointer result;
    QObject::connect(&loader, &igQtFileLoader::NewModel,
                     [&](DataObject::Pointer object, ItemSource) { result = DynamicCast<DrawObject>(object); });
    QStringList paths;
    for (int f = 0; f < Frames; ++f) paths << QString::fromStdString((dir / (kind + std::to_string(f) + ".vtu")).generic_string());
    loader.OpenFiles(paths);
    return result;
}

void CheckFrame(DrawObject* root, unsigned int expected, bool pointOnly) {
    Require(root->GetViewStyle() == expected, "animation container lost the intended display style");
    Require(root->HasSubDataObject(), "animation frame has no data");
    for (auto it = root->SubDataObjectIteratorBegin(); it != root->SubDataObjectIteratorEnd(); ++it) {
        auto draw = DynamicCast<DrawObject>(it->second);
        Require(draw && draw->GetViewStyle() == expected, "frame style differs from animation container");
        if (pointOnly) {
            Require(!draw->GetShellRenderingOption(), "VERTEX frame uses surface extraction");
            Require(draw->GetRenderPoints()->GetNumberOfElements() == PointsPerFrame, "VERTEX render points are missing");
        }
    }
}

void Snap(igQtAnimationWidget& animation, int frame) {
    Require(QMetaObject::invokeMethod(&animation, "playAnimation_snap", Qt::DirectConnection,
                                     Q_ARG(unsigned int, static_cast<unsigned int>(frame))), "snap slot failed");
}

void Run(Scene* scene, const fs::path& dir, const std::string& kind, bool multiSelect, bool cache) {
    std::cout << "[CASE] " << kind << (multiSelect ? " multi-select" : " PVD") << " cache=" << cache << std::endl;
    auto root = Load(dir, kind, multiSelect);
    Require(root != nullptr, "animation load failed");
    const unsigned int expected = kind == "points" ? IG_POINTS : kind == "surface" ? IG_SURFACE : IG_POINTS | IG_SURFACE;
    Require(root->GetViewStyle() == expected, "first frame style was not inherited by animation container");
    const auto modelId = scene->AddModel(root);
    struct ModelScope {
        Scene* scene;
        unsigned int id;
        ~ModelScope() { scene->RemoveModel(id); }
    } modelScope{scene, modelId};
    scene->ResetCameraView(root->GetBoundingBox());
    {
        igQtAnimationWidget animation;
        auto frames = root->GetTimeFrames();
        if (cache) frames->EnableCache(Frames);
        else frames->DisableCache();
        root->ViewCloudPicture(scene, 0, 0);
        for (int f : {0, 1, 2, 0, 2}) {
            Snap(animation, f); // saveAnimation() calls this same slot before capturing each output frame.
            CheckFrame(root, expected, kind == "points");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            scene->Draw();
            glFinish();
            Require(glGetError() == GL_NO_ERROR, "OpenGL frame failed");
            auto pixels = scene->CaptureScreen(0, 0, Width, Height, GLFramebuffer::Type::RGBA, true);
            Require(pixels.size() == Width * Height * 4, "export frame capture failed");
            if (kind == "points") {
                // Compare with a hidden-model frame so axes/background cannot pass this check.
                root->SetVisibility(false);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                scene->Draw();
                auto blank = scene->CaptureScreen(0, 0, Width, Height, GLFramebuffer::Type::RGBA, true);
                root->SetVisibility(true);
                int changed = 0;
                for (size_t p = 0; p < pixels.size(); p += 4)
                    if (pixels[p] != blank[p] || pixels[p + 1] != blank[p + 1] || pixels[p + 2] != blank[p + 2]) ++changed;
                Require(changed > 20, "captured animation frame contains no visible points");
                const auto path = dir / (std::string(multiSelect ? "multi" : "pvd") + "-" + std::to_string(f) + ".png");
                QImage image(pixels.data(), Width, Height, QImage::Format_RGBA8888);
                Require(image.save(QString::fromStdString(path.string())), "could not save captured point frame");
            }
        }
        Require(QMetaObject::invokeMethod(&animation, "playAnimation_interpolate", Qt::DirectConnection,
                                         Q_ARG(int, 0), Q_ARG(float, .5f)), "interpolation slot failed");
        CheckFrame(root, expected, kind == "points");
        // A one-time default must not force points back on after the user disables them.
        for (unsigned int manual : {0u, static_cast<unsigned int>(IG_WIREFRAME | IG_SURFACE)}) {
            root->SetViewStyle(manual);
            Snap(animation, 1);
            CheckFrame(root, manual, kind == "points");
        }
    }
}
} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    Log::Init();
    const fs::path dir = fs::path(QCoreApplication::applicationDirPath().toStdString()) / "fixtures";
    WriteFixtures(dir);
    Require(glfwInit() != 0, "GLFW initialization failed");
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto* window = glfwCreateWindow(Width, Height, "Vertex animation regression", nullptr, nullptr);
    Require(window != nullptr, "OpenGL context unavailable");
    glfwMakeContextCurrent(window);
    auto scene = SceneManager::Instance()->NewScene();
    scene->Initialize();
    scene->Resize(Width, Height, 1);
    scene->EnableFramePacing(false);
    scene->SetMakeCurrentFunctor([&]() { glfwMakeContextCurrent(window); });
    int failures = 0;
    for (const std::string kind : {"points", "surface", "mixed"}) for (bool multi : {false, true}) {
        if (multi && kind == "mixed") continue;
        for (bool cache : {false, true}) {
            try { Run(scene, dir, kind, multi, cache); }
            catch (const std::exception& error) { ++failures; std::cerr << "FAIL: " << error.what() << std::endl; }
        }
    }
    scene->Finalize();
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "[SUMMARY] 10 cases, failures=" << failures << std::endl;
    return failures ? 1 : 0;
}
