//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include "girderfilebrowserfetcher.h"

#include "girderrequest.h"

#include <QNetworkAccessManager>

namespace cumulus
{

// Set the various names we will use for the requests
static const QString& GET_FOLDERS_REQUEST = "getFoldersRequest";
static const QString& GET_ITEMS_REQUEST = "getItemsRequest";
static const QString& GET_ROOT_PATH_REQUEST = "getRootPathRequest";
static const QString& GET_USERS_REQUEST = "getUsersRequest";
static const QString& GET_COLLECTIONS_REQUEST = "getCollectionsRequest";
static const QString& GET_MY_USER_REQUEST = "getMyUserRequest";

GirderFileBrowserFetcher::GirderFileBrowserFetcher(QNetworkAccessManager* networkManager,
  QObject* parent)
  : QObject(parent)
  , m_networkManager(networkManager)
  , m_fetchInProgress(false)
{
  // These indicate if a folder request is currently pending
  m_folderRequestPending["folders"] = false;
  m_folderRequestPending["items"] = false;
  m_folderRequestPending["rootPath"] = false;

  // Indicate that a fetch is complete if a reply is sent
  connect(this, &GirderFileBrowserFetcher::folderInformation, this, [this]() {
    m_fetchInProgress = false;
  });
  connect(this, &GirderFileBrowserFetcher::error, this, [this]() { m_fetchInProgress = false; });
}

GirderFileBrowserFetcher::~GirderFileBrowserFetcher() = default;

void GirderFileBrowserFetcher::getFolderInformation(const QMap<QString, QString>& parentInfo)
{
  // If we are currently performing a fetch, ignore the request.
  if (m_fetchInProgress)
    return;

  m_fetchInProgress = true;
  m_folderRequestErrorOccurred = false;

  m_currentParentInfo = parentInfo;

  // The first two directory levels will be different from the rest.
  if (currentParentType() == "root")
  {
    getRootFolderInformation();
    return;
  }
  else if (currentParentType() == "Users")
  {
    getUsersFolderInformation();
    return;
  }
  else if (currentParentType() == "Collections")
  {
    getCollectionsFolderInformation();
    return;
  }

  // For the standard case
  getContainingFolders();
  getContainingItems();
  getRootPath();
}

// A convenience function to do three things:
//   1. Call "send()" on the GirderRequest sender.
//   2. Create connections for finish condition.
//   3. Create connections for error condition.
// Sender must be a GirderRequest object, and Receiver must be
// a GirderFileBrowserFetcher object. Slot can be either a
// GirderFileBrowserFetcher function or a lambda.
template<typename Sender, typename Signal, typename Receiver, typename Slot>
static void sendAndConnect(Sender* sender, Signal signal, Receiver* receiver, Slot slot)
{
  sender->send();

  QObject::connect(sender, signal, receiver, slot);
  QObject::connect(
    sender, &GirderRequest::error, receiver, &GirderFileBrowserFetcher::errorReceived);
}

void GirderFileBrowserFetcher::getHomeFolderInformation()
{
  // If we are currently performing a fetch, ignore the request.
  if (m_fetchInProgress)
    return;

  m_fetchInProgress = true;
  m_folderRequestErrorOccurred = false;

  std::unique_ptr<GetMyUserRequest> getMyUserRequest(
    new GetMyUserRequest(m_networkManager, m_apiUrl, m_girderToken));

  sendAndConnect(getMyUserRequest.get(),
    &GetMyUserRequest::myUser,
    this,
    [this](const QMap<QString, QString>& myUserInfo) {
      this->m_currentParentInfo["name"] = myUserInfo.value("login");
      this->m_currentParentInfo["id"] = myUserInfo.value("id");
      this->m_currentParentInfo["type"] = "user";
      this->getContainingFolders();
      this->getContainingItems();
      this->getRootPath();
    });

  m_girderRequests[GET_MY_USER_REQUEST] = std::move(getMyUserRequest);
}

bool GirderFileBrowserFetcher::folderRequestPending() const
{
  const QList<bool>& list = m_folderRequestPending.values();
  return std::any_of(list.cbegin(), list.cend(), [](bool b) { return b; });
}

void GirderFileBrowserFetcher::getRootFolderInformation()
{
  // These are the names of the two folders in the root directory
  QStringList names = { "Collections", "Users" };
  QList<QMap<QString, QString> > folders;
  for (const auto& name : names)
  {
    QMap<QString, QString> folderInfo;
    folderInfo["type"] = name;
    folderInfo["id"] = "";
    folderInfo["name"] = name;
    folders.append(folderInfo);
  }

  // We have no files for the first folder level
  QList<QMap<QString, QString> > files;

  // Root info is empty as well
  QList<QMap<QString, QString> > rootPath;

  emit folderInformation(m_currentParentInfo, folders, files, rootPath);
}

void GirderFileBrowserFetcher::getUsersFolderInformation()
{
  std::unique_ptr<GetUsersRequest> getUsersRequest(
    new GetUsersRequest(m_networkManager, m_apiUrl, m_girderToken));

  sendAndConnect(getUsersRequest.get(),
    &GetUsersRequest::users,
    this,
    [this](const QMap<QString, QString>& usersMap) {
      this->finishGettingSecondLevelFolderInformation("user", usersMap);
    });

  m_girderRequests[GET_USERS_REQUEST] = std::move(getUsersRequest);
}

void GirderFileBrowserFetcher::getCollectionsFolderInformation()
{
  std::unique_ptr<GetCollectionsRequest> getCollectionsRequest(
    new GetCollectionsRequest(m_networkManager, m_apiUrl, m_girderToken));

  sendAndConnect(getCollectionsRequest.get(),
    &GetCollectionsRequest::collections,
    this,
    [this](const QMap<QString, QString>& collectionsMap) {
      this->finishGettingSecondLevelFolderInformation("collection", collectionsMap);
    });

  m_girderRequests[GET_COLLECTIONS_REQUEST] = std::move(getCollectionsRequest);
}

// Type is probably either "user" or "collection"
void GirderFileBrowserFetcher::finishGettingSecondLevelFolderInformation(const QString& type,
  const QMap<QString, QString>& map)
{
  int numItems = map.keys().size();

  // Let's sort these by name. QMap sorts by key.
  QMap<QString, QString> sortedByName;
  for (const auto& id : map.keys())
    sortedByName[map.value(id)] = id;

  QList<QMap<QString, QString> > folders;
  for (const auto& name : sortedByName.keys())
  {
    QMap<QString, QString> folderInfo;
    folderInfo["type"] = type;
    folderInfo["id"] = sortedByName[name];
    folderInfo["name"] = name;
    folders.append(folderInfo);
  }

  // We have no files for the second directory level
  QList<QMap<QString, QString> > files;


// We currently do not put in any root path information here
//  QMap<QString, QString> rootInfo;
//  rootInfo["name"] = "root";
//  rootInfo["id"] = "";
//  rootInfo["type"] = "root";
//
//  QList<QMap<QString, QString> > rootPath{ rootInfo };

  QList<QMap<QString, QString> > rootPath;

  emit folderInformation(m_currentParentInfo, folders, files, rootPath);
}

void GirderFileBrowserFetcher::finishGettingFolderInformation()
{
  // Let's sort these by name. QMap sorts by key.
  QMap<QString, QString> sortedByName;
  for (const auto& id : m_currentFolders.keys())
    sortedByName[m_currentFolders.value(id)] = id;

  QList<QMap<QString, QString> > folders;
  for (const auto& name : sortedByName.keys())
  {
    QString id = sortedByName.value(name);

    QMap<QString, QString> folderInfo;
    folderInfo["type"] = "folder";
    folderInfo["id"] = id;
    folderInfo["name"] = name;
    folders.append(folderInfo);
  }

  sortedByName.clear();
  // Let's sort these by name. QMap sorts by key.
  for (const auto& id : m_currentItems.keys())
    sortedByName[m_currentItems.value(id)] = id;

  QList<QMap<QString, QString> > files;
  for (const auto& name : sortedByName.keys())
  {
    QString id = sortedByName.value(name);

    QMap<QString, QString> fileInfo;
    fileInfo["type"] = "item";
    fileInfo["id"] = id;
    fileInfo["name"] = name;
    files.append(fileInfo);
  }

  emit folderInformation(m_currentParentInfo, folders, files, m_currentRootPath);
}

void GirderFileBrowserFetcher::getContainingFolders()
{
  m_currentFolders.clear();

  std::unique_ptr<ListFoldersRequest> getFoldersRequest(new ListFoldersRequest(
    m_networkManager, m_apiUrl, m_girderToken, currentParentId(), currentParentType()));

  sendAndConnect(getFoldersRequest.get(),
    &ListFoldersRequest::folders,
    this,
    [this](const QMap<QString, QString>& folders) {
      this->m_currentFolders = folders;
      this->m_folderRequestPending["folders"] = false;
      this->finishGettingFolderInformationIfReady();
    });

  m_girderRequests[GET_FOLDERS_REQUEST] = std::move(getFoldersRequest);
  m_folderRequestPending["folders"] = true;
}

void GirderFileBrowserFetcher::getContainingItems()
{
  m_currentItems.clear();

  // Parent type must be folder, or there are no items
  if (currentParentType() != "folder")
    return;

  std::unique_ptr<ListItemsRequest> getItemsRequest(
    new ListItemsRequest(m_networkManager, m_apiUrl, m_girderToken, currentParentId()));

  sendAndConnect(getItemsRequest.get(),
    &ListItemsRequest::items,
    this,
    [this](const QMap<QString, QString>& items) {
      this->m_currentItems = items;
      this->m_folderRequestPending["items"] = false;
      this->finishGettingFolderInformationIfReady();
    });

  m_girderRequests[GET_ITEMS_REQUEST] = std::move(getItemsRequest);
  m_folderRequestPending["items"] = true;
}

void GirderFileBrowserFetcher::getRootPath()
{
  m_currentRootPath.clear();

  // Parent type must be folder, or this cannot be called.
  if (currentParentType() != "folder")
    return;

  std::unique_ptr<GetFolderRootPathRequest> getRootPathRequest(
    new GetFolderRootPathRequest(m_networkManager, m_apiUrl, m_girderToken, currentParentId()));

  sendAndConnect(getRootPathRequest.get(),
    &GetFolderRootPathRequest::rootPath,
    this,
    [this](const QList<QMap<QString, QString> >& rootPath) {
      this->m_currentRootPath = rootPath;
      this->m_folderRequestPending["rootPath"] = false;
      this->finishGettingFolderInformationIfReady();
    });

  m_girderRequests[GET_ROOT_PATH_REQUEST] = std::move(getRootPathRequest);
  m_folderRequestPending["rootPath"] = true;
}

void GirderFileBrowserFetcher::errorReceived(const QString& message)
{
  QObject* sender = QObject::sender();
  QString completeMessage;
  if (sender == m_girderRequests[GET_FOLDERS_REQUEST].get())
  {
    m_folderRequestErrorOccurred = true;
    completeMessage += "An error occurred while getting folders:\n";
    m_folderRequestPending["folders"] = false;
  }
  else if (sender == m_girderRequests[GET_ITEMS_REQUEST].get())
  {
    m_folderRequestErrorOccurred = true;
    completeMessage += "An error occurred while updating items:\n";
    m_folderRequestPending["items"] = false;
  }
  else if (sender == m_girderRequests[GET_ROOT_PATH_REQUEST].get())
  {
    m_folderRequestErrorOccurred = true;
    completeMessage += "An error occurred while updating the root path:\n";
    m_folderRequestPending["rootPath"] = false;
  }
  else if (sender == m_girderRequests[GET_USERS_REQUEST].get())
  {
    completeMessage += "An error occurred while getting users:\n";
  }
  else if (sender == m_girderRequests[GET_COLLECTIONS_REQUEST].get())
  {
    completeMessage += "An error occurred while getting collections:\n";
  }
  else if (sender == m_girderRequests[GET_MY_USER_REQUEST].get())
  {
    completeMessage += "Failed to get information about current user:\n";
  }
  completeMessage += message;
  emit error(message);
}

} // end namespace
