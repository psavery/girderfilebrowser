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
  GirderAuthenticator(const QString& girderUrl, QNetworkAccessManager* networkManager, QObject* parent = nullptr);

  virtual ~GirderAuthenticator() override;

  void authenticateApiKey(const QString& apiKey);
  void authenticatePassword(const QString& username, const QString& password);

signals:
  void authenticationSucceeded(const QString& girderToken);
  void authenticationErrored(const QString& errorMessage);

private slots:
  void finishAuthentication();

private:
  QNetworkAccessManager* m_networkManager;
  QString m_girderUrl;
  std::unique_ptr<QNetworkReply> m_pendingReply;
};

} // end namespace

#endif
