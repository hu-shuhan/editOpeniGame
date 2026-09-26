#pragma once

#include <IQCore/igQtExportModule.h>

#include <QString>

/** Returns true for symbolic links and, on Windows, any reparse point. */
IG_QT_MODULE_EXPORT bool igQtIsLinkOrReparsePoint(const QString& path);

/** Finds a validated VTM entry point, or one standalone root file handled by
 * iGame::FileIO or the local XML spline dispatch. VTM references and standalone
 * XML VTK headers are checked before opening. Other existing reader formats are
 * containment-checked here and then validated by their normal reader.
 */
IG_QT_MODULE_EXPORT QString igQtFindRemoteDatasetEntryPoint(
        const QString& packageRoot, QString& errorMessage);

/**
 * Validates every VTM file reference that iGameVTMReader will consume.
 *
 * The reader chooses Block mode whenever vtkMultiBlockDataSet has a direct
 * Block child.  In that mode it reads the `file` attribute from every direct
 * child element of each Block, regardless of that child's tag name.  Without
 * a Block it reads direct DataSet children.  This validator deliberately
 * mirrors that selection rule before applying package containment checks.
 */
IG_QT_MODULE_EXPORT bool igQtValidateRemoteVtmManifest(
        const QString& manifestPath,
        const QString& packageRoot,
        QString& errorMessage);

/** Validates all package-local VTS/VTU/PVD references consumed by iGamePVDReader. */
IG_QT_MODULE_EXPORT bool igQtValidateRemotePvdManifest(
        const QString& manifestPath,
        const QString& packageRoot,
        QString& errorMessage);
