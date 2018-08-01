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

  const char* apiUrl = std::getenv("GIRDER_API_URL");
  const char* apiKey = std::getenv("GIRDER_API_KEY");

  if (!apiUrl) {
    std::cerr << "Error: the GIRDER_API_URL environment variable "
              << "must be set for the api url!\n";
    return EXIT_FAILURE;
  }

  if (!apiKey) {
    std::cerr << "Error: the GIRDER_API_KEY environment variable "
              << "must be set for the api key!\n";
    return EXIT_FAILURE;
  }

  std::unique_ptr<QNetworkAccessManager> networkManager(new QNetworkAccessManager);

  cumulus::GirderAuthenticator girderAuthenticator(apiUrl, networkManager.get());
  QString girderToken = girderAuthenticator.authenticateApiKey(apiKey);
  if (girderToken.isEmpty()) {
    std::cerr << "Error: failed to receive a girder token from the server!\n";
    return EXIT_FAILURE;
  }

  cumulus::GirderFileBrowserDialog gfbDialog(networkManager.get(), apiUrl, girderToken);
  gfbDialog.setGirderUrl(apiUrl);
  gfbDialog.show();

  return app.exec();
}
