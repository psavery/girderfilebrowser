//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
// .NAME girderauthenticator.h
// .SECTION Description
// .SECTION See Also

#ifndef __smtk_extension_cumulus_girderauthenticator_h
#define __smtk_extension_cumulus_girderauthenticator_h

#include <memory>

#include <QObject>
#include <QString>

#include "Exports.h"

class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;

namespace cumulus
{

class SMTKCUMULUSEXT_EXPORT GirderAuthenticator : public QObject
{
  Q_OBJECT

public:
  GirderAuthenticator(const QString& girderUrl, QNetworkAccessManager* networkManager);

  virtual ~GirderAuthenticator() override;

  void authenticateApiKey(const QString& apiKey);

signals:
  void authenticationSucceeded(const QString& girderToken);
  void authenticationErrored(const QString& errorMessage);

private:
  // Read the girder token from a reply
  // If errorMessage is set, an error occurred.
  static QString getTokenFromReply(QNetworkReply* reply, QString& errorMessage);

private slots:
  void finishAuthenticatingApiKey();
  // Deletes the reply by posting a event in the event loop
  void deleteReplyLater();

private:
  QNetworkAccessManager* m_networkManager;
  QString m_girderUrl;
  std::unique_ptr<QNetworkReply> m_pendingReply;
};

} // end namespace

#endif
