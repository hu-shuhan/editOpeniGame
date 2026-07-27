#include "IQCore/igQtDataCodecDecodeSettings.h"

#include <IGDC/iGameDataCodecIOSettings.h>

#include <QSettings>

namespace {

constexpr auto kSettingsOrganization = "iGame";
constexpr auto kSettingsApplication = "iGameVis";
constexpr auto kDecodeSettingsGroup = "DataCodec/Decode";
constexpr auto kPerformanceTierKey = "PerformanceTier";
constexpr auto kDecodeAttributesOnDemandKey = "DecodeAttributesOnDemand";
constexpr auto kEnableDecodedResultCacheKey = "EnableDecodedResultCache";
constexpr auto kDecodedResultCacheFrameLimitKey = "DecodedResultCacheFrameLimit";

QSettings CreateSettings() {
    return QSettings(
        QSettings::IniFormat,
        QSettings::UserScope,
        QString::fromLatin1(kSettingsOrganization),
        QString::fromLatin1(kSettingsApplication));
}

::datacodec::DataCodecDecodeTier DecodeTierFromValue(const int value) {
    const auto tier = static_cast<::datacodec::DataCodecDecodeTier>(value);
    switch (tier) {
        case ::datacodec::DataCodecDecodeTier::Fast:
        case ::datacodec::DataCodecDecodeTier::Balanced:
        case ::datacodec::DataCodecDecodeTier::LowMemory:
            return tier;
    }
    return ::datacodec::DataCodecDecodeTier::Fast;
}

}

igQtDataCodecDecodeSettings igQtDataCodecDecodeSettingsStore::Load() {
    auto storage = CreateSettings();
    storage.beginGroup(QString::fromLatin1(kDecodeSettingsGroup));
    const auto tier = DecodeTierFromValue(storage.value(
        QString::fromLatin1(kPerformanceTierKey),
        static_cast<int>(::datacodec::DataCodecDecodeTier::Fast)).toInt());
    const auto decodeAttributesOnDemand = storage.value(
        QString::fromLatin1(kDecodeAttributesOnDemandKey),
        false).toBool();
    const auto enableDecodedResultCache = storage.value(
        QString::fromLatin1(kEnableDecodedResultCacheKey),
        false).toBool();
    const auto decodedResultCacheFrameLimit = static_cast<std::size_t>(qBound(
        1,
        storage.value(
            QString::fromLatin1(kDecodedResultCacheFrameLimitKey),
            3).toInt(),
        10));
    storage.endGroup();
    return igQtDataCodecDecodeSettings{
        .performanceTier = tier,
        .decodeAttributesOnDemand = decodeAttributesOnDemand,
        .enableDecodedResultCache = enableDecodedResultCache,
        .decodedResultCacheFrameLimit = decodedResultCacheFrameLimit,
    };
}

void igQtDataCodecDecodeSettingsStore::Save(
    const igQtDataCodecDecodeSettings& settings) {
    auto storage = CreateSettings();
    storage.beginGroup(QString::fromLatin1(kDecodeSettingsGroup));
    storage.setValue(
        QString::fromLatin1(kPerformanceTierKey),
        static_cast<int>(settings.performanceTier));
    storage.setValue(
        QString::fromLatin1(kDecodeAttributesOnDemandKey),
        settings.decodeAttributesOnDemand);
    storage.setValue(
        QString::fromLatin1(kEnableDecodedResultCacheKey),
        settings.enableDecodedResultCache);
    storage.setValue(
        QString::fromLatin1(kDecodedResultCacheFrameLimitKey),
        static_cast<qulonglong>(settings.decodedResultCacheFrameLimit));
    storage.endGroup();
    storage.sync();
    Apply(settings);
}

void igQtDataCodecDecodeSettingsStore::Apply(
    const igQtDataCodecDecodeSettings& settings) {
    auto options = iGame::DataCodecIOSettings::GetDefaultDecodeOptions();
    options.tier = settings.performanceTier;
    options.enableDecodedResultCache = settings.enableDecodedResultCache;
    options.decodedResultCacheFrameLimit = settings.decodedResultCacheFrameLimit;
    iGame::DataCodecIOSettings::SetDefaultDecodeOptions(options);
    iGame::DataCodecIOSettings::SetDefaultLoadAllAvailableAttributes(
        !settings.decodeAttributesOnDemand);
}
