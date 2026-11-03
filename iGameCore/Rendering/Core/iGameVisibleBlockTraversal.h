#pragma once

#include "iGameDrawObject.h"
#include <functional>
#include <unordered_set>

IGAME_NAMESPACE_BEGIN
namespace RenderingDetail {
template<typename Functor>
void ForEachVisibleLeaf(DataObject::Pointer root, Functor&& visit) {
    std::unordered_set<DataObject*> active;
    std::function<void(DataObject::Pointer, unsigned)> walk;
    walk = [&](DataObject::Pointer object, unsigned depth) {
        if (!object || depth > 64 || !active.insert(object.get()).second) return;
        auto draw = DynamicCast<DrawObject>(object);
        if (!draw || draw->GetVisibility()) {
            if (object->HasSubDataObject()) {
                for (auto it = object->SubDataObjectIteratorBegin(); it != object->SubDataObjectIteratorEnd(); ++it)
                    walk(it->second, depth + 1);
            } else if (draw) {
                visit(draw);
            }
        }
        active.erase(object.get());
    };
    walk(root, 0);
}
}
IGAME_NAMESPACE_END
