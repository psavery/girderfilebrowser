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

#include <QObject>
#include <QString>

#include "Exports.h"

class QNetworkAccessManager;

namespace cumulus
{

class SMTKCUMULUSEXT_EXPORT GirderAuthenticator : public QObject
{
  Q_OBJECT

public:
  GirderAuthenticator(const QString& girderUrl, QNetworkAccessManager* networkManager);

  // Returns a girder token or an empty string on failure
  QString authenticateApiKey(const QString& apiKey);

private:
  QNetworkAccessManager* m_networkManager;
  QString m_girderUrl;
};

} // end namespace

#endif
