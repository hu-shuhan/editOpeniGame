#include <IQCore/igQtRemotePackageValidation.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QVector>
#include <QXmlStreamReader>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace
{
constexpr qsizetype MaximumReferenceCount = 1000000;

// Keep this list aligned with the formats dispatched by FileIO::ReadFile(),
// plus the XML spline path dispatched by igQtFileLoader. The remote package
// layer cannot call either dispatch function here because the standalone
// validation test target is intentionally independent from the application.
bool IsRemoteReaderExtension(const QString& suffix)
{
    static const QSet<QString> extensions = {
        QStringLiteral("vtk"), QStringLiteral("igc"), QStringLiteral("igcm"),
        QStringLiteral("obj"), QStringLiteral("off"), QStringLiteral("mesh"),
        QStringLiteral("stl"), QStringLiteral("ply"), QStringLiteral("xml"),
        QStringLiteral("pvd"),
        QStringLiteral("vts"), QStringLiteral("vtu"), QStringLiteral("vtp"),
        QStringLiteral("vtm"), QStringLiteral("inp"), QStringLiteral("cas"),
        QStringLiteral("bdf"), QStringLiteral("ccm"), QStringLiteral("rst"),
        QStringLiteral("rth"), QStringLiteral("d3plot")
#if defined(CGNS_ENABLE)
        , QStringLiteral("cgns")
#endif
#if defined(AbqSDK_ENABLE)
        , QStringLiteral("odb")
#endif
    };
    return extensions.contains(suffix.toLower());
}

bool IsRemoteManifestExtension(const QString& suffix)
{
    // These formats describe other files and must win over their leaf files
    // when both are present in the archive root.
    const QString normalized = suffix.toLower();
    return normalized == QStringLiteral("igcm") || normalized == QStringLiteral("pvd");
}

bool IsD3PlotEntryPoint(const QFileInfo& info)
{
    const QString baseName = info.fileName().toLower();
    if (baseName == QStringLiteral("d3plot") ||
        baseName == QStringLiteral("d3plot.d3plot")) {
        return true;
    }
    if (!baseName.startsWith(QStringLiteral("d3plot"))) { return false; }
    for (qsizetype index = 6; index < baseName.size(); ++index) {
        if (!baseName.at(index).isDigit()) { return false; }
    }
    return baseName.size() > 6;
}

bool IsWithinRoot(const QString& candidate, const QString& root)
{
    const Qt::CaseSensitivity pathCase =
#if defined(Q_OS_WIN)
            Qt::CaseInsensitive;
#else
            Qt::CaseSensitive;
#endif
    const QString cleanCandidate = QDir::cleanPath(candidate);
    QString rootPrefix = QDir::cleanPath(root);
    if (!rootPrefix.endsWith(QLatin1Char('/'))) { rootPrefix.append(QLatin1Char('/')); }
    return cleanCandidate.startsWith(rootPrefix, pathCase);
}

bool AppendReference(const QString& reference,
                     QVector<QString>& references,
                     QString& errorMessage)
{
    if (references.size() >= MaximumReferenceCount) {
        errorMessage = QStringLiteral("VTM contains too many dataset references");
        return false;
    }
    references.push_back(reference);
    return true;
}

bool IsSafePackageReference(const QString& reference)
{
    if (reference.isEmpty() || reference.contains(QLatin1Char('\\')) ||
        reference.contains(QLatin1Char(':')) || QDir::isAbsolutePath(reference)) {
        return false;
    }
    const QString cleanReference = QDir::cleanPath(reference);
    return cleanReference != QStringLiteral("..") &&
           !cleanReference.startsWith(QStringLiteral("../"));
}

bool ValidatePvdRecursive(const QString& manifestPath,
                          const QString& canonicalRoot,
                          QSet<QString>& visiting,
                          QSet<QString>& validated,
                          qsizetype& referenceCount,
                          QString& errorMessage)
{
    const QFileInfo manifestInfo(manifestPath);
    const QString canonicalManifest = manifestInfo.canonicalFilePath();
    if (canonicalManifest.isEmpty() || !manifestInfo.isFile() ||
        igQtIsLinkOrReparsePoint(manifestPath) ||
        !IsWithinRoot(canonicalManifest, canonicalRoot)) {
        errorMessage = QStringLiteral("PVD entry point is missing, a link, or outside the package");
        return false;
    }
    if (validated.contains(canonicalManifest)) { return true; }
    if (visiting.contains(canonicalManifest)) {
        errorMessage = QStringLiteral("PVD contains a cyclic manifest reference");
        return false;
    }

    QFile manifest(manifestPath);
    if (!manifest.open(QIODevice::ReadOnly)) {
        errorMessage = QStringLiteral("Cannot read PVD entry point: %1").arg(manifest.errorString());
        return false;
    }
    visiting.insert(canonicalManifest);
    QXmlStreamReader xml(&manifest);
    const QString manifestDirectory = manifestInfo.absolutePath();
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name() != QStringLiteral("DataSet")) { continue; }
        const auto fileAttribute = xml.attributes().value(QStringLiteral("file"));
        if (fileAttribute.isNull()) { continue; }
        if (++referenceCount > MaximumReferenceCount) {
            errorMessage = QStringLiteral("PVD contains too many dataset references");
            visiting.remove(canonicalManifest);
            return false;
        }
        const QString reference = fileAttribute.toString();
        if (!IsSafePackageReference(reference)) {
            errorMessage = QStringLiteral("PVD contains an unsafe dataset reference: %1").arg(reference);
            visiting.remove(canonicalManifest);
            return false;
        }
        const QFileInfo resolvedInfo(QDir(manifestDirectory).filePath(QDir::cleanPath(reference)));
        const QString resolved = resolvedInfo.canonicalFilePath();
        if (resolved.isEmpty() || !resolvedInfo.isFile() ||
            igQtIsLinkOrReparsePoint(resolvedInfo.filePath()) ||
            !IsWithinRoot(resolved, canonicalRoot)) {
            errorMessage = QStringLiteral("PVD dataset reference is missing, a link, or outside the package: %1")
                    .arg(reference);
            visiting.remove(canonicalManifest);
            return false;
        }
        const QString suffix = resolvedInfo.suffix().toLower();
        if (suffix != QStringLiteral("vtu") && suffix != QStringLiteral("vts") &&
            suffix != QStringLiteral("pvd")) {
            errorMessage = QStringLiteral("PVD references an unsupported dataset type: %1").arg(reference);
            visiting.remove(canonicalManifest);
            return false;
        }
        if (suffix == QStringLiteral("pvd") &&
            !ValidatePvdRecursive(resolvedInfo.filePath(), canonicalRoot, visiting, validated,
                                  referenceCount, errorMessage)) {
            visiting.remove(canonicalManifest);
            return false;
        }
    }
    visiting.remove(canonicalManifest);
    if (xml.hasError()) {
        errorMessage = QStringLiteral("PVD XML is invalid: %1").arg(xml.errorString());
        return false;
    }
    validated.insert(canonicalManifest);
    return true;
}
} // namespace

bool igQtIsLinkOrReparsePoint(const QString& path)
{
    if (QFileInfo(path).isSymLink()) { return true; }
#if defined(Q_OS_WIN)
    const std::wstring nativePath = QDir::toNativeSeparators(path).toStdWString();
    const DWORD attributes = GetFileAttributesW(nativePath.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
    return false;
#endif
}

QString igQtFindRemoteDatasetEntryPoint(const QString& packageRoot, QString& errorMessage)
{
    errorMessage.clear();
    const QFileInfo rootInfo(packageRoot);
    if (!rootInfo.isDir() || igQtIsLinkOrReparsePoint(packageRoot)) {
        errorMessage = QStringLiteral("Extracted package root is not a real directory");
        return {};
    }
    const QString canonicalRoot = rootInfo.canonicalFilePath();
    if (canonicalRoot.isEmpty()) {
        errorMessage = QStringLiteral("Cannot resolve the extracted package root");
        return {};
    }
    const QDir directory(packageRoot);
    const QStringList rootVtms = directory.entryList(
            {QStringLiteral("*.vtm")}, QDir::Files | QDir::NoSymLinks, QDir::Name);
    QString found;
    if (rootVtms.size() == 1) {
        found = directory.filePath(rootVtms.front());
    } else if (rootVtms.size() > 1) {
        errorMessage = QStringLiteral("Package contains more than one root VTM entry point");
        return {};
    } else {
        // Preserve legacy nested-manifest behavior before considering new
        // standalone datasets. A VTM may itself reference root VTP/VTU files.
        QDirIterator iterator(packageRoot, {QStringLiteral("*.vtm")},
                              QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const QString candidate = iterator.next();
            if (!found.isEmpty()) {
                errorMessage = QStringLiteral("Package contains more than one VTM entry point");
                return {};
            }
            found = QDir::cleanPath(candidate);
        }
    }
    if (!found.isEmpty()) {
        return igQtValidateRemoteVtmManifest(found, packageRoot, errorMessage) ? found : QString();
    }

    // A non-VTM package is opened through the same FileIO::ReadFile() path as
    // a local file. Select one root entry point from every reader format that
    // FileIO dispatches, instead of limiting remote packages to VTP/VTU.
    // Manifest-like formats win over their referenced leaf files.
    QFileInfoList candidates;
    QFileInfoList manifests;
    const QFileInfoList files = directory.entryInfoList(
            QDir::Files | QDir::NoSymLinks, QDir::Name);
    for (const QFileInfo& info : files) {
        const QString suffix = info.suffix().toLower();
        if (IsRemoteManifestExtension(suffix)) {
            manifests.push_back(info);
        }
        if (IsRemoteReaderExtension(suffix) || IsD3PlotEntryPoint(info)) {
            candidates.push_back(info);
        }
    }
    if (manifests.size() > 1) {
        errorMessage = QStringLiteral("Package contains more than one root manifest entry point");
        return {};
    }
    if (manifests.size() == 1) {
        found = manifests.front().filePath();
    } else if (candidates.size() == 1) {
        found = candidates.front().filePath();
    } else if (candidates.isEmpty()) {
        errorMessage = QStringLiteral(
                "Package does not contain a supported FileIO reader entry point");
        return {};
    } else {
        errorMessage = QStringLiteral(
                "Package contains multiple possible FileIO reader entry points; add one root model file");
        return {};
    }

    const QFileInfo datasetInfo(found);
    const QString canonicalDataset = datasetInfo.canonicalFilePath();
    if (igQtIsLinkOrReparsePoint(found) || canonicalDataset.isEmpty() ||
        !IsWithinRoot(canonicalDataset, canonicalRoot)) {
        errorMessage = QStringLiteral("Dataset entry point is missing, a link, or outside the package");
        return {};
    }
    QFile dataset(found);
    if (!dataset.open(QIODevice::ReadOnly)) {
        errorMessage = QStringLiteral("Cannot read dataset entry point: %1").arg(dataset.errorString());
        return {};
    }
    // Qt may decode its entire input buffer before yielding the first XML
    // token. Raw appended Float64/Int64 bytes can be invalid UTF-8, so merely
    // returning when Piece is encountered is too late. Only give the parser
    // the bounded XML prefix preceding AppendedData, never its binary payload.
    QByteArray header = dataset.read(1024 * 1024);
    const int appendedStart = header.indexOf("<AppendedData");
    if (appendedStart >= 0) { header.truncate(appendedStart); }
    QXmlStreamReader xml(header);
    const QString suffix = datasetInfo.suffix().toLower();
    if (suffix == QStringLiteral("pvd")) {
        return igQtValidateRemotePvdManifest(found, packageRoot, errorMessage) ? found : QString();
    }
    // Keep the strict XML/header check for XML VTK standalone datasets.
    // Legacy VTK, CGNS, OBJ, STL, solver formats, and binary iGame formats
    // are validated by their existing Reader instead.
    if (suffix != QStringLiteral("vtp") && suffix != QStringLiteral("vtu") &&
        suffix != QStringLiteral("vts")) {
        return found;
    }
    const QString expectedType = suffix == QStringLiteral("vtp")
            ? QStringLiteral("PolyData")
            : (suffix == QStringLiteral("vts")
               ? QStringLiteral("StructuredGrid")
               : QStringLiteral("UnstructuredGrid"));
    int depth = 0;
    bool datasetStarted = false;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            ++depth;
            if (depth == 1 && (xml.name() != QStringLiteral("VTKFile") ||
                              xml.attributes().value(QStringLiteral("type")) != expectedType)) {
                errorMessage = QStringLiteral("Standalone dataset VTKFile type does not match its extension");
                return {};
            }
            if (depth == 2 && xml.name() == expectedType) { datasetStarted = true; }
            if (datasetStarted && depth == 3 && xml.name() == QStringLiteral("Piece")) {
                bool validCount = false;
                xml.attributes().value(QStringLiteral("NumberOfPoints")).toULongLong(&validCount);
                if (!validCount) {
                    errorMessage = QStringLiteral("Standalone dataset Piece has an invalid NumberOfPoints");
                    return {};
                }
                return found;
            }
        } else if (xml.isEndElement()) {
            if (depth == 2) { datasetStarted = false; }
            --depth;
        }
    }
    errorMessage = QStringLiteral("Standalone dataset is missing a valid VTK Piece header within 1 MiB");
    return {};
}

bool igQtValidateRemoteVtmManifest(const QString& manifestPath,
                                   const QString& packageRoot,
                                   QString& errorMessage)
{
    errorMessage.clear();
    const QFileInfo rootInfo(packageRoot);
    if (!rootInfo.isDir() || igQtIsLinkOrReparsePoint(packageRoot)) {
        errorMessage = QStringLiteral(
                "Extracted package root is not a real directory (links and reparse points are rejected)");
        return false;
    }
    const QString canonicalRoot = rootInfo.canonicalFilePath();
    if (canonicalRoot.isEmpty()) {
        errorMessage = QStringLiteral("Cannot resolve the extracted package root");
        return false;
    }

    const QFileInfo manifestInfo(manifestPath);
    if (!manifestInfo.isFile() || igQtIsLinkOrReparsePoint(manifestPath)) {
        errorMessage = QStringLiteral("VTM entry point is missing or is a link");
        return false;
    }
    const QString canonicalManifest = manifestInfo.canonicalFilePath();
    if (canonicalManifest.isEmpty() || !IsWithinRoot(canonicalManifest, canonicalRoot)) {
        errorMessage = QStringLiteral("VTM entry point is outside the extracted package");
        return false;
    }

    QFile manifest(manifestPath);
    if (!manifest.open(QIODevice::ReadOnly)) {
        errorMessage = QStringLiteral("Cannot read VTM entry point: %1")
                .arg(manifest.errorString());
        return false;
    }

    QXmlStreamReader xml(&manifest);
    QVector<QString> blockReferences;
    QVector<QString> flatReferences;
    int depth = 0;
    int multiBlockDepth = -1;
    int activeBlockDepth = -1;
    bool foundMultiBlock = false;
    bool completedMultiBlock = false;
    bool hasDirectBlock = false;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            ++depth;
            if (!foundMultiBlock &&
                xml.name() == QStringLiteral("vtkMultiBlockDataSet")) {
                foundMultiBlock = true;
                multiBlockDepth = depth;
                continue;
            }
            if (multiBlockDepth < 0 || completedMultiBlock) { continue; }

            if (depth == multiBlockDepth + 1 &&
                xml.name() == QStringLiteral("Block")) {
                hasDirectBlock = true;
                activeBlockDepth = depth;
                continue;
            }

            const auto fileAttribute = xml.attributes().value(QStringLiteral("file"));
            if (activeBlockDepth >= 0 && depth == activeBlockDepth + 1) {
                if (!fileAttribute.isNull() &&
                    !AppendReference(fileAttribute.toString(), blockReferences, errorMessage)) {
                    return false;
                }
            } else if (depth == multiBlockDepth + 1 &&
                       xml.name() == QStringLiteral("DataSet")) {
                if (!fileAttribute.isNull() &&
                    !AppendReference(fileAttribute.toString(), flatReferences, errorMessage)) {
                    return false;
                }
            }
        } else if (xml.isEndElement()) {
            if (depth == activeBlockDepth) { activeBlockDepth = -1; }
            if (depth == multiBlockDepth) {
                multiBlockDepth = -1;
                completedMultiBlock = true;
            }
            --depth;
        }
    }
    if (xml.hasError()) {
        errorMessage = QStringLiteral("VTM XML is invalid: %1").arg(xml.errorString());
        return false;
    }
    if (!foundMultiBlock) {
        errorMessage = QStringLiteral("VTM is missing vtkMultiBlockDataSet");
        return false;
    }

    const QVector<QString>& references = hasDirectBlock ? blockReferences : flatReferences;
    if (references.isEmpty()) {
        errorMessage = QStringLiteral("VTM does not reference any dataset files");
        return false;
    }

    const QString manifestDirectory = manifestInfo.absolutePath();
    for (const QString& reference : references) {
        if (reference.isEmpty() || reference.contains(QLatin1Char('\\')) ||
            reference.contains(QLatin1Char(':')) || QDir::isAbsolutePath(reference)) {
            errorMessage = QStringLiteral("VTM contains an unsafe dataset reference: %1")
                    .arg(reference);
            return false;
        }
        const QString cleanReference = QDir::cleanPath(reference);
        if (cleanReference == QStringLiteral("..") ||
            cleanReference.startsWith(QStringLiteral("../"))) {
            errorMessage = QStringLiteral("VTM dataset reference escapes the package: %1")
                    .arg(reference);
            return false;
        }

        const QFileInfo resolvedInfo(QDir(manifestDirectory).filePath(cleanReference));
        const QString resolved = resolvedInfo.canonicalFilePath();
        if (resolved.isEmpty() || !resolvedInfo.isFile() ||
            igQtIsLinkOrReparsePoint(resolvedInfo.filePath())) {
            errorMessage = QStringLiteral("VTM dataset reference is missing or is a link: %1")
                    .arg(reference);
            return false;
        }
        if (!IsWithinRoot(resolved, canonicalRoot)) {
            errorMessage = QStringLiteral(
                    "VTM dataset reference is missing or outside the package: %1")
                    .arg(reference);
            return false;
        }
    }
    return true;
}

bool igQtValidateRemotePvdManifest(const QString& manifestPath,
                                   const QString& packageRoot,
                                   QString& errorMessage)
{
    errorMessage.clear();
    const QFileInfo rootInfo(packageRoot);
    if (!rootInfo.isDir() || igQtIsLinkOrReparsePoint(packageRoot)) {
        errorMessage = QStringLiteral("Extracted package root is not a real directory");
        return false;
    }
    const QString canonicalRoot = rootInfo.canonicalFilePath();
    if (canonicalRoot.isEmpty()) {
        errorMessage = QStringLiteral("Cannot resolve the extracted package root");
        return false;
    }
    QSet<QString> visiting;
    QSet<QString> validated;
    qsizetype referenceCount = 0;
    if (!ValidatePvdRecursive(manifestPath, canonicalRoot, visiting, validated,
                              referenceCount, errorMessage)) {
        return false;
    }
    if (referenceCount == 0) {
        errorMessage = QStringLiteral("PVD does not reference any dataset files");
        return false;
    }
    return true;
}
