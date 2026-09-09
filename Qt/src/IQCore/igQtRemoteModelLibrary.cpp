#include <IQCore/igQtRemoteModelLibrary.h>

#include <IQCore/igQtFileLoader.h>
#include <IQCore/igQtRemoteCatalogClient.h>

#include <QAbstractItemView>
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTableWidget>
#include <QVBoxLayout>

#include <limits>

namespace
{
constexpr auto SettingsGroup = "RemoteModelLibrary";

QString DefaultCacheRoot()
{
#if defined(Q_OS_WIN)
    const QStorageInfo dataDrive(QStringLiteral("D:/"));
    const QFileInfo dataDriveRoot(QStringLiteral("D:/"));
    if (dataDrive.isValid() && dataDrive.isReady() && !dataDrive.isReadOnly() &&
        dataDriveRoot.isDir() && dataDriveRoot.isWritable()) {
        return QStringLiteral("D:/iGameVis-cs-cache");
    }
#endif
    QString root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (root.isEmpty()) { root = QDir::tempPath(); }
    return QDir(root).filePath(QStringLiteral("remote-models"));
}

QString FormatBytes(quint64 size)
{
    static const char* const units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double value = static_cast<double>(size);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    if (unit == 0) { return QStringLiteral("%1 B").arg(size); }
    return QStringLiteral("%1 %2").arg(value, 0, 'f', value < 10.0 ? 2 : 1)
            .arg(QString::fromLatin1(units[unit]));
}

QString FormatModificationTime(qint64 ticks)
{
    // Current servers expose filesystem-clock ticks.  When a server happens
    // to use Unix seconds or milliseconds, make the value friendly; otherwise
    // keep the lossless tick count visible instead of guessing its epoch.
    constexpr qint64 year2000Seconds = 946684800ll;
    constexpr qint64 year3000Seconds = 32503680000ll;
    qint64 milliseconds = 0;
    if (ticks >= year2000Seconds && ticks <= year3000Seconds) {
        milliseconds = ticks * 1000ll;
    } else if (ticks >= year2000Seconds * 1000ll &&
               ticks <= year3000Seconds * 1000ll) {
        milliseconds = ticks;
    }
    if (milliseconds != 0) {
        return QDateTime::fromMSecsSinceEpoch(milliseconds, Qt::UTC)
                .toLocalTime()
                .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }
    return QString::number(ticks);
}

class NumericTableItem final : public QTableWidgetItem
{
public:
    NumericTableItem(const QString& text, quint64 sortValue)
        : QTableWidgetItem(text)
    {
        setData(Qt::UserRole, sortValue);
    }

    bool operator<(const QTableWidgetItem& other) const override
    {
        return data(Qt::UserRole).toULongLong() < other.data(Qt::UserRole).toULongLong();
    }
};

class SignedNumericTableItem final : public QTableWidgetItem
{
public:
    SignedNumericTableItem(const QString& text, qint64 sortValue)
        : QTableWidgetItem(text)
    {
        setData(Qt::UserRole, sortValue);
    }

    bool operator<(const QTableWidgetItem& other) const override
    {
        return data(Qt::UserRole).toLongLong() < other.data(Qt::UserRole).toLongLong();
    }
};
} // namespace

igQtRemoteModelLibrary::igQtRemoteModelLibrary(igQtFileLoader* fileLoader,
                                               QWidget* parent)
    : QWidget(parent, Qt::Window)
    , m_FileLoader(fileLoader)
    , m_CatalogClient(new igQtRemoteCatalogClient(this))
{
    setObjectName(QStringLiteral("RemoteModelLibrary"));
    setWindowTitle(QStringLiteral("Remote Model Library"));
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_QuitOnClose, false);
    resize(920, 560);
    setMinimumSize(720, 420);

    QSettings settings;
    settings.beginGroup(QString::fromLatin1(SettingsGroup));
    const QString initialHost = settings.value(QStringLiteral("host"),
                                                QStringLiteral("127.0.0.1")).toString();
    const int initialPort = settings.value(QStringLiteral("port"), 34567).toInt();
    const QString initialCache = settings.value(QStringLiteral("cacheRoot"),
                                                 DefaultCacheRoot()).toString();
    settings.endGroup();

    auto* endpointBox = new QGroupBox(QStringLiteral("Server and cache"), this);
    auto* endpointLayout = new QFormLayout(endpointBox);
    endpointLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_HostEdit = new QLineEdit(initialHost, endpointBox);
    m_HostEdit->setObjectName(QStringLiteral("RemoteCatalogHost"));
    m_HostEdit->setClearButtonEnabled(true);
    endpointLayout->addRow(QStringLiteral("Host"), m_HostEdit);

    m_PortSpin = new QSpinBox(endpointBox);
    m_PortSpin->setObjectName(QStringLiteral("RemoteCatalogPort"));
    m_PortSpin->setRange(1, std::numeric_limits<quint16>::max());
    m_PortSpin->setValue(qBound(1, initialPort, 65535));
    endpointLayout->addRow(QStringLiteral("Port"), m_PortSpin);

    auto* cacheRow = new QWidget(endpointBox);
    auto* cacheLayout = new QHBoxLayout(cacheRow);
    cacheLayout->setContentsMargins(0, 0, 0, 0);
    m_CacheEdit = new QLineEdit(initialCache, cacheRow);
    m_CacheEdit->setObjectName(QStringLiteral("RemoteCatalogCache"));
    auto* browseButton = new QPushButton(QStringLiteral("Browse..."), cacheRow);
    browseButton->setObjectName(QStringLiteral("RemoteCatalogBrowse"));
    cacheLayout->addWidget(m_CacheEdit, 1);
    cacheLayout->addWidget(browseButton);
    endpointLayout->addRow(QStringLiteral("Cache"), cacheRow);

    m_FetchButton = new QPushButton(QStringLiteral("Fetch"), endpointBox);
    m_FetchButton->setObjectName(QStringLiteral("RemoteCatalogFetch"));
    endpointLayout->addRow(QString(), m_FetchButton);

    m_Table = new QTableWidget(this);
    m_Table->setObjectName(QStringLiteral("RemoteCatalogTable"));
    m_Table->setColumnCount(4);
    m_Table->setHorizontalHeaderLabels(
            {QStringLiteral("Name"), QStringLiteral("Size"),
             QStringLiteral("Modified"), QStringLiteral("Version")});
    m_Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_Table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_Table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_Table->setAlternatingRowColors(true);
    m_Table->setSortingEnabled(true);
    m_Table->verticalHeader()->setVisible(false);
    m_Table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_Table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_Table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_Table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
    m_Table->setColumnWidth(3, 260);

    auto* footer = new QHBoxLayout;
    m_StatusLabel = new QLabel(QStringLiteral("Choose an endpoint, then click Fetch."), this);
    m_StatusLabel->setObjectName(QStringLiteral("RemoteCatalogStatus"));
    m_StatusLabel->setWordWrap(true);
    m_OpenButton = new QPushButton(QStringLiteral("Open"), this);
    m_OpenButton->setObjectName(QStringLiteral("RemoteCatalogOpen"));
    m_OpenButton->setDefault(true);
    m_CancelButton = new QPushButton(QStringLiteral("Cancel"), this);
    m_CancelButton->setObjectName(QStringLiteral("RemoteCatalogCancel"));
    footer->addWidget(m_StatusLabel, 1);
    footer->addWidget(m_OpenButton);
    footer->addWidget(m_CancelButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(endpointBox);
    layout->addWidget(m_Table, 1);
    layout->addLayout(footer);

    connect(m_FetchButton, &QPushButton::clicked,
            this, &igQtRemoteModelLibrary::FetchCatalog);
    connect(browseButton, &QPushButton::clicked,
            this, &igQtRemoteModelLibrary::BrowseCacheDirectory);
    connect(m_OpenButton, &QPushButton::clicked,
            this, &igQtRemoteModelLibrary::OpenSelectedPackage);
    connect(m_CancelButton, &QPushButton::clicked,
            this, &igQtRemoteModelLibrary::CancelActivity);
    connect(m_Table, &QTableWidget::itemSelectionChanged,
            this, &igQtRemoteModelLibrary::UpdateActions);
    connect(m_Table, &QTableWidget::cellDoubleClicked,
            this, [this](int, int) { OpenSelectedPackage(); });

    auto invalidateCatalog = [this]() {
        if (!m_CatalogBusy && !m_PackageBusy && m_Table->rowCount() != 0) {
            m_Table->clearContents();
            m_Table->setRowCount(0);
            m_CatalogHost.clear();
            m_CatalogPort = 0;
            m_StatusLabel->setText(
                    QStringLiteral("Endpoint changed; click Fetch to refresh the catalog."));
        }
        UpdateActions();
    };
    connect(m_HostEdit, &QLineEdit::textEdited, this,
            [invalidateCatalog](const QString&) { invalidateCatalog(); });
    connect(m_PortSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [invalidateCatalog](int) { invalidateCatalog(); });

    connect(m_CatalogClient, &igQtRemoteCatalogClient::RunningChanged,
            this, &igQtRemoteModelLibrary::SetCatalogBusy);
    connect(m_CatalogClient, &igQtRemoteCatalogClient::StatusChanged,
            m_StatusLabel, &QLabel::setText);
    connect(m_CatalogClient, &igQtRemoteCatalogClient::CatalogReady,
            this, [this](const QVector<igQtRemoteCatalogEntry>& entries) {
                m_Table->setSortingEnabled(false);
                m_Table->clearContents();
                m_Table->setRowCount(entries.size());
                for (int row = 0; row < entries.size(); ++row) {
                    const auto& entry = entries[row];
                    const QString shownName = entry.displayName.isEmpty()
                            ? entry.packageId : entry.displayName;
                    auto* nameItem = new QTableWidgetItem(shownName);
                    nameItem->setData(Qt::UserRole, entry.packageId);
                    nameItem->setToolTip(QStringLiteral("Package id: %1\nArchive: %2")
                                                 .arg(entry.packageId, entry.fileName));
                    auto* sizeItem = new NumericTableItem(
                            FormatBytes(entry.fileSize), entry.fileSize);
                    sizeItem->setToolTip(QStringLiteral("%1 bytes").arg(entry.fileSize));
                    auto* modifiedItem = new SignedNumericTableItem(
                            FormatModificationTime(entry.mtimeTicks), entry.mtimeTicks);
                    modifiedItem->setToolTip(QStringLiteral("Server mtime ticks: %1")
                                                     .arg(entry.mtimeTicks));
                    auto* versionItem = new QTableWidgetItem(entry.versionToken);
                    versionItem->setToolTip(entry.versionToken);
                    m_Table->setItem(row, 0, nameItem);
                    m_Table->setItem(row, 1, sizeItem);
                    m_Table->setItem(row, 2, modifiedItem);
                    m_Table->setItem(row, 3, versionItem);
                }
                m_Table->setSortingEnabled(true);
                if (!entries.isEmpty()) {
                    m_Table->selectRow(0);
                    m_Table->scrollToTop();
                }
                m_StatusLabel->setText(entries.isEmpty()
                        ? QStringLiteral("The server catalog is empty.")
                        : QStringLiteral("Fetched %1 remote model(s) from %2.")
                                  .arg(entries.size())
                                  .arg(CurrentEndpointDescription()));
                UpdateActions();
            });
    connect(m_CatalogClient, &igQtRemoteCatalogClient::Failed,
            this, [this](const QString& message) {
                m_StatusLabel->setText(QStringLiteral("Catalog fetch failed: %1").arg(message));
            });
    connect(m_CatalogClient, &igQtRemoteCatalogClient::Cancelled,
            this, [this]() {
                m_StatusLabel->setText(QStringLiteral("Catalog fetch cancelled."));
            });

    if (m_FileLoader != nullptr) {
        connect(m_FileLoader, &igQtFileLoader::RemotePackageRunningChanged,
                this, &igQtRemoteModelLibrary::SetPackageBusy);
        connect(m_FileLoader, &igQtFileLoader::RemotePackageStatusChanged,
                this, [this](const QString& message) {
                    if (m_PackageBusy) { m_StatusLabel->setText(message); }
                });
        connect(m_FileLoader, &igQtFileLoader::RemotePackageFailed,
                this, [this](const QString& message) {
                    m_PackageFailed = true;
                    m_StatusLabel->setText(QStringLiteral("Open failed: %1").arg(message));
                });
        connect(m_FileLoader, &igQtFileLoader::RemotePackageDatasetOpened,
                this, [this](const QString&) {
                    m_PackageOpened = true;
                    m_StatusLabel->setText(QStringLiteral("Remote model loaded."));
                });
        connect(m_FileLoader, &igQtFileLoader::RemotePackageFinished,
                this, [this]() {
                    if (!m_PackageFailed &&
                        !m_StatusLabel->text().contains(QStringLiteral("cancel"), Qt::CaseInsensitive)) {
                        m_StatusLabel->setText(m_PackageOpened
                                ? QStringLiteral("Remote model loaded.")
                                : QStringLiteral("Remote package request completed, but no model was opened."));
                    }
                });
        SetPackageBusy(m_FileLoader->IsRemotePackageRunning());
    }
    UpdateActions();
}

igQtRemoteModelLibrary::~igQtRemoteModelLibrary()
{
    SaveSettings();
}

void igQtRemoteModelLibrary::closeEvent(QCloseEvent* event)
{
    SaveSettings();
    QWidget::closeEvent(event);
}

void igQtRemoteModelLibrary::FetchCatalog()
{
    if (m_CatalogBusy || m_PackageBusy) { return; }
    const QString host = m_HostEdit->text().trimmed();
    if (host.isEmpty()) {
        m_StatusLabel->setText(QStringLiteral("Enter a server host before fetching."));
        m_HostEdit->setFocus();
        return;
    }

    m_Table->clearContents();
    m_Table->setRowCount(0);
    m_CatalogHost = host;
    m_CatalogPort = static_cast<quint16>(m_PortSpin->value());
    SaveSettings();
    if (!m_CatalogClient->Fetch(m_CatalogHost, m_CatalogPort)) {
        m_StatusLabel->setText(QStringLiteral("A catalog request is already active."));
    }
    UpdateActions();
}

void igQtRemoteModelLibrary::BrowseCacheDirectory()
{
    const QString selected = QFileDialog::getExistingDirectory(
            this, QStringLiteral("Choose remote model cache"), m_CacheEdit->text());
    if (!selected.isEmpty()) { m_CacheEdit->setText(QDir::cleanPath(selected)); }
}

void igQtRemoteModelLibrary::OpenSelectedPackage()
{
    if (m_CatalogBusy || m_PackageBusy || m_FileLoader == nullptr) { return; }
    const QString packageId = SelectedPackageId();
    const QString cacheRoot = m_CacheEdit->text().trimmed();
    if (packageId.isEmpty()) {
        m_StatusLabel->setText(QStringLiteral("Select one remote model to open."));
        return;
    }
    if (cacheRoot.isEmpty()) {
        m_StatusLabel->setText(QStringLiteral("Choose a cache directory before opening."));
        m_CacheEdit->setFocus();
        return;
    }
    if (m_CatalogHost.isEmpty() || m_CatalogPort == 0) {
        m_StatusLabel->setText(QStringLiteral("Fetch the catalog again before opening."));
        return;
    }

    const QString packageCache = igQtRemotePackageCacheDirectory(
            cacheRoot, m_CatalogHost, m_CatalogPort, packageId);
    if (packageCache.isEmpty() || !QDir().mkpath(packageCache)) {
        m_StatusLabel->setText(QStringLiteral("Cannot create package cache: %1")
                                       .arg(packageCache));
        return;
    }

    const quint64 packageSize = SelectedPackageSize();
    const QStorageInfo storage(packageCache);
    if (storage.isValid() && storage.isReady() &&
        packageSize > static_cast<quint64>(storage.bytesAvailable())) {
        m_StatusLabel->setText(
                QStringLiteral("Not enough free space in the cache: the %1 archive alone "
                               "exceeds the %2 available on %3. Extraction needs additional space.")
                        .arg(FormatBytes(packageSize),
                             FormatBytes(static_cast<quint64>(storage.bytesAvailable())),
                             storage.rootPath()));
        return;
    }

    SaveSettings();
    m_PackageFailed = false;
    m_PackageOpened = false;
    if (!m_FileLoader->OpenRemotePackage(
                m_CatalogHost, m_CatalogPort, packageId, packageCache)) {
        if (!m_PackageFailed) {
            m_StatusLabel->setText(QStringLiteral("Could not start the package request."));
        }
        UpdateActions();
    }
}

void igQtRemoteModelLibrary::CancelActivity()
{
    if (m_CatalogBusy) {
        m_StatusLabel->setText(QStringLiteral("Cancelling catalog fetch..."));
        m_CatalogClient->Cancel();
    } else if (m_PackageBusy && m_FileLoader != nullptr) {
        m_StatusLabel->setText(QStringLiteral("Cancelling remote package request..."));
        m_FileLoader->CancelRemotePackage();
    } else {
        close();
    }
}

void igQtRemoteModelLibrary::UpdateActions()
{
    const bool busy = m_CatalogBusy || m_PackageBusy;
    m_HostEdit->setEnabled(!busy);
    m_PortSpin->setEnabled(!busy);
    m_CacheEdit->setEnabled(!busy);
    m_FetchButton->setEnabled(!busy && !m_HostEdit->text().trimmed().isEmpty());
    m_Table->setEnabled(!busy);
    m_OpenButton->setEnabled(!busy && !SelectedPackageId().isEmpty());
    m_CancelButton->setEnabled(true);
}

void igQtRemoteModelLibrary::SetCatalogBusy(bool busy)
{
    m_CatalogBusy = busy;
    UpdateActions();
}

void igQtRemoteModelLibrary::SetPackageBusy(bool busy)
{
    m_PackageBusy = busy;
    UpdateActions();
}

void igQtRemoteModelLibrary::SaveSettings() const
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(SettingsGroup));
    settings.setValue(QStringLiteral("host"), m_HostEdit->text().trimmed());
    settings.setValue(QStringLiteral("port"), m_PortSpin->value());
    settings.setValue(QStringLiteral("cacheRoot"), m_CacheEdit->text().trimmed());
    settings.endGroup();
}

QString igQtRemoteModelLibrary::SelectedPackageId() const
{
    const auto rows = m_Table->selectionModel()->selectedRows(0);
    if (rows.size() != 1) { return {}; }
    const QTableWidgetItem* const item = m_Table->item(rows.front().row(), 0);
    return item == nullptr ? QString() : item->data(Qt::UserRole).toString();
}

quint64 igQtRemoteModelLibrary::SelectedPackageSize() const
{
    const auto rows = m_Table->selectionModel()->selectedRows(0);
    if (rows.size() != 1) { return 0; }
    const QTableWidgetItem* const item = m_Table->item(rows.front().row(), 1);
    return item == nullptr ? 0 : item->data(Qt::UserRole).toULongLong();
}

QString igQtRemoteModelLibrary::CurrentEndpointDescription() const
{
    return QStringLiteral("%1:%2").arg(m_CatalogHost).arg(m_CatalogPort);
}
