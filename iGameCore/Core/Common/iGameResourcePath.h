#ifndef iGameResourcePath_h
#define iGameResourcePath_h

#include <string>

namespace iGame {

// Returns the absolute directory containing the running executable, falling
// back to the current working directory when it cannot be determined.
std::string GetExecutableDirectory();

// Resolves a resource path that is normally expressed relative to the process
// working directory (e.g. "Resources/Shaders/Vertex.vert").  It tries, in
// order: the working directory (previous behaviour), next to the executable,
// and the executable's parent directory (multi-config Debug/Release layout).
// When none exists, the original path is returned unchanged so callers still
// log the familiar value and behave as before when resources are missing.
std::string ResolveResourcePath(const std::string& relativePath);

} // namespace iGame

#endif // iGameResourcePath_h
