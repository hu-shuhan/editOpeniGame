#ifndef CatScriptLibraryType_h
#define CatScriptLibraryType_h

// COPYRIGHT DASSAULT SYSTEMES 2002

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/**
 * Enum to describe the various kinds of macro
 * libraries supported by the V5 scripting architecture.
 * The macros contained by these libraries can be invoked by calling
 * @href CATScriptUtilities#ExecuteScript
 * @param catScriptLibraryTypeDocument
 * The library physically stores its macros in a V5 document.
 * @param catScriptLibraryTypeDirectory
 * The library physically stores its macros in a filesystem directory.
 * @param catScriptLibraryTypeVBAProject
 * The library physically stores its macros in a VBA project file (with the .catvba extension).
 * @param catScriptLibraryTypeVSTAProject
 * The library physically stores its macros in a VSTA project file (with the .csproj or .vbproj extension).
 */
enum CatScriptLibraryType {
        catScriptLibraryTypeDocument,
        catScriptLibraryTypeDirectory,
        catScriptLibraryTypeVBAProject,
        catScriptLibraryTypeVSTAProject
};

#endif // CatScriptLibraryType_h

