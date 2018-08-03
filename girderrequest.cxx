//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include "girderrequest.h"
#include "cJSON.h"
#include "utils.h"

#include <QDebug>
#include <QPair>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkCookieJar>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace cumulus
{

GirderRequest::GirderRequest(QNetworkAccessManager* networkManager, const QString& girderUrl,
  const QString& girderToken, QObject* parent)
  : QObject(parent)
  , m_girderUrl(girderUrl)
  , m_girderToken(girderToken)
  , m_networkManager(networkManager)
{
}

GirderRequest::~GirderRequest()
{
}

ListItemsRequest::ListItemsRequest(QNetworkAccessManager* networkManager, const QString& girderUrl,
  const QString& girderToken, const QString folderId, QObject* parent)
  : GirderRequest(networkManager, girderUrl, girderToken, parent)
  , m_folderId(folderId)
{
}

ListItemsRequest::~ListItemsRequest()
{
}

void ListItemsRequest::send()
{
  QUrlQuery urlQuery;
  urlQuery.addQueryItem("folderId", m_folderId);
  urlQuery.addQueryItem("limit", "0");

  QUrl url(QString("%1/item").arg(m_girderUrl));
  url.setQuery(urlQuery); // reconstructs the query string from the QUrlQuery

  QNetworkRequest request(url);
  request.setRawHeader(QByteArray("Girder-Token"), m_girderToken.toUtf8());

  auto reply = m_networkManager->get(request);
  QObject::connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

void ListItemsRequest::finished()
{
  auto reply = qobject_cast<QNetworkReply*>(this->sender());
  QByteArray bytes = reply->readAll();
  if (reply->error())
  {
    emit error(handleGirderError(reply, bytes), reply);
  }
  else
  {
    cJSON* jsonResponse = cJSON_Parse(bytes.constData());

    if (!jsonResponse || jsonResponse->type != cJSON_Array)
    {
      emit error(QString("Invalid response to listItems."));
      cJSON_Delete(jsonResponse);
      return;
    }

    QMap<QString, QString> itemMap;
    for (cJSON* jsonItem = jsonResponse->child; jsonItem; jsonItem = jsonItem->next)
    {

      cJSON* idItem = cJSON_GetObjectItem(jsonItem, "_id");
      if (!idItem || idItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract item id."));
        break;
      }
      QString id(idItem->valuestring);

      cJSON* nameItem = cJSON_GetObjectItem(jsonItem, "name");
      if (!nameItem || nameItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract item name."));
        break;
      }
      QString name(nameItem->valuestring);

      itemMap[id] = name;
    }

    emit items(itemMap);

    cJSON_Delete(jsonResponse);
  }

  reply->deleteLater();
}

ListFilesRequest::ListFilesRequest(QNetworkAccessManager* networkManager, const QString& girderUrl,
  const QString& girderToken, const QString itemId, QObject* parent)
  : GirderRequest(networkManager, girderUrl, girderToken, parent)
  , m_itemId(itemId)
{
}

ListFilesRequest::~ListFilesRequest()
{
}

void ListFilesRequest::send()
{
  QUrl url = QUrl(QString("%1/item/%2/files").arg(m_girderUrl).arg(m_itemId));
  QUrlQuery urlQuery;
  urlQuery.addQueryItem("limit", "0");
  url.setQuery(urlQuery);

  QNetworkRequest request(url);
  request.setRawHeader(QByteArray("Girder-Token"), m_girderToken.toUtf8());

  auto reply = m_networkManager->get(request);
  QObject::connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

void ListFilesRequest::finished()
{
  auto reply = qobject_cast<QNetworkReply*>(this->sender());
  QByteArray bytes = reply->readAll();
  if (reply->error())
  {
    emit error(handleGirderError(reply, bytes), reply);
  }
  else
  {
    cJSON* jsonResponse = cJSON_Parse(bytes.constData());

    if (!jsonResponse || jsonResponse->type != cJSON_Array)
    {
      emit error(QString("Invalid response to listFiles."));
      cJSON_Delete(jsonResponse);
      return;
    }

    QMap<QString, QString> fileMap;
    for (cJSON* jsonFile = jsonResponse->child; jsonFile; jsonFile = jsonFile->next)
    {

      cJSON* idItem = cJSON_GetObjectItem(jsonFile, "_id");
      if (!idItem || idItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract file id."));
        break;
      }
      QString id(idItem->valuestring);

      cJSON* nameItem = cJSON_GetObjectItem(jsonFile, "name");
      if (!nameItem || nameItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract file id."));
        break;
      }
      QString name(nameItem->valuestring);

      fileMap[id] = name;
    }

    emit files(fileMap);

    cJSON_Delete(jsonResponse);
  }
  reply->deleteLater();
}

ListFoldersRequest::ListFoldersRequest(QNetworkAccessManager* networkManager,
  const QString& girderUrl, const QString& girderToken, const QString parentId, const QString parentType, QObject* parent)
  : GirderRequest(networkManager, girderUrl, girderToken, parent)
  , m_parentId(parentId)
  , m_parentType(parentType)
{
}

ListFoldersRequest::~ListFoldersRequest()
{
}

void ListFoldersRequest::send()
{
  QUrlQuery urlQuery;
  urlQuery.addQueryItem("parentId", m_parentId);
  urlQuery.addQueryItem("parentType", m_parentType);
  urlQuery.addQueryItem("limit", "0");

  QUrl url(QString("%1/folder").arg(m_girderUrl));
  url.setQuery(urlQuery); // reconstructs the query string from the QUrlQuery

  QNetworkRequest request(url);
  request.setRawHeader(QByteArray("Girder-Token"), m_girderToken.toUtf8());

  auto reply = m_networkManager->get(request);
  QObject::connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

void ListFoldersRequest::finished()
{
  auto reply = qobject_cast<QNetworkReply*>(this->sender());
  QByteArray bytes = reply->readAll();
  if (reply->error())
  {
    emit error(handleGirderError(reply, bytes), reply);
  }
  else
  {
    cJSON* jsonResponse = cJSON_Parse(bytes.constData());

    if (!jsonResponse || jsonResponse->type != cJSON_Array)
    {
      emit error(QString("Invalid response to listFolders."));
      cJSON_Delete(jsonResponse);
      return;
    }

    QMap<QString, QString> folderMap;
    for (cJSON* jsonFolder = jsonResponse->child; jsonFolder; jsonFolder = jsonFolder->next)
    {

      cJSON* idItem = cJSON_GetObjectItem(jsonFolder, "_id");
      if (!idItem || idItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract file id."));
        break;
      }
      QString id(idItem->valuestring);

      cJSON* nameItem = cJSON_GetObjectItem(jsonFolder, "name");
      if (!nameItem || nameItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract file id."));
        break;
      }
      QString name(nameItem->valuestring);

      folderMap[id] = name;
    }

    emit folders(folderMap);

    cJSON_Delete(jsonResponse);
  }
  reply->deleteLater();
}

DownloadFolderRequest::DownloadFolderRequest(QNetworkAccessManager* networkManager,
  const QString& girderUrl, const QString& girderToken, const QString& downloadPath,
  const QString& folderId, QObject* parent)
  : GirderRequest(networkManager, girderUrl, girderToken, parent)
  , m_folderId(folderId)
  , m_downloadPath(downloadPath)
  , m_itemsToDownload(NULL)
  , m_foldersToDownload(NULL)
{
  QDir(m_downloadPath).mkpath(".");
}

DownloadFolderRequest::~DownloadFolderRequest()
{
  delete m_itemsToDownload;
  delete m_foldersToDownload;
}

void DownloadFolderRequest::send()
{
  ListItemsRequest* itemsRequest =
    new ListItemsRequest(m_networkManager, m_girderUrl, m_girderToken, m_folderId, this);

  connect(
    itemsRequest, SIGNAL(items(const QList<QString>)), this, SLOT(items(const QList<QString>)));
  connect(itemsRequest, SIGNAL(error(const QString, QNetworkReply*)), this,
    SIGNAL(error(const QString, QNetworkReply*)));

  itemsRequest->send();

  ListFoldersRequest* foldersRequest =
    new ListFoldersRequest(m_networkManager, m_girderUrl, m_girderToken, m_folderId, "folder", this);

  connect(foldersRequest, SIGNAL(folders(const QMap<QString, QString>)), this,
    SLOT(folders(const QMap<QString, QString>)));
  connect(foldersRequest, SIGNAL(error(const QString, QNetworkReply*)), this,
    SIGNAL(error(const QString, QNetworkReply*)));

  foldersRequest->send();
}

void DownloadFolderRequest::items(const QList<QString>& itemIds)
{
  m_itemsToDownload = new QList<QString>(itemIds);

  foreach (QString itemId, itemIds)
  {
    DownloadItemRequest* request = new DownloadItemRequest(
      m_networkManager, m_girderUrl, m_girderToken, m_downloadPath, itemId, this);

    connect(request, SIGNAL(complete()), this, SLOT(downloadItemFinished()));
    connect(request, SIGNAL(error(const QString, QNetworkReply*)), this,
      SIGNAL(error(const QString, QNetworkReply*)));
    connect(request, SIGNAL(info(const QString)), this, SIGNAL(info(const QString)));
    request->send();
  }
}

void DownloadFolderRequest::downloadItemFinished()
{
  DownloadItemRequest* request = qobject_cast<DownloadItemRequest*>(this->sender());

  m_itemsToDownload->removeOne(request->itemId());

  if (this->isComplete())
  {
    emit complete();
  }

  request->deleteLater();
}

void DownloadFolderRequest::folders(const QMap<QString, QString>& folders)
{
  m_foldersToDownload = new QMap<QString, QString>(folders);

  QMapIterator<QString, QString> i(folders);
  while (i.hasNext())
  {
    i.next();
    QString id = i.key();
    QString name = i.value();
    QString path = QDir(m_downloadPath).filePath(name);
    DownloadFolderRequest* request =
      new DownloadFolderRequest(m_networkManager, m_girderUrl, m_girderToken, path, id, this);

    connect(request, SIGNAL(complete()), this, SLOT(downloadFolderFinished()));
    connect(request, SIGNAL(error(const QString, QNetworkReply*)), this,
      SIGNAL(error(const QString, QNetworkReply*)));
    connect(request, SIGNAL(info(const QString)), this, SIGNAL(info(const QString)));

    request->send();
  }
}

void DownloadFolderRequest::downloadFolderFinished()
{
  DownloadFolderRequest* request = qobject_cast<DownloadFolderRequest*>(this->sender());

  m_foldersToDownload->remove(request->folderId());

  if (this->isComplete())
  {
    emit complete();
  }

  request->deleteLater();
}

bool DownloadFolderRequest::isComplete()
{
  return (m_itemsToDownload && m_itemsToDownload->isEmpty() && m_foldersToDownload &&
    m_foldersToDownload->isEmpty());
}

DownloadItemRequest::DownloadItemRequest(QNetworkAccessManager* networkManager,
  const QString& girderUrl, const QString& girderToken, const QString& path, const QString& itemId,
  QObject* parent)
  : GirderRequest(networkManager, girderUrl, girderToken, parent)
  , m_itemId(itemId)
  , m_downloadPath(path)
{
}

DownloadItemRequest::~DownloadItemRequest()
{
}

void DownloadItemRequest::send()
{
  ListFilesRequest* request =
    new ListFilesRequest(m_networkManager, m_girderUrl, m_girderToken, m_itemId, this);

  connect(request, SIGNAL(files(const QMap<QString, QString>)), this,
    SLOT(files(const QMap<QString, QString>)));
  connect(request, SIGNAL(error(const QString, QNetworkReply*)), this,
    SIGNAL(error(const QString, QNetworkReply*)));
  connect(request, SIGNAL(info(const QString)), this, SIGNAL(info(const QString)));

  request->send();
}

void DownloadItemRequest::files(const QMap<QString, QString>& files)
{
  m_filesToDownload = files;

  QMapIterator<QString, QString> i(files);
  while (i.hasNext())
  {
    i.next();
    QString id = i.key();
    QString name = i.value();
    DownloadFileRequest* request = new DownloadFileRequest(
      m_networkManager, m_girderUrl, m_girderToken, m_downloadPath, name, id, this);

    connect(request, SIGNAL(complete()), this, SLOT(fileDownloadFinish()));
    connect(request, SIGNAL(error(const QString, QNetworkReply*)), this,
      SIGNAL(error(const QString, QNetworkReply*)));
    connect(request, SIGNAL(info(const QString)), this, SIGNAL(info(const QString)));

    request->send();
  }
}

void DownloadItemRequest::fileDownloadFinish()
{
  DownloadFileRequest* request = qobject_cast<DownloadFileRequest*>(this->sender());

  m_filesToDownload.remove(request->fileId());

  if (m_filesToDownload.isEmpty())
  {
    emit complete();
    this->deleteLater();
  }

  request->deleteLater();
}

DownloadFileRequest::DownloadFileRequest(QNetworkAccessManager* networkManager,
  const QString& girderUrl, const QString& girderToken, const QString& path,
  const QString& fileName, const QString& fileId, QObject* parent)
  : GirderRequest(networkManager, girderUrl, girderToken, parent)
  , m_fileName(fileName)
  , m_fileId(fileId)
  , m_downloadPath(path)

  , m_retryCount(0)
{
}

DownloadFileRequest::~DownloadFileRequest()
{
}

void DownloadFileRequest::send()
{
  QString girderAuthUrl = QString("%1/file/%2/download").arg(m_girderUrl).arg(m_fileId);

  QNetworkRequest request(girderAuthUrl);
  request.setRawHeader(QByteArray("Girder-Token"), m_girderToken.toUtf8());

  auto reply = m_networkManager->get(request);
  QObject::connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

void DownloadFileRequest::finished()
{
  auto reply = qobject_cast<QNetworkReply*>(this->sender());
  if (reply->error())
  {
    QByteArray bytes = reply->readAll();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).value<int>();

    if (statusCode == 400 && m_retryCount < 5)
    {
      this->send();
      m_retryCount++;
    }
    else
    {
      emit error(handleGirderError(reply, bytes), reply);
    }
  }
  else
  {
    // We need todo the redirect ourselves!
    QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (!redirectUrl.isEmpty())
    {
      QNetworkRequest request;
      request.setUrl(redirectUrl);
      auto reply = m_networkManager->get(request);
      QObject::connect(reply, SIGNAL(finished()), this, SLOT(finished()));
      return;
    }

    emit info(QString("Downloading %1 ...").arg(this->fileName()));
    QDir downloadDir(m_downloadPath);
    QFile file(downloadDir.filePath(this->fileName()));
    file.open(QIODevice::WriteOnly);

    qint64 count = 0;
    char bytes[1024];

    while ((count = reply->read(bytes, sizeof(bytes))) > 0)
    {
      file.write(bytes, count);
    }

    file.close();
  }
  reply->deleteLater();
  emit complete();
}

GetFolderParentRequest::GetFolderParentRequest(QNetworkAccessManager* networkManager,
  const QString& girderUrl, const QString& girderToken, const QString& folderId, QObject* parent)
  : GirderRequest(networkManager, girderUrl, girderToken, parent)
  , m_folderId(folderId)
{
}

GetFolderParentRequest::~GetFolderParentRequest()
{
}

void GetFolderParentRequest::send()
{
  QUrl url(QString("%1/folder/%2").arg(m_girderUrl).arg(m_folderId));

  QNetworkRequest request(url);
  request.setRawHeader(QByteArray("Girder-Token"), m_girderToken.toUtf8());

  auto reply = m_networkManager->get(request);
  QObject::connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

void GetFolderParentRequest::finished()
{
  auto reply = qobject_cast<QNetworkReply*>(this->sender());
  QByteArray bytes = reply->readAll();
  if (reply->error())
  {
    emit error(handleGirderError(reply, bytes), reply);
  }
  else
  {
    cJSON* jsonResponse = cJSON_Parse(bytes.constData());

    // There should only be one response
    if (!jsonResponse || jsonResponse->type != cJSON_Object)
    {
      emit error(QString("Invalid response to getFolderParentRequest."));
      reply->deleteLater();
      cJSON_Delete(jsonResponse);
      return;
    }

    QMap<QString, QString> parentInfo;
    cJSON* nameItem = cJSON_GetObjectItem(jsonResponse, "parentCollection");
    if (!nameItem || nameItem->type != cJSON_String)
    {
      emit error(QString("Unable to extract parent collection."));
      reply->deleteLater();
      cJSON_Delete(jsonResponse);
      return;
    }
    parentInfo["type"] = nameItem->valuestring;

    cJSON* idItem = cJSON_GetObjectItem(jsonResponse, "parentId");
    if (!idItem || idItem->type != cJSON_String)
    {
      emit error(QString("Unable to extract parent id."));
      reply->deleteLater();
      cJSON_Delete(jsonResponse);
      return;
    }
    parentInfo["id"] = idItem->valuestring;

    emit parent(parentInfo);

    cJSON_Delete(jsonResponse);
  }
  reply->deleteLater();
}

GetRootPathRequest::GetRootPathRequest(QNetworkAccessManager* networkManager,
  const QString& girderUrl, const QString& girderToken, const QString& parentId, const QString& parentType, QObject* parent)
  : GirderRequest(networkManager, girderUrl, girderToken, parent)
  , m_parentId(parentId)
  , m_parentType(parentType)
{
}

GetRootPathRequest::~GetRootPathRequest()
{
}

void GetRootPathRequest::send()
{
  QUrl url(QString("%1/%2/%3/rootpath").arg(m_girderUrl).arg(m_parentType).arg(m_parentId));

  QNetworkRequest request(url);
  request.setRawHeader(QByteArray("Girder-Token"), m_girderToken.toUtf8());

  auto reply = m_networkManager->get(request);
  QObject::connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

void GetRootPathRequest::finished()
{
  auto reply = qobject_cast<QNetworkReply*>(this->sender());
  QByteArray bytes = reply->readAll();
  if (reply->error())
  {
    emit error(handleGirderError(reply, bytes), reply);
  }
  else
  {
    cJSON* jsonResponse = cJSON_Parse(bytes.constData());

    if (!jsonResponse || jsonResponse->type != cJSON_Array)
    {
      emit error(QString("Invalid response to GetRootPathRequest."));
      cJSON_Delete(jsonResponse);
      return;
    }

    QList<QMap<QString, QString>> rootPathList;
    for (cJSON* jsonFolder = jsonResponse->child; jsonFolder; jsonFolder = jsonFolder->next)
    {
      // For some reason, everything is under an "object" key...
      cJSON* objectEntry = cJSON_GetObjectItem(jsonFolder, "object");
      if (!objectEntry || objectEntry->type != cJSON_Object)
      {
        emit error(QString("Object key is missing."));
        break;
      }

      QMap<QString, QString> entryMap;
      cJSON* modelTypeItem = cJSON_GetObjectItem(objectEntry, "_modelType");
      if (!modelTypeItem || modelTypeItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract model type."));
        break;
      }
      entryMap["type"] = modelTypeItem->valuestring;

      cJSON* idItem = cJSON_GetObjectItem(objectEntry, "_id");
      if (!idItem || idItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract id."));
        break;
      }
      entryMap["id"] = idItem->valuestring;

      QString nameField;
      if (entryMap["type"] == "user")
        nameField = "login";
      else
        nameField = "name";

      cJSON* nameItem = cJSON_GetObjectItem(objectEntry, nameField.toStdString().c_str());
      if (!nameItem || nameItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract name."));
        break;
      }
      entryMap["name"] = nameItem->valuestring;

      rootPathList.append(entryMap);
    }

    emit rootPath(rootPathList);

    cJSON_Delete(jsonResponse);
  }
  reply->deleteLater();
}

GetUsersRequest::GetUsersRequest(QNetworkAccessManager* networkManager, const QString& girderUrl,
  const QString& girderToken, QObject* parent)
  : GirderRequest(networkManager, girderUrl, girderToken, parent)
{
}

GetUsersRequest::~GetUsersRequest() = default;

void GetUsersRequest::send()
{
  QUrlQuery urlQuery;
  urlQuery.addQueryItem("limit", "0");

  QUrl url(QString("%1/user").arg(m_girderUrl));
  url.setQuery(urlQuery); // reconstructs the query string from the QUrlQuery

  QNetworkRequest request(url);
  request.setRawHeader(QByteArray("Girder-Token"), m_girderToken.toUtf8());

  auto reply = m_networkManager->get(request);
  QObject::connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

void GetUsersRequest::finished()
{
  auto reply = qobject_cast<QNetworkReply*>(this->sender());
  QByteArray bytes = reply->readAll();
  if (reply->error())
  {
    emit error(handleGirderError(reply, bytes), reply);
  }
  else
  {
    cJSON* jsonResponse = cJSON_Parse(bytes.constData());

    if (!jsonResponse || jsonResponse->type != cJSON_Array)
    {
      emit error(QString("Invalid response to GetUsers."));
      cJSON_Delete(jsonResponse);
      return;
    }

    QMap<QString, QString> usersMap;
    for (cJSON* jsonItem = jsonResponse->child; jsonItem; jsonItem = jsonItem->next)
    {

      cJSON* idItem = cJSON_GetObjectItem(jsonItem, "_id");
      if (!idItem || idItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract user id."));
        break;
      }
      QString id(idItem->valuestring);

      cJSON* loginItem = cJSON_GetObjectItem(jsonItem, "login");
      if (!loginItem || loginItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract user login."));
        break;
      }
      QString login(loginItem->valuestring);

      usersMap[id] = login;
    }

    emit users(usersMap);

    cJSON_Delete(jsonResponse);
  }

  reply->deleteLater();
}

GetCollectionsRequest::GetCollectionsRequest(QNetworkAccessManager* networkManager, const QString& girderUrl,
  const QString& girderToken, QObject* parent)
  : GirderRequest(networkManager, girderUrl, girderToken, parent)
{
}

GetCollectionsRequest::~GetCollectionsRequest() = default;

void GetCollectionsRequest::send()
{
  QUrlQuery urlQuery;
  urlQuery.addQueryItem("limit", "0");

  QUrl url(QString("%1/collection").arg(m_girderUrl));
  url.setQuery(urlQuery); // reconstructs the query string from the QUrlQuery

  QNetworkRequest request(url);
  request.setRawHeader(QByteArray("Girder-Token"), m_girderToken.toUtf8());

  auto reply = m_networkManager->get(request);
  QObject::connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

void GetCollectionsRequest::finished()
{
  auto reply = qobject_cast<QNetworkReply*>(this->sender());
  QByteArray bytes = reply->readAll();
  if (reply->error())
  {
    emit error(handleGirderError(reply, bytes), reply);
  }
  else
  {
    cJSON* jsonResponse = cJSON_Parse(bytes.constData());

    if (!jsonResponse || jsonResponse->type != cJSON_Array)
    {
      emit error(QString("Invalid response to GetUsers."));
      cJSON_Delete(jsonResponse);
      return;
    }

    QMap<QString, QString> collectionsMap;
    for (cJSON* jsonItem = jsonResponse->child; jsonItem; jsonItem = jsonItem->next)
    {
      cJSON* idItem = cJSON_GetObjectItem(jsonItem, "_id");
      if (!idItem || idItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract collection id."));
        break;
      }
      QString id(idItem->valuestring);

      cJSON* nameItem = cJSON_GetObjectItem(jsonItem, "name");
      if (!nameItem || nameItem->type != cJSON_String)
      {
        emit error(QString("Unable to extract collection login."));
        break;
      }
      QString name(nameItem->valuestring);

      collectionsMap[id] = name;
    }

    emit collections(collectionsMap);

    cJSON_Delete(jsonResponse);
  }

  reply->deleteLater();
}

GetMyUserRequest::GetMyUserRequest(QNetworkAccessManager* networkManager, const QString& girderUrl,
  const QString& girderToken, QObject* parent)
  : GirderRequest(networkManager, girderUrl, girderToken, parent)
{
}

GetMyUserRequest::~GetMyUserRequest() = default;

void GetMyUserRequest::send()
{
  QUrl url(QString("%1/user/me").arg(m_girderUrl));

  QNetworkRequest request(url);
  request.setRawHeader(QByteArray("Girder-Token"), m_girderToken.toUtf8());

  auto reply = m_networkManager->get(request);
  QObject::connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

void GetMyUserRequest::finished()
{
  auto reply = qobject_cast<QNetworkReply*>(this->sender());
  QByteArray bytes = reply->readAll();
  if (reply->error())
  {
    emit error(handleGirderError(reply, bytes), reply);
  }
  else
  {
    cJSON* jsonResponse = cJSON_Parse(bytes.constData());

    // There should only be one response
    if (!jsonResponse || jsonResponse->type != cJSON_Object)
    {
      emit error(QString("Invalid response to GetMyUserRequest."));
      reply->deleteLater();
      cJSON_Delete(jsonResponse);
      return;
    }

    QMap<QString, QString> myInfo;
    cJSON* loginItem = cJSON_GetObjectItem(jsonResponse, "login");
    if (!loginItem || loginItem->type != cJSON_String)
    {
      emit error(QString("Unable to extract login."));
      reply->deleteLater();
      cJSON_Delete(jsonResponse);
      return;
    }
    myInfo["login"] = loginItem->valuestring;

    cJSON* idItem = cJSON_GetObjectItem(jsonResponse, "_id");
    if (!idItem || idItem->type != cJSON_String)
    {
      emit error(QString("Unable to extract parent id."));
      reply->deleteLater();
      cJSON_Delete(jsonResponse);
      return;
    }
    myInfo["id"] = idItem->valuestring;

    emit myUser(myInfo);

    cJSON_Delete(jsonResponse);
  }
  reply->deleteLater();
}

} // end namespace
