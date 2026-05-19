#include "iGameDrawObject.h"
#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <ctime>
#include <emscripten/emscripten.h>
#include <map>
#include <string>
#include <vector>

#ifndef __EMSCRIPTEN__
#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE
#endif
#endif

namespace
{
iGame::Scene::Pointer g_scene;
iGame::RenderWindow::Pointer g_window;
iGame::Interactor::Pointer g_interactor;
IGuint g_activeModelId = 0;

struct WebModelMeta {
    IGuint id = 0;
    std::string name;
    bool visible = true;
    std::string sourceType;
    long long loadTime = 0;
};

std::map<IGuint, WebModelMeta> g_modelRegistry;

std::string EscapeJsonString(const std::string& in) {
    std::string out;
    out.reserve(in.size() + 8);
    for (char c: in) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

void EnsureScene() {
    if (g_scene != nullptr && g_window != nullptr && g_interactor != nullptr) return;

    g_scene = iGame::Scene::New();
    g_window = iGame::RenderWindow::New();
    g_window->SetSize(1280, 720);
    g_window->SetScene(g_scene);

    g_interactor = iGame::Interactor::New();
    g_interactor->Initialize(g_scene);
    g_window->SetInteractor(g_interactor);
    g_scene->SetInteractor(g_interactor);
}

long long GetUnixTimestampSeconds() { return static_cast<long long>(std::time(nullptr)); }

void SyncActiveModelIdFromScene() {
    if (g_scene == nullptr) {
        g_activeModelId = 0;
        return;
    }
    auto current = g_scene->GetCurrentModelID();
    if (current != 0) {
        g_activeModelId = current;
        return;
    }
    g_activeModelId = 0;
}

void RemoveAllUserModels() {
    if (g_scene == nullptr) return;
    std::vector<IGuint> ids;
    ids.reserve(g_modelRegistry.size());
    for (const auto& kv: g_modelRegistry) { ids.push_back(kv.first); }
    for (IGuint id: ids) {
        g_scene->RemoveModel(id);
        g_modelRegistry.erase(id);
    }
    SyncActiveModelIdFromScene();
}

int AddModelFromDataObject(iGame::SmartPointer<iGame::DataObject> dataObj, const char* sourceName, bool replaceExisting,
                           const char* sourceType) {
    if (dataObj == nullptr || g_scene == nullptr) return 0;

    if (replaceExisting) { RemoveAllUserModels(); }

    const IGuint modelId = g_scene->AddModel(dataObj);
    if (modelId == 0) return 0;

    WebModelMeta meta;
    meta.id = modelId;
    meta.name = (sourceName != nullptr && sourceName[0] != '\0') ? sourceName
                                                                 : std::string("model_") + std::to_string(modelId);
    meta.visible = true;
    meta.sourceType = (sourceType != nullptr && sourceType[0] != '\0') ? sourceType : "memory-vtk";
    meta.loadTime = GetUnixTimestampSeconds();
    g_modelRegistry[modelId] = meta;

    g_activeModelId = modelId;
    g_scene->SetCurrentModel(static_cast<int>(modelId));
    g_scene->ResetCameraView();
    return 1;
}

iGame::Model::Pointer GetActiveModel() {
    if (g_scene == nullptr) return nullptr;
    if (g_activeModelId != 0) {
        auto model = g_scene->GetModelById(static_cast<int>(g_activeModelId));
        if (model != nullptr) { return model; }
    }
    return g_scene->GetCurrentModel();
}

} // namespace

extern "C" {

int igw_init() {
    EnsureScene();
    return (g_scene != nullptr && g_window != nullptr && g_window->GetRawWindowPtr() != nullptr) ? 1 : 0;
}

int igw_load_vtk_from_mem(const char* bytes, int size) {
    if (bytes == nullptr || size <= 0) return 0;

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadVTKFromMemory(bytes, static_cast<size_t>(size));
    if (dataObj == nullptr) return 0;

    return AddModelFromDataObject(dataObj, "Imported VTK", false, "memory-vtk");
}

int igw_load_vtu_from_mem(const char* bytes, int size) {
    if (bytes == nullptr || size <= 0) return 0;

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadVTUFromMemory(bytes, static_cast<size_t>(size));
    if (dataObj == nullptr) return 0;

    return AddModelFromDataObject(dataObj, "Imported VTU", false, "memory-vtu");
}

int igw_load_vtk_from_mem_ex(const char* bytes, int size, const char* sourceName, int replaceExisting) {
    if (bytes == nullptr || size <= 0) return 0;

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadVTKFromMemory(bytes, static_cast<size_t>(size));
    if (dataObj == nullptr) return 0;

    return AddModelFromDataObject(dataObj, sourceName, replaceExisting != 0, "memory-vtk");
}

int igw_load_vtu_from_mem_ex(const char* bytes, int size, const char* sourceName, int replaceExisting) {
    if (bytes == nullptr || size <= 0) return 0;

    EnsureScene();

    auto dataObj = iGame::FileIO::ReadVTUFromMemory(bytes, static_cast<size_t>(size));
    if (dataObj == nullptr) return 0;

    return AddModelFromDataObject(dataObj, sourceName, replaceExisting != 0, "memory-vtu");
}

const char* igw_get_model_list_json() {
    static std::string json;
    EnsureScene();

    json = "[";
    bool first = true;
    for (auto it = g_modelRegistry.begin(); it != g_modelRegistry.end(); ++it) {
        const auto modelId = it->first;
        auto& meta = it->second;

        auto model = g_scene->GetModelById(static_cast<int>(modelId));
        if (model == nullptr) { continue; }
        meta.visible = model->GetVisibility();

        if (!first) { json += ","; }
        first = false;

        json += "{";
        json += "\"modelId\":" + std::to_string(modelId) + ",";
        json += "\"name\":\"" + EscapeJsonString(meta.name) + "\",";
        json += "\"visible\":" + std::string(meta.visible ? "true" : "false") + ",";
        json += "\"isActive\":" + std::string(modelId == g_activeModelId ? "true" : "false") + ",";
        json += "\"sourceType\":\"" + EscapeJsonString(meta.sourceType) + "\",";
        json += "\"loadTime\":" + std::to_string(meta.loadTime);
        json += "}";
    }
    json += "]";
    return json.c_str();
}

int igw_set_active_model(int modelId) {
    EnsureScene();
    if (g_scene == nullptr) return 0;

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) return 0;

    g_scene->SetCurrentModel(modelId);
    g_activeModelId = static_cast<IGuint>(modelId);
    g_scene->Update();
    return 1;
}

int igw_set_model_visibility(int modelId, int visible) {
    EnsureScene();
    if (g_scene == nullptr) return 0;

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) return 0;

    const bool on = (visible != 0);
    g_scene->ChangeModelVisibility(modelId, on);
    auto it = g_modelRegistry.find(static_cast<IGuint>(modelId));
    if (it != g_modelRegistry.end()) { it->second.visible = on; }
    g_scene->Update();
    return 1;
}

int igw_remove_model(int modelId) {
    EnsureScene();
    if (g_scene == nullptr) return 0;

    auto model = g_scene->GetModelById(modelId);
    if (model == nullptr) return 0;

    g_scene->RemoveModel(static_cast<IGuint>(modelId));
    g_modelRegistry.erase(static_cast<IGuint>(modelId));
    SyncActiveModelIdFromScene();
    g_scene->Update();
    return 1;
}

int igw_remove_all_models() {
    EnsureScene();
    if (g_scene == nullptr) return 0;

    RemoveAllUserModels();
    g_scene->Update();
    return 1;
}

int igw_set_view_style(int styleMask) {
    EnsureScene();
    auto model = GetActiveModel();
    if (model == nullptr) return 0;

    auto drawObj = DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) return 0;

    drawObj->SetViewStyle(static_cast<IGenum>(styleMask));
    g_scene->Update();
    return 1;
}

int igw_set_scalar_field(int attributeIndex, int dimension, int dataLocation) {
    EnsureScene();
    auto model = GetActiveModel();
    if (model == nullptr) return -1;

    auto drawObj = DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) return -2;

    if (attributeIndex < 0) {
        model->ViewCloudPicture(-1, -1);
        return 1;
    }

    auto attrs = drawObj->GetAttributeSet()->GetAllAttributes();
    if (attrs == nullptr || attributeIndex >= static_cast<int>(attrs->GetNumberOfElements())) { return -3; }

    auto attr = attrs->GetElement(attributeIndex);
    if (attr.isDeleted || attr.pointer == nullptr) { return -3; }

    const int attrDim = attr.pointer->GetDimension();
    if (dimension < -1 || (attrDim > 0 && dimension >= attrDim)) { return -5; }

    if (dataLocation == IG_POINT || dataLocation == IG_CELL) {
        if (attr.attachmentType != dataLocation) { return -4; }
    }

    model->ViewCloudPicture(attributeIndex, dimension);
    return 1;
}

int igw_set_surface_shading_mode(int mode) {
    EnsureScene();
    if (g_scene == nullptr) return 0;
    g_scene->SetSurfaceShadingMode(mode);
    g_scene->Update();
    return 1;
}

int igw_set_background_color(int r, int g, int b) {
    EnsureScene();
    if (g_scene == nullptr) return 0;

    // Keep input robust for web-side callers.
    r = std::max(0, std::min(255, r));
    g = std::max(0, std::min(255, g));
    b = std::max(0, std::min(255, b));

    g_scene->SetBackGround(r, g, b);
    g_scene->Update();
    return 1;
}

const char* igw_get_attribute_list_json() {
    static std::string json;

    EnsureScene();
    auto model = GetActiveModel();
    if (model == nullptr) {
        json = "[]";
        return json.c_str();
    }

    auto drawObj = DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (drawObj == nullptr) {
        json = "[]";
        return json.c_str();
    }

    auto attrs = drawObj->GetAttributeSet()->GetAllAttributes();
    if (attrs == nullptr) {
        json = "[]";
        return json.c_str();
    }

    json = "[";
    bool first = true;
    const auto n = static_cast<int>(attrs->GetNumberOfElements());
    for (int i = 0; i < n; ++i) {
        const auto& attr = attrs->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }

        if (!first) { json += ","; }
        first = false;

        const std::string name = EscapeJsonString(attr.pointer->GetName());
        const int dim = attr.pointer->GetDimension();

        json += "{";
        json += "\"index\":" + std::to_string(i) + ",";
        json += "\"name\":\"" + name + "\",";
        json += "\"type\":" + std::to_string(static_cast<int>(attr.type)) + ",";
        json += "\"attachment\":" + std::to_string(static_cast<int>(attr.attachmentType)) + ",";
        json += "\"dimension\":" + std::to_string(dim);
        json += "}";
    }
    json += "]";
    return json.c_str();
}

void igw_render_frame() {
    if (g_window == nullptr) return;
    g_window->RenderOneFrame();
}

void igw_reset_camera() {
    if (g_scene == nullptr) return;
    g_scene->ResetCameraView();
}

void igw_send_mouse_event(int type, int button, float x, float y, double delta) {
    if (g_interactor == nullptr) return;

    iGame::IEvent event;
    switch (type) {
        case 0:
            event.type = iGame::IEvent::MousePress;
            break;
        case 1:
            event.type = iGame::IEvent::MouseMove;
            break;
        case 2:
            event.type = iGame::IEvent::MouseRelease;
            break;
        case 3:
            event.type = iGame::IEvent::Wheel;
            break;
        default:
            return;
    }

    switch (button) {
        case 1:
            event.button = iGame::MouseButton::LeftButton;
            break;
        case 2:
            event.button = iGame::MouseButton::RightButton;
            break;
        case 4:
            event.button = iGame::MouseButton::MiddleButton;
            break;
        default:
            event.button = iGame::MouseButton::NoButton;
            break;
    }

    event.pos.x = x;
    event.pos.y = y;
    event.delta = delta;

    g_interactor->FilterEvent(event);
}

} // extern "C"
