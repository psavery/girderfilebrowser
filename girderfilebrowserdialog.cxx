//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include "girderfilebrowserdialog.h"
#include "ui_girderfilebrowserdialog.h"

#include "girderrequest.h"

#include <QLabel>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTimer>

namespace cumulus
{

GirderFileBrowserDialog::GirderFileBrowserDialog(QNetworkAccessManager* networkManager,
  const QString& girderUrl,
  const QString& girderToken,
  QWidget* parent)
  : QDialog(parent)
  , m_ui(new Ui::GirderFileBrowserDialog)
  , m_itemModel(new QStandardItemModel(this))
  , m_networkManager(networkManager)
  , m_girderUrl(girderUrl)
  , m_girderToken(girderToken)
  , m_folderIcon(new QIcon(":/icons/folder.png"))
  , m_fileIcon(new QIcon(":/icons/file.png"))
{
  m_ui->setupUi(this);

  m_ui->list_fileBrowser->setModel(m_itemModel.get());
  m_ui->list_fileBrowser->setEditTriggers(QAbstractItemView::NoEditTriggers);

  m_ui->layout_rootPath->setAlignment(Qt::AlignLeft);

  connect(m_ui->list_fileBrowser,
    &QAbstractItemView::doubleClicked,
    this,
    &GirderFileBrowserDialog::itemDoubleClicked);
  // This is if the user presses the "enter" key... Do the same thing as double-click.
  connect(m_ui->list_fileBrowser,
    &QAbstractItemView::activated,
    this,
    &GirderFileBrowserDialog::itemDoubleClicked);
  connect(m_ui->push_goUpDir, &QPushButton::pressed, this, &GirderFileBrowserDialog::goUpDirectory);
  connect(m_ui->push_goHome, &QPushButton::pressed, this, &GirderFileBrowserDialog::goHome);

  m_updatesPending["users"] = false;
  m_updatesPending["collections"] = false;
  m_updatesPending["folders"] = false;
  m_updatesPending["items"] = false;
  m_updatesPending["rootPath"] = false;

  QString currentParentName = "root";
  QString currentParentId = "";
  QString currentParentType = "root";

  updateBrowser(currentParentName, currentParentId, currentParentType);
}

GirderFileBrowserDialog::~GirderFileBrowserDialog() = default;

void GirderFileBrowserDialog::updateBrowser(const QString& parentName,
  const QString& parentId,
  const QString& parentType)
{
  // If there are updates pending, we will not call this now
  if (updatesPending())
    return;

  m_currentParentInfo["name"] = parentName;
  m_currentParentInfo["id"] = parentId;
  m_currentParentInfo["type"] = parentType;
  updateBrowserList();
}

void GirderFileBrowserDialog::updateBrowserList()
{
  m_updateErrorOccurred = false;

  // The first two directory levels will be different from the rest.
  if (currentParentType() == "root")
  {
    updateBrowserListForRoot();
    return;
  }
  else if (currentParentType() == "Users")
  {
    updateBrowserListForUsers();
    return;
  }
  else if (currentParentType() == "Collections")
  {
    updateBrowserListForCollections();
    return;
  }

  // For the standard case
  updateCurrentFolders();
  updateCurrentItems();
  updateRootPath();
}

bool GirderFileBrowserDialog::updatesPending() const
{
  const QList<bool>& list = m_updatesPending.values();
  return std::any_of(list.cbegin(), list.cend(), [](bool b) { return b; });
}

void GirderFileBrowserDialog::updateBrowserListForRoot()
{
  size_t numRows = 2;
  m_itemModel->setRowCount(numRows);
  m_itemModel->setColumnCount(1);

  m_cachedRowInfo.clear();
  m_itemModel->clear();

  for (size_t currentRow = 0; currentRow < numRows; ++currentRow)
  {
    QString name;
    if (currentRow == 0)
      name = "Users";
    else if (currentRow == 1)
      name = "Collections";

    m_itemModel->setItem(currentRow, 0, new QStandardItem(*m_folderIcon, name));
    QMap<QString, QString> rowInfo;
    rowInfo["type"] = name;
    rowInfo["id"] = "";
    rowInfo["name"] = name;
    m_cachedRowInfo.append(rowInfo);
  }

  m_currentRootPath.clear();
  updateRootPathWidget();
}

void GirderFileBrowserDialog::updateBrowserListForUsers()
{
  m_getUsersRequest.reset(new GetUsersRequest(m_networkManager, m_girderUrl, m_girderToken));

  m_getUsersRequest->send();

  connect(m_getUsersRequest.get(),
    &GetUsersRequest::users,
    this,
    &GirderFileBrowserDialog::finishUpdatingBrowserListForUsers);

  connect(m_getUsersRequest.get(),
    &GetUsersRequest::error,
    this,
    &GirderFileBrowserDialog::errorReceived);

  m_updatesPending["users"] = true;
}

void GirderFileBrowserDialog::finishUpdatingBrowserListForUsers(
  const QMap<QString, QString>& usersMap)
{
  m_updatesPending["users"] = false;

  int numRows = usersMap.keys().size();
  m_itemModel->setRowCount(numRows);
  m_itemModel->setColumnCount(1);

  m_cachedRowInfo.clear();
  m_itemModel->clear();

  int currentRow = 0;

  // Let's sort these by name. QMap sorts by key.
  QMap<QString, QString> sortedByName;
  for (const auto& id : usersMap.keys())
    sortedByName[usersMap.value(id)] = id;

  for (const auto& name : sortedByName.keys())
  {
    QString id = sortedByName[name];
    m_itemModel->setItem(currentRow, 0, new QStandardItem(*m_folderIcon, name));

    QMap<QString, QString> rowInfo;
    rowInfo["type"] = "user";
    rowInfo["id"] = id;
    rowInfo["name"] = name;
    m_cachedRowInfo.append(rowInfo);
    ++currentRow;
  }

  m_currentRootPath.clear();
  updateRootPathWidget();
}

void GirderFileBrowserDialog::updateBrowserListForCollections()
{
  m_getCollectionsRequest.reset(
    new GetCollectionsRequest(m_networkManager, m_girderUrl, m_girderToken));

  m_getCollectionsRequest->send();

  connect(m_getCollectionsRequest.get(),
    &GetCollectionsRequest::collections,
    this,
    &GirderFileBrowserDialog::finishUpdatingBrowserListForCollections);

  connect(m_getCollectionsRequest.get(),
    &GetCollectionsRequest::error,
    this,
    &GirderFileBrowserDialog::errorReceived);

  m_updatesPending["collections"] = true;
}

void GirderFileBrowserDialog::finishUpdatingBrowserListForCollections(
  const QMap<QString, QString>& collectionsMap)
{
  m_updatesPending["collections"] = false;

  int numRows = collectionsMap.keys().size();
  m_itemModel->setRowCount(numRows);
  m_itemModel->setColumnCount(1);

  m_cachedRowInfo.clear();
  m_itemModel->clear();

  int currentRow = 0;

  // Let's sort these by name. QMap sorts by key.
  QMap<QString, QString> sortedByName;
  for (const auto& id : collectionsMap.keys())
    sortedByName[collectionsMap.value(id)] = id;

  for (const auto& name : sortedByName.keys())
  {
    QString id = sortedByName[name];
    m_itemModel->setItem(currentRow, 0, new QStandardItem(*m_folderIcon, name));

    QMap<QString, QString> rowInfo;
    rowInfo["type"] = "collection";
    rowInfo["id"] = id;
    rowInfo["name"] = name;
    m_cachedRowInfo.append(rowInfo);
    ++currentRow;
  }

  m_currentRootPath.clear();
  updateRootPathWidget();
}

void GirderFileBrowserDialog::finishUpdatingBrowserList()
{
  int numRows = m_currentFolders.keys().size() + m_currentItems.keys().size();
  m_itemModel->setRowCount(numRows);
  m_itemModel->setColumnCount(1);

  m_cachedRowInfo.clear();
  m_itemModel->clear();

  int currentRow = 0;

  // Let's sort these by name. QMap sorts by key.
  QMap<QString, QString> sortedByName;
  for (const auto& id : m_currentFolders.keys())
    sortedByName[m_currentFolders.value(id)] = id;

  for (const auto& name : sortedByName.keys())
  {
    QString id = sortedByName.value(name);
    m_itemModel->setItem(currentRow, 0, new QStandardItem(*m_folderIcon, name));

    QMap<QString, QString> rowInfo;
    rowInfo["type"] = "folder";
    rowInfo["id"] = id;
    rowInfo["name"] = name;
    m_cachedRowInfo.append(rowInfo);
    ++currentRow;
  }

  sortedByName.clear();
  // Let's sort these by name. QMap sorts by key.
  for (const auto& id : m_currentItems.keys())
    sortedByName[m_currentItems.value(id)] = id;

  for (const auto& name : sortedByName.keys())
  {
    QString id = sortedByName.value(name);
    m_itemModel->setItem(currentRow, 0, new QStandardItem(*m_fileIcon, name));
    QMap<QString, QString> rowInfo;
    rowInfo["type"] = "item";
    rowInfo["id"] = id;
    rowInfo["name"] = name;
    m_cachedRowInfo.append(rowInfo);
    ++currentRow;
  }

  updateRootPathWidget();
}

void GirderFileBrowserDialog::updateRootPathWidget()
{
  // Clear the current items from the QLayout
  QHBoxLayout* layout = m_ui->layout_rootPath;
  QWidget* parentWidget = layout->parentWidget();
  while (QLayoutItem* item = layout->takeAt(0))
  {
    layout->removeWidget(item->widget());
    item->widget()->deleteLater();
  }

  // This will contain the full list of the path including the root and Users/Collections
  QList<QMap<QString, QString>> fullRootPath;

  if (currentParentName() != "root")
  {
    QMap<QString, QString> rootEntry;
    // The root button
    rootEntry["name"] = "root";
    rootEntry["id"] = "";
    rootEntry["type"] = "root";
    fullRootPath.append(rootEntry);
  }

  bool needUsersButton = false;
  bool needCollectionsButton = false;
  if (!m_currentRootPath.isEmpty())
  {
    const auto& topPathItem = m_currentRootPath.front();
    QString topItemType = topPathItem.value("type");
    if (topItemType == "user")
      needUsersButton = true;
    else if (topItemType == "collection")
      needCollectionsButton = true;
  }

  if (currentParentType() == "user")
    needUsersButton = true;
  else if (currentParentType() == "collection")
    needCollectionsButton = true;

  if (needUsersButton)
  {
    QMap<QString, QString> usersEntry;
    // Users button
    usersEntry["name"] = "Users";
    usersEntry["id"] = "";
    usersEntry["type"] = "Users";
    fullRootPath.append(usersEntry);
  }
  else if (needCollectionsButton)
  {
    QMap<QString, QString> collectionsEntry;
    // Collections button
    collectionsEntry["name"] = "Collections";
    collectionsEntry["id"] = "";
    collectionsEntry["type"] = "Collections";
    fullRootPath.append(collectionsEntry);
  }

  // Add the folders
  for (const auto& rootPathItem : m_currentRootPath)
    fullRootPath.append(rootPathItem);

  layout->addWidget(new QLabel("/", parentWidget));
  // Only put the most recent three buttons at the top
  int startInd = (fullRootPath.size() > 1 ? fullRootPath.size() - 2 : 0);
  for (startInd; startInd < fullRootPath.size(); ++startInd)
  {
    const auto& rootPathItem = fullRootPath[startInd];
    QString name = rootPathItem.value("name");
    QString id = rootPathItem.value("id");
    QString type = rootPathItem.value("type");

    auto callFunc = [this, name, id, type]() { this->updateBrowser(name, id, type); };

    QPushButton* button = new QPushButton(name, parentWidget);
    connect(button, &QPushButton::pressed, callFunc);

    layout->addWidget(button);
    layout->addWidget(new QLabel("/", parentWidget));
  }

  // This button doesn't do anything... it is only here for consistency
  layout->addWidget(new QPushButton(currentParentName(), parentWidget));
  layout->addWidget(new QLabel("/", parentWidget));
}

void GirderFileBrowserDialog::itemDoubleClicked(const QModelIndex& index)
{
  int row = index.row();
  if (row < m_cachedRowInfo.size())
  {
    QString parentType = m_cachedRowInfo[row].value("type", "unknown");

    QStringList folderTypes{ "root", "Users", "Collections", "user", "collection", "folder" };

    if (folderTypes.contains(parentType))
    {
      QString parentId = m_cachedRowInfo[row].value("id", "");
      QString parentName = m_cachedRowInfo[row].value("name", "");
      updateBrowser(parentName, parentId, parentType);
    }
  }
}

void GirderFileBrowserDialog::goUpDirectory()
{
  QString parentName, parentId, parentType;
  if (currentParentType() == "root")
  {
    // Do nothing
    return;
  }
  else if (currentParentType() == "Users")
  {
    parentName = "root";
    parentId = "";
    parentType = "root";
  }
  else if (currentParentType() == "Collections")
  {
    parentName = "root";
    parentId = "";
    parentType = "root";
  }
  else if (currentParentType() == "user")
  {
    parentName = "Users";
    parentId = "";
    parentType = "Users";
  }
  else if (currentParentType() == "collection")
  {
    parentName = "Collections";
    parentId = "";
    parentType = "Collections";
  }
  else if (currentParentType() == "folder" && !m_currentRootPath.isEmpty())
  {
    parentName = m_currentRootPath.back().value("name");
    parentId = m_currentRootPath.back().value("id");
    parentType = m_currentRootPath.back().value("type");
  }
  else
  {
    // Do nothing
    return;
  }

  updateBrowser(parentName, parentId, parentType);
}

void GirderFileBrowserDialog::goHome()
{
  m_getMyUserRequest.reset(
    new GetMyUserRequest(m_networkManager, m_girderUrl, m_girderToken));

  m_getMyUserRequest->send();

  connect(m_getMyUserRequest.get(),
    &GetMyUserRequest::myUser,
    this,
    &GirderFileBrowserDialog::finishGoingHome);

  connect(m_getMyUserRequest.get(),
    &GetMyUserRequest::error,
    this,
    &GirderFileBrowserDialog::errorReceived);
}

void GirderFileBrowserDialog::finishGoingHome(const QMap<QString, QString>& myUserInfo)
{
  QString name = myUserInfo.value("login");
  QString id = myUserInfo.value("id");
  QString type = "user";

  updateBrowser(name, id, type);
}

void GirderFileBrowserDialog::updateCurrentFolders()
{
  m_currentFolders.clear();

  m_updateFoldersRequest.reset(new ListFoldersRequest(
    m_networkManager, m_girderUrl, m_girderToken, currentParentId(), currentParentType()));

  m_updateFoldersRequest->send();

  connect(m_updateFoldersRequest.get(),
    &ListFoldersRequest::folders,
    this,
    &GirderFileBrowserDialog::finishUpdatingFolders);

  connect(m_updateFoldersRequest.get(),
    &ListFoldersRequest::error,
    this,
    &GirderFileBrowserDialog::errorReceived);

  m_updatesPending["folders"] = true;
}

void GirderFileBrowserDialog::finishUpdatingFolders(const QMap<QString, QString>& newFolders)
{
  m_currentFolders = newFolders;
  m_updatesPending["folders"] = false;
  if (!updatesPending() && !updateErrors())
    finishUpdatingBrowserList();
}

void GirderFileBrowserDialog::updateCurrentItems()
{
  m_currentItems.clear();

  // Parent type must be folder, or there are no items
  if (currentParentType() != "folder")
    return;

  m_updateItemsRequest.reset(
    new ListItemsRequest(m_networkManager, m_girderUrl, m_girderToken, currentParentId()));

  m_updateItemsRequest->send();

  connect(m_updateItemsRequest.get(),
    &ListItemsRequest::items,
    this,
    &GirderFileBrowserDialog::finishUpdatingItems);

  connect(m_updateItemsRequest.get(),
    &ListItemsRequest::error,
    this,
    &GirderFileBrowserDialog::errorReceived);

  m_updatesPending["items"] = true;
}

void GirderFileBrowserDialog::finishUpdatingItems(const QMap<QString, QString>& newItems)
{
  m_currentItems = newItems;
  m_updatesPending["items"] = false;
  if (!updatesPending() && !updateErrors())
    finishUpdatingBrowserList();
}

void GirderFileBrowserDialog::updateRootPath()
{
  m_currentRootPath.clear();

  // Parent type must be folder, or this cannot be called.
  if (currentParentType() != "folder")
    return;

  m_updateRootPathRequest.reset(
    new GetFolderRootPathRequest(m_networkManager, m_girderUrl, m_girderToken, currentParentId()));

  m_updateRootPathRequest->send();

  connect(m_updateRootPathRequest.get(),
    &GetFolderRootPathRequest::rootPath,
    this,
    &GirderFileBrowserDialog::finishUpdatingRootPath);

  connect(m_updateRootPathRequest.get(),
    &GetFolderRootPathRequest::error,
    this,
    &GirderFileBrowserDialog::errorReceived);

  m_updatesPending["rootPath"] = true;
}

void GirderFileBrowserDialog::finishUpdatingRootPath(
  const QList<QMap<QString, QString> >& newRootPath)
{
  m_currentRootPath = newRootPath;
  m_updatesPending["rootPath"] = false;
  if (!updatesPending() && !updateErrors())
    finishUpdatingBrowserList();
}

void GirderFileBrowserDialog::errorReceived(const QString& message)
{
  m_updateErrorOccurred = true;
  QObject* sender = QObject::sender();
  if (sender == m_updateFoldersRequest.get())
  {
    qDebug() << "An error occurred while updating folders:";
    m_updatesPending["folders"] = false;
  }
  else if (sender == m_updateItemsRequest.get())
  {
    qDebug() << "An error occurred while updating items:";
    m_updatesPending["items"] = false;
  }
  else if (sender == m_updateRootPathRequest.get())
  {
    qDebug() << "An error occurred while updating the root path:";
    m_updatesPending["rootPath"] = false;
  }
  else if (sender == m_getUsersRequest.get())
  {
    qDebug() << "An error occurred while getting users:";
    m_updatesPending["users"] = false;
  }
  else if (sender == m_getCollectionsRequest.get())
  {
    qDebug() << "An error occurred while getting collections:";
    m_updatesPending["collections"] = false;
  }
  else if (sender == m_getMyUserRequest.get())
  {
    qDebug() << "Failed to get information about current user:";
  }
  qDebug() << message;
}

} // end namespace
