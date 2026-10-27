#pragma once

#include "iGameDataObject.h"
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN
namespace ExtractionDetail {
ArrayObject::Pointer CopyArray(ArrayObject::Pointer source, const std::vector<igIndex>& ids);
bool CopyAttributes(DataObject::Pointer input, DataObject::Pointer output,
                    const std::vector<igIndex>& pointIds, const std::vector<igIndex>& cellIds,
                    bool copyCells, std::string& error);
}
IGAME_NAMESPACE_END
