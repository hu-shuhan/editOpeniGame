#pragma once

#include <QCryptographicHash>
#include <QDataStream>
#include <QString>

// Length-delimited, content-addressed identity of a fully validated INFO
// response. Never construct a cache hit from the package display name alone.
inline QString igQtRemoteModelIdentity(const QString& address, quint16 port,
                                      const QString& packageId, const QString& fileName,
                                      const QString& versionToken, const QByteArray& sha256,
                                      quint64 fileSize)
{
    if (address.trimmed().isEmpty() || port == 0 || packageId.isEmpty() ||
        fileName.isEmpty() || versionToken.isEmpty() || sha256.size() != 32 || fileSize == 0) {
        return {};
    }
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_12);
    stream << QStringLiteral("igame-resident-static-v1")
           << address.trimmed().toLower() << port << packageId << fileName
           << versionToken << sha256 << fileSize;
    return QString::fromLatin1(QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex());
}
