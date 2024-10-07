/**
 * @class   iGameFileDialog
 * @brief   iGameFileDialog's brief
 */

#pragma once
#include <QFileDialog>
#include <QTreeView>
#include <QListView>
#include <QFileSystemModel>
#include <QDir>
#include <QDebug>
#include <QTreeWidget>

#include "igQtExportModule.h"
class IG_QT_MODULE_EXPORT iGameFileDialog : public QFileDialog {

    Q_OBJECT
private:
//    QFileSystemModel fileSystemModel;
//    QStandardItemModel customModel;
private:
    void updateFileList(const QString &path);


public:
    explicit iGameFileDialog(QWidget* parent = nullptr);

    static QStringList getOpenFileNames(const QString& caption, const QString& dir, const QString& filter);

    void onDirectoryLoaded(const QString &path);

    void groupFilesByName();

};
