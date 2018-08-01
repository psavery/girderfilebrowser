//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include <QByteArray>
#include <QDebug>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>

#include "girderauthenticator.h"

namespace cumulus
{

GirderAuthenticator::GirderAuthenticator(const QString& girderUrl,
  QNetworkAccessManager* networkManager)
  : m_networkManager(networkManager)
  , m_girderUrl(girderUrl)
{
}

GirderAuthenticator::~GirderAuthenticator() = default;

void GirderAuthenticator::authenticateApiKey(const QString& apiKey)
{
  // Only submit one request at a time.
  if (m_pendingReply)
    return;

  static const QString& tokenDuration = "90";

  QByteArray postData;
  postData.append(("key=" + apiKey + "&").toUtf8());
  postData.append(("duration=" + tokenDuration).toUtf8());

  QUrl url(QString("%1/api_key/token").arg(m_girderUrl));

  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  m_pendingReply.reset(m_networkManager->post(request, postData));

  connect(m_pendingReply.get(), &QNetworkReply::finished, this, &GirderAuthenticator::finishAuthenticatingApiKey);
}

QString GirderAuthenticator::getTokenFromReply(QNetworkReply* reply,
                                               QString& errorMessage)
{
  errorMessage.clear();
  QString girderToken;

  QByteArray bytes = reply->readAll();
  if (reply->error())
  {
    errorMessage += "Error: api key authentication failed!\n";
    errorMessage += "Response from server was: " + bytes + "\n";
    return "";
  }
  else
  {
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie> >(v);
    foreach (QNetworkCookie cookie, cookies)
    {
      if (cookie.name() == "girderToken")
      {
        girderToken = cookie.value();
      }
    }

    if (girderToken.isEmpty())
    {
      errorMessage += "Error: Girder response did not set girderToken!\n";
      return "";
    }
  }

  return girderToken;
}

void GirderAuthenticator::finishAuthenticatingApiKey()
{
  QString errorMessage;
  QString girderToken = getTokenFromReply(m_pendingReply.get(), errorMessage);
  emit deleteReplyLater();
  if (girderToken.isEmpty() || !errorMessage.isEmpty()) {
    errorMessage += "Api key authentication failed!\n";
    emit authenticationErrored(errorMessage);
    return;
  }

  emit authenticationSucceeded(girderToken);
}

void GirderAuthenticator::deleteReplyLater()
{
  m_pendingReply.reset();
}

} // end namespace
