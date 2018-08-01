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
class QStandardItemModel;

namespace Ui
{
class GirderFileBrowserDialog;
}

namespace cumulus
{

// The various kinds of girder requests
class GirderRequest;
class ListFoldersRequest;
class ListItemsRequest;
class GetFolderRootPathRequest;
class GetUsersRequest;
class GetCollectionsRequest;
class GetMyUserRequest;

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
  void setApiUrlAndGirderToken(const QString& url, const QString& token)
  {
    setGirderUrl(url);
    setGirderToken(token);
  }

private slots:
  void itemDoubleClicked(const QModelIndex&);
  void goUpDirectory();
  void goHome();

  // We need one of these for each girder request...
  void finishUpdatingFolders(const QMap<QString, QString>& newFolders);
  void finishUpdatingItems(const QMap<QString, QString>& newItems);
  void finishUpdatingRootPath(const QList<QMap<QString, QString> >& newRootpath);
  void finishGoingHome(const QMap<QString, QString>& myUserInfo);
  void finishUpdatingBrowserListForUsers(const QMap<QString, QString>& usersMap);
  void finishUpdatingBrowserListForCollections(const QMap<QString, QString>& collectionsMap);
  void finishUpdatingBrowserList();

  void errorReceived(const QString& message);

private:
  // The generic updates
  void updateCurrentFolders();
  void updateCurrentItems();
  void updateRootPath();

  void updateBrowserList();

  // The special cases in the top two level directories
  void updateBrowserListForRoot();
  void updateBrowserListForUsers();
  void updateBrowserListForCollections();

  // Update the root path widget
  void updateRootPathWidget();

  // Convenience functions...
  QString currentParentName() const { return m_currentParentInfo.value("name"); }
  QString currentParentId() const { return m_currentParentInfo.value("id"); }
  QString currentParentType() const { return m_currentParentInfo.value("type"); }

  bool updatesPending() const;
  bool updateErrors() const { return m_updateErrorOccurred; }

  // Members
  std::unique_ptr<Ui::GirderFileBrowserDialog> m_ui;
  std::unique_ptr<QStandardItemModel> m_itemModel;
  QNetworkAccessManager* m_networkManager;

  QString m_girderUrl;
  QString m_girderToken;

  // These map ids to names
  QMap<QString, QString> m_currentFolders;
  QMap<QString, QString> m_currentItems;
  // A hierarchy of folders to the root folder. The first item in the
  // QList should be the user. The rest are folders.
  // The keys "type", "id", and "name" should be present for each entry.
  QList<QMap<QString, QString> > m_currentRootPath;

  // These will be deleted when a new request is made.
  std::unique_ptr<ListFoldersRequest> m_updateFoldersRequest;
  std::unique_ptr<ListItemsRequest> m_updateItemsRequest;
  std::unique_ptr<GetFolderRootPathRequest> m_updateRootPathRequest;
  std::unique_ptr<GetUsersRequest> m_getUsersRequest;
  std::unique_ptr<GetCollectionsRequest> m_getCollectionsRequest;
  std::unique_ptr<GetMyUserRequest> m_getMyUserRequest;

  QMap<QString, bool> m_updatesPending;
  bool m_updateErrorOccurred;

  QMap<QString, QString> m_currentParentInfo;

  // Cache info for each row so we can use it when the user double clicks
  // on the row.
  QList<QMap<QString, QString> > m_cachedRowInfo;

  // Our icons that we use
  std::unique_ptr<QIcon> m_folderIcon;
  std::unique_ptr<QIcon> m_fileIcon;
};

} // end of namespace

#endif
