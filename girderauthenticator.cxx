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
#include <QEventLoop>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QString>
#include <QTimer>
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

QString GirderAuthenticator::authenticateApiKey(const QString& apiKey)
{
  QString girderToken;

  static const QString& tokenDuration = "90";

  QByteArray postData;
  postData.append(("key=" + apiKey + "&").toUtf8());
  postData.append(("duration=" + tokenDuration).toUtf8());

  QUrl url(QString("%1/api_key/token").arg(m_girderUrl));

  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  bool timedOut = false;

  QNetworkReply* reply = postAndWaitForReply(request, postData, timedOut);

  if (timedOut)
  {
    qDebug() << "Error: timeout occurred during girder api key authentication!";
    reply->deleteLater();
    return girderToken;
  }

  girderToken = getTokenFromReply(reply);

  reply->deleteLater();

  return girderToken;
}

// Returns true if a reply was received, and false on timeout
QNetworkReply* GirderAuthenticator::postAndWaitForReply(const QNetworkRequest& request,
                                          const QByteArray& postData,
                                          bool& timedOut,
                                          int timeOutMilliseconds)
{
  // Make a timer for timeout
  QTimer timer;

  // Quit the event loop on timeout and indicate that timeout occurred
  connect(&timer, &QTimer::timeout, [&timedOut]() { timedOut = true; });

  // Wait until we get a response or the timeout occurs
  QEventLoop loop;
  QNetworkReply* reply = m_networkManager->post(request, postData);
  connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  timer.start(timeOutMilliseconds);
  loop.exec();

  return reply;
}

QString GirderAuthenticator::getTokenFromReply(QNetworkReply* reply)
{
  QString girderToken;

  QByteArray bytes = reply->readAll();
  if (reply->error())
  {
    qDebug() << "Error: api authentication failed!";
    qDebug() << "Response from server was: " << bytes;
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
      qDebug() << "Error: Girder response did not set girderToken!";
    }
  }

  return girderToken;
}

} // end namespace
