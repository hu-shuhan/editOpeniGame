/**
 * @class igQtRemoteModelLibrary
 * @brief Non-modal browser for models published by an iGameVis data server.
 */

#pragma once

#include <IQCore/igQtExportModule.h>

#include <QString>
#include <QWidget>

class QCloseEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class igQtFileLoader;
class igQtRemoteCatalogClient;

class IG_QT_MODULE_EXPORT igQtRemoteModelLibrary final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(igQtRemoteModelLibrary)

public:
    explicit igQtRemoteModelLibrary(igQtFileLoader* fileLoader,
                                    QWidget* parent = nullptr);
    ~igQtRemoteModelLibrary() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void FetchCatalog();
    void BrowseCacheDirectory();
    void OpenSelectedPackage();
    void CancelActivity();
    void UpdateActions();

private:
    void SetCatalogBusy(bool busy);
    void SetPackageBusy(bool busy);
    void SaveSettings() const;
    QString SelectedPackageId() const;
    quint64 SelectedPackageSize() const;
    QString CurrentEndpointDescription() const;

    igQtFileLoader* m_FileLoader{nullptr};
    igQtRemoteCatalogClient* m_CatalogClient{nullptr};
    QLineEdit* m_HostEdit{nullptr};
    QSpinBox* m_PortSpin{nullptr};
    QLineEdit* m_CacheEdit{nullptr};
    QPushButton* m_FetchButton{nullptr};
    QTableWidget* m_Table{nullptr};
    QLabel* m_StatusLabel{nullptr};
    QPushButton* m_OpenButton{nullptr};
    QPushButton* m_CancelButton{nullptr};
    QString m_CatalogHost;
    quint16 m_CatalogPort{0};
    bool m_CatalogBusy{false};
    bool m_PackageBusy{false};
    bool m_PackageFailed{false};
    bool m_PackageOpened{false};
};
