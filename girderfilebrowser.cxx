//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include <cstdlib>
#include <iostream>
#include <memory>

#include <QApplication>
#include <QNetworkAccessManager>
#include <QString>

#include "girderauthenticator.h"
#include "girderfilebrowserdialog.h"
#include "girderlogindialog.h"

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);

  std::unique_ptr<QNetworkAccessManager> networkManager(new QNetworkAccessManager);

  using cumulus::GirderLoginDialog;
  GirderLoginDialog loginDialog;

  using cumulus::GirderAuthenticator;
  GirderAuthenticator girderAuthenticator(networkManager.get());

  using cumulus::GirderFileBrowserDialog;
  GirderFileBrowserDialog gfbDialog(networkManager.get());

  QString apiUrl = std::getenv("GIRDER_API_URL");
  QString apiKey = std::getenv("GIRDER_API_KEY");

  if (!apiUrl.isEmpty())
    loginDialog.setApiUrl(apiUrl);

  // Attempt api key authentication if both environment variables are present
  if (!apiUrl.isEmpty() && !apiKey.isEmpty())
  {
    girderAuthenticator.authenticateApiKey(apiUrl, apiKey);
  }

  // This will be hidden automatically if api key authentication succeeds
  loginDialog.show();

  // Connect the "ok" button of the login dialog to the girder authenticator
  QObject::connect(&loginDialog,
    &GirderLoginDialog::beginAuthentication,
    &girderAuthenticator,
    &GirderAuthenticator::authenticatePassword);
  // If authentication fails, print a message on the login dialog
  QObject::connect(&girderAuthenticator,
    &GirderAuthenticator::authenticationErrored,
    &loginDialog,
    &GirderLoginDialog::authenticationFailed);
  // If authentication fails, also print it to the terminal
  QObject::connect(&girderAuthenticator,
    &GirderAuthenticator::authenticationErrored,
    [](const QString& errorMessage) {
      std::cerr << errorMessage.toStdString() << "\n";
    });

  // If we succeed in authenticating, hide the login dialog, set the
  // girder token, and show the browser window.
  QObject::connect(&girderAuthenticator,
    &GirderAuthenticator::authenticationSucceeded,
    &loginDialog,
    &GirderLoginDialog::hide);
  QObject::connect(&girderAuthenticator,
    &GirderAuthenticator::authenticationSucceeded,
    &gfbDialog,
    &GirderFileBrowserDialog::setApiUrlAndGirderToken);
  QObject::connect(&girderAuthenticator,
    &GirderAuthenticator::authenticationSucceeded,
    &gfbDialog,
    &GirderFileBrowserDialog::show);

  return app.exec();
}
