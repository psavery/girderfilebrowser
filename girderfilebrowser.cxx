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

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);

  QString apiUrl = std::getenv("GIRDER_API_URL");
  QString apiKey = std::getenv("GIRDER_API_KEY");

  if (apiUrl.isEmpty())
  {
    std::cerr << "Error: the GIRDER_API_URL environment variable "
              << "must be set for the api url!\n";
    return EXIT_FAILURE;
  }

  if (apiKey.isEmpty())
  {
    std::cerr << "Error: the GIRDER_API_KEY environment variable "
              << "must be set for the api key!\n";
    return EXIT_FAILURE;
  }

  std::unique_ptr<QNetworkAccessManager> networkManager(new QNetworkAccessManager);

  using cumulus::GirderAuthenticator;
  GirderAuthenticator girderAuthenticator(apiUrl, networkManager.get());
  girderAuthenticator.authenticateApiKey(apiKey);

  using cumulus::GirderFileBrowserDialog;
  GirderFileBrowserDialog gfbDialog(networkManager.get(), apiUrl);

  QObject::connect(&girderAuthenticator,
    &GirderAuthenticator::authenticationSucceeded,
    &gfbDialog,
    &GirderFileBrowserDialog::setGirderToken);
  QObject::connect(&girderAuthenticator,
    &GirderAuthenticator::authenticationSucceeded,
    &gfbDialog,
    &GirderFileBrowserDialog::show);

  QObject::connect(&girderAuthenticator,
    &GirderAuthenticator::authenticationErrored,
    [](const QString& errorMessage) {
      std::cerr << "Girder authentication failed!\n";
      std::cerr << "Error message is:\n\"" << errorMessage.toStdString() << "\"\n";
    });

  return app.exec();
}
