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
#include <map>

class QIcon;
class QModelIndex;
class QNetworkAccessManager;
class QStandardItemModel;

namespace Ui
{
class GirderFileBrowserDialog;
}

namespace cumulus
{

class GirderRequest;

class SMTKCUMULUSEXT_EXPORT GirderFileBrowserDialog : public QDialog
{
  Q_OBJECT

public:
  explicit GirderFileBrowserDialog(QNetworkAccessManager* networkAccessManager,
    QWidget* parent = nullptr);

  virtual ~GirderFileBrowserDialog() override;

  void setGirderUrl(const QString& url) { m_girderUrl = url; }
  void setGirderToken(const QString& token) { m_girderToken = token; }

  // This will not do anything if an update is currently pending
  void updateBrowser(const QString& parentName, const QString& parentId, const QString& parentType);

public slots:
  // A convenience function for authentication success
  void setApiUrlAndGirderToken(const QString& url, const QString& token);

private slots:
  void itemDoubleClicked(const QModelIndex&);
  void goUpDirectory();
  void goHome();

  void finishUpdatingBrowserList();

  void errorReceived(const QString& message);

private:
  // The generic updates
  void updateCurrentFolders();
  void updateCurrentItems();
  void updateRootPath();

  void updateBrowserList();

  // Checks to see if all steps are complete and there are no errors
  void updateBrowserListIfReady();

  // The special cases in the top two level directories
  void updateBrowserListForRoot();
  void updateBrowserListForUsers();
  void updateBrowserListForCollections();

  // A general update function called by updateBrowserListForUsers() and
  // updateBrowserListForCollections()
  void updateSecondDirectoryLevel(const QString& type, const QMap<QString, QString>& map);

  // Update the root path widget
  void updateRootPathWidget();

  // Convenience functions...
  QString currentParentName() const { return m_currentParentInfo.value("name"); }
  QString currentParentId() const { return m_currentParentInfo.value("id"); }
  QString currentParentType() const { return m_currentParentInfo.value("type"); }

  // Are updates pending?
  bool browserListUpdatesPending() const;

  // Did any update errors occur?
  bool browserListUpdateErrors() const { return m_browserListUpdateErrorOccurred; }

  // Members
  std::unique_ptr<Ui::GirderFileBrowserDialog> m_ui;
  std::unique_ptr<QStandardItemModel> m_itemModel;
  QNetworkAccessManager* m_networkManager;

  QString m_girderUrl;
  QString m_girderToken;

  // These map ids to names. They are used by updateBrowserList().
  QMap<QString, QString> m_currentFolders;
  QMap<QString, QString> m_currentItems;
  // A hierarchy of folders to the root folder. The first item in the
  // QList should be the user. The rest are folders.
  // The keys "type", "id", and "name" should be present for each entry.
  QList<QMap<QString, QString> > m_currentRootPath;

  // Our requests.
  // These will be deleted when a new request is made.
  // We must use a std::map here because QMap has errors with unique_ptr
  // as value.
  std::map<QString, std::unique_ptr<GirderRequest>> m_updateRequests;

  // Are there any updates pending?
  QMap<QString, bool> m_browserListUpdatesPending;
  // Did any update errors occur?
  bool m_browserListUpdateErrorOccurred;

  // Information about the current parent
  QMap<QString, QString> m_currentParentInfo;

  // Cache info for each row so we can use it when the user double clicks
  // on the row.
  QList<QMap<QString, QString> > m_cachedRowInfo;

  // Our icons that we use
  std::unique_ptr<QIcon> m_folderIcon;
  std::unique_ptr<QIcon> m_fileIcon;
};

inline void GirderFileBrowserDialog::setApiUrlAndGirderToken(const QString& url,
  const QString& token)
{
  setGirderUrl(url);
  setGirderToken(token);
}

inline void GirderFileBrowserDialog::updateBrowserListIfReady()
{
  if (!browserListUpdatesPending() && !browserListUpdateErrors())
    finishUpdatingBrowserList();
}

} // end of namespace

#endif
