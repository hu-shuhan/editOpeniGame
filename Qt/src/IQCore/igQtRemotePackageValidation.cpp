#include <IQCore/igQtRemotePackageValidation.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
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
