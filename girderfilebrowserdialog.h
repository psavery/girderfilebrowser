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
    const QString& girderUrl,
    const QString& girderToken,
    QWidget* parent = nullptr);

  virtual ~GirderFileBrowserDialog() override;

  QString girderUrl() const { return m_girderUrl; }
  void setGirderUrl(const QString& url) { m_girderUrl = url; }

  // This will not do anything if an update is currently pending
  void updateBrowser(const QString& parentName, const QString& parentId, const QString& parentType);

private slots:
  void itemDoubleClicked(const QModelIndex&);
  void goUpDirectory();
  void goHome();

private:
  void updateCurrentFolders();
  void updateCurrentItems();
  void updateRootPath();
  void errorReceived(const QString& message);

  void finishUpdatingFolders(const QMap<QString, QString>& newFolders);
  void finishUpdatingItems(const QMap<QString, QString>& newItems);
  void finishUpdatingRootPath(const QList<QMap<QString, QString> >& newRootpath);
  void finishGoingHome(const QMap<QString, QString>& myUserInfo);

  void updateBrowserList();
  void finishUpdatingBrowserList();

  // The special cases
  void updateBrowserListForRoot();
  void updateBrowserListForUsers();
  void finishUpdatingBrowserListForUsers(const QMap<QString, QString>& usersMap);
  void updateBrowserListForCollections();
  void finishUpdatingBrowserListForCollections(const QMap<QString, QString>& collectionsMap);

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
