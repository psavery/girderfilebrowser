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
  QWidget* parent)
  : QDialog(parent)
  , m_ui(new Ui::GirderFileBrowserDialog)
  , m_itemModel(new QStandardItemModel(this))
  , m_networkManager(networkManager)
  , m_folderIcon(new QIcon(":/icons/folder.png"))
  , m_fileIcon(new QIcon(":/icons/file.png"))
{
  m_ui->setupUi(this);

  m_ui->list_fileBrowser->setModel(m_itemModel.get());

  m_ui->layout_rootPath->setAlignment(Qt::AlignLeft);

  // Upon item double-clicked...
  connect(m_ui->list_fileBrowser,
    &QAbstractItemView::doubleClicked,
    this,
    &GirderFileBrowserDialog::itemDoubleClicked);

  // This is if the user presses the "enter" key... Do the same thing as double-click.
  connect(m_ui->list_fileBrowser,
    &QAbstractItemView::activated,
    this,
    &GirderFileBrowserDialog::itemDoubleClicked);

  // Connect buttons
  connect(m_ui->push_goUpDir, &QPushButton::pressed, this, &GirderFileBrowserDialog::goUpDirectory);
  connect(m_ui->push_goHome, &QPushButton::pressed, this, &GirderFileBrowserDialog::goHome);

  // These indicate if a browser list update request is currently pending
  m_browserListUpdatesPending["folders"] = false;
  m_browserListUpdatesPending["items"] = false;
  m_browserListUpdatesPending["rootPath"] = false;

  // We will start in root
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
  // If there are browser list updates pending, we will not call this now
  if (browserListUpdatesPending())
    return;

  m_currentParentInfo["name"] = parentName;
  m_currentParentInfo["id"] = parentId;
  m_currentParentInfo["type"] = parentType;
  updateBrowserList();
}

void GirderFileBrowserDialog::updateBrowserList()
{
  m_browserListUpdateErrorOccurred = false;

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

bool GirderFileBrowserDialog::browserListUpdatesPending() const
{
  const QList<bool>& list = m_browserListUpdatesPending.values();
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
      name = "Collections";
    else if (currentRow == 1)
      name = "Users";

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

// A convenience function to do three things:
//   1. Call "send()" on the GirderRequest sender.
//   2. Create connections for finish condition.
//   3. Create connections for error condition.
// Sender must be a GirderRequest object, and Receiver must be
// a GirderFileBrowserDialog object. Slot can be either a
// GirderFileBrowserDialog function or a lambda.
template<typename Sender, typename Signal, typename Receiver, typename Slot>
static void sendAndConnect(Sender* sender, Signal signal, Receiver* receiver, Slot slot)
{
  sender->send();

  QObject::connect(sender, signal, receiver, slot);
  QObject::connect(
    sender, &GirderRequest::error, receiver, &GirderFileBrowserDialog::errorReceived);
}

void GirderFileBrowserDialog::updateBrowserListForUsers()
{
  m_getUsersRequest.reset(new GetUsersRequest(m_networkManager, m_girderUrl, m_girderToken));

  sendAndConnect(m_getUsersRequest.get(),
    &GetUsersRequest::users,
    this,
    [this](const QMap<QString, QString>& usersMap) {
      this->updateSecondDirectoryLevel("user", usersMap);
    });
}

void GirderFileBrowserDialog::updateBrowserListForCollections()
{
  m_getCollectionsRequest.reset(
    new GetCollectionsRequest(m_networkManager, m_girderUrl, m_girderToken));

  sendAndConnect(m_getCollectionsRequest.get(),
    &GetCollectionsRequest::collections,
    this,
    [this](const QMap<QString, QString>& collectionsMap) {
      this->updateSecondDirectoryLevel("collection", collectionsMap);
    });
}

// Type is probably either "user" or "collection"
void GirderFileBrowserDialog::updateSecondDirectoryLevel(const QString& type,
  const QMap<QString, QString>& map)
{
  int numRows = map.keys().size();
  m_itemModel->setRowCount(numRows);
  m_itemModel->setColumnCount(1);

  m_cachedRowInfo.clear();
  m_itemModel->clear();

  int currentRow = 0;

  // Let's sort these by name. QMap sorts by key.
  QMap<QString, QString> sortedByName;
  for (const auto& id : map.keys())
    sortedByName[map.value(id)] = id;

  for (const auto& name : sortedByName.keys())
  {
    QString id = sortedByName[name];
    m_itemModel->setItem(currentRow, 0, new QStandardItem(*m_folderIcon, name));

    QMap<QString, QString> rowInfo;
    rowInfo["type"] = type;
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

  // This will contain the full list of the path including the root
  // and Users/Collections
  QList<QMap<QString, QString> > fullRootPath;

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
    connect(button, &QPushButton::pressed, this, callFunc);

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
  m_getMyUserRequest.reset(new GetMyUserRequest(m_networkManager, m_girderUrl, m_girderToken));

  sendAndConnect(m_getMyUserRequest.get(),
    &GetMyUserRequest::myUser,
    this,
    [this](const QMap<QString, QString>& myUserInfo) {
      QString name = myUserInfo.value("login");
      QString id = myUserInfo.value("id");
      QString type = "user";
      this->updateBrowser(name, id, type);
    });
}

void GirderFileBrowserDialog::updateCurrentFolders()
{
  m_currentFolders.clear();

  m_updateFoldersRequest.reset(new ListFoldersRequest(
    m_networkManager, m_girderUrl, m_girderToken, currentParentId(), currentParentType()));

  sendAndConnect(m_updateFoldersRequest.get(),
    &ListFoldersRequest::folders,
    this,
    [this](const QMap<QString, QString>& newFolders) {
      this->m_currentFolders = newFolders;
      this->m_browserListUpdatesPending["folders"] = false;
      this->updateBrowserListIfReady();
    });

  m_browserListUpdatesPending["folders"] = true;
}

void GirderFileBrowserDialog::updateCurrentItems()
{
  m_currentItems.clear();

  // Parent type must be folder, or there are no items
  if (currentParentType() != "folder")
    return;

  m_updateItemsRequest.reset(
    new ListItemsRequest(m_networkManager, m_girderUrl, m_girderToken, currentParentId()));

  sendAndConnect(m_updateItemsRequest.get(),
    &ListItemsRequest::items,
    this,
    [this](const QMap<QString, QString>& newItems) {
      this->m_currentItems = newItems;
      this->m_browserListUpdatesPending["items"] = false;
      this->updateBrowserListIfReady();
    });

  m_browserListUpdatesPending["items"] = true;
}

void GirderFileBrowserDialog::updateRootPath()
{
  m_currentRootPath.clear();

  // Parent type must be folder, or this cannot be called.
  if (currentParentType() != "folder")
    return;

  m_updateRootPathRequest.reset(
    new GetFolderRootPathRequest(m_networkManager, m_girderUrl, m_girderToken, currentParentId()));

  sendAndConnect(m_updateRootPathRequest.get(),
    &GetFolderRootPathRequest::rootPath,
    this,
    [this](const QList<QMap<QString, QString>>& newRootPath) {
      this->m_currentRootPath = newRootPath;
      this->m_browserListUpdatesPending["rootPath"] = false;
      this->updateBrowserListIfReady();
    });

  m_browserListUpdatesPending["rootPath"] = true;
}

void GirderFileBrowserDialog::errorReceived(const QString& message)
{
  QObject* sender = QObject::sender();
  if (sender == m_updateFoldersRequest.get())
  {
    m_browserListUpdateErrorOccurred = true;
    qDebug() << "An error occurred while updating folders:";
    m_browserListUpdatesPending["folders"] = false;
  }
  else if (sender == m_updateItemsRequest.get())
  {
    m_browserListUpdateErrorOccurred = true;
    qDebug() << "An error occurred while updating items:";
    m_browserListUpdatesPending["items"] = false;
  }
  else if (sender == m_updateRootPathRequest.get())
  {
    m_browserListUpdateErrorOccurred = true;
    qDebug() << "An error occurred while updating the root path:";
    m_browserListUpdatesPending["rootPath"] = false;
  }
  else if (sender == m_getUsersRequest.get())
  {
    qDebug() << "An error occurred while getting users:";
  }
  else if (sender == m_getCollectionsRequest.get())
  {
    qDebug() << "An error occurred while getting collections:";
  }
  else if (sender == m_getMyUserRequest.get())
  {
    qDebug() << "Failed to get information about current user:";
  }
  qDebug() << message;
}

} // end namespace
