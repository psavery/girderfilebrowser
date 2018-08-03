//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
// .NAME girderfilebrowserdialog.h
// .SECTION Description
// .SECTION See Also

#ifndef __smtk_extension_cumulus_girderfilebrowserdialog_h
#define __smtk_extension_cumulus_girderfilebrowserdialog_h

#include "Exports.h"

#include <QDialog>
#include <QMap>
#include <QString>

#include <memory>

class QIcon;
class QModelIndex;
class QNetworkAccessManager;
class QResizeEvent;
class QStandardItemModel;

namespace Ui
{
class GirderFileBrowserDialog;
}

namespace cumulus
{

class GirderFileBrowserFetcher;

class SMTKCUMULUSEXT_EXPORT GirderFileBrowserDialog : public QDialog
{
  Q_OBJECT

public:
  explicit GirderFileBrowserDialog(QNetworkAccessManager* networkAccessManager,
    QWidget* parent = nullptr);

  virtual ~GirderFileBrowserDialog() override;

  void setApiUrl(const QString& url);
  void setGirderToken(const QString& token);

signals:
  // These will not do anything if an update is currently pending
  void changeFolder(const QMap<QString, QString>& parentInfo);
  void goHome();

public slots:
  // A convenience function for authentication success
  void setApiUrlAndGirderToken(const QString& url, const QString& token);

protected:
  void resizeEvent(QResizeEvent* event) override;

private slots:
  void rowActivated(const QModelIndex&);
  void goUpDirectory();

  void finishChangingFolder(const QMap<QString, QString>& newParentInfo,
    const QList<QMap<QString, QString> >& folders,
    const QList<QMap<QString, QString> >& files,
    const QList<QMap<QString, QString> >& rootPath);

  void errorReceived(const QString& message);

private:
  void updateRootPathWidget();

  // Convenience functions...
  QString currentParentName() const { return m_currentParentInfo.value("name"); }
  QString currentParentId() const { return m_currentParentInfo.value("id"); }
  QString currentParentType() const { return m_currentParentInfo.value("type"); }

  // Members
  QNetworkAccessManager* m_networkManager;
  std::unique_ptr<Ui::GirderFileBrowserDialog> m_ui;
  std::unique_ptr<QStandardItemModel> m_itemModel;
  std::unique_ptr<GirderFileBrowserFetcher> m_girderFileBrowserFetcher;

  QMap<QString, QString> m_currentParentInfo;

  // Cache info for each row so we can use it when the user double clicks
  // on the row.
  QList<QMap<QString, QString> > m_cachedRowInfo;
  QList<QMap<QString, QString> > m_currentRootPathInfo;

  // Our icons that we use
  std::unique_ptr<QIcon> m_folderIcon;
  std::unique_ptr<QIcon> m_fileIcon;
};

} // end of namespace

#endif
