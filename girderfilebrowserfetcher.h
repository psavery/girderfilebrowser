//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
// .NAME girderfilebrowserdialog.h
// .SECTION Description
// .SECTION See Also

#ifndef __smtk_extension_cumulus_girderfilebrowserfetcher_h
#define __smtk_extension_cumulus_girderfilebrowserfetcher_h

#include "Exports.h"

#include <QMap>
#include <QObject>
#include <QString>

#include <map>
#include <memory>

class QNetworkAccessManager;

namespace cumulus
{

class GirderRequest;

class SMTKCUMULUSEXT_EXPORT GirderFileBrowserFetcher : public QObject
{
  Q_OBJECT

public:
  explicit GirderFileBrowserFetcher(QNetworkAccessManager* networkAccessManager,
    QObject* parent = nullptr);

  virtual ~GirderFileBrowserFetcher() override;

  void setApiUrl(const QString& url) { m_apiUrl = url; }
  void setGirderToken(const QString& token) { m_girderToken = token; }

signals:
  // Emitted when getFolderInformation() is complete
  void folderInformation(const QMap<QString, QString>& parentInfo,
    const QList<QMap<QString, QString> >& folders,
    const QList<QMap<QString, QString> >& files,
    const QList<QMap<QString, QString> >& rootPath);

  // Emitted when there is an error
  void error(const QString& message);

public slots:
  // Emits folderInformation() when it is completed.
  // This map should contain "name", "id", and "type" entries.
  void getFolderInformation(const QMap<QString, QString>& parentInfo);

  // Get the information about the home folder
  void getHomeFolderInformation();

  // Convenience function for signals
  void setApiUrlAndGirderToken(const QString& apiUrl, const QString& girderToken);

private slots:
  void errorReceived(const QString& message);

private:
  // The generic cases
  void getContainingFolders();
  void getContainingItems();
  void getRootPath();

  // Checks to see if all steps are complete and there are no errors
  void finishGettingFolderInformationIfReady();
  void finishGettingFolderInformation();

  // The special cases in the top two level directories
  void getRootFolderInformation();
  void getUsersFolderInformation();
  void getCollectionsFolderInformation();

  // A function to prepend /root, /root/Users, or /root/Collections to the root path
  void prependNeededRootPathItems();

  // A general update function called by getUsersFolderInformation() and
  // getCollectionsFolderInformation()
  void finishGettingSecondLevelFolderInformation(const QString& type,
    const QMap<QString, QString>& map);

  // Convenience functions...
  QString currentParentName() const { return m_currentParentInfo.value("name"); }
  QString currentParentId() const { return m_currentParentInfo.value("id"); }
  QString currentParentType() const { return m_currentParentInfo.value("type"); }

  // Are any folder requests pending?
  bool folderRequestPending() const;

  // Did any folder request errors occur?
  bool folderRequestErrors() const { return m_folderRequestErrorOccurred; }

  // Members
  QNetworkAccessManager* m_networkManager;

  QString m_apiUrl;
  QString m_girderToken;

  // These maps are < id => name >
  QMap<QString, QString> m_currentFolders;
  QMap<QString, QString> m_currentItems;
  // Each of these QMaps represents a girder object. These maps
  // should all have 3 keys: "name", "id", and "type"
  QList<QMap<QString, QString> > m_currentRootPath;

  // Information about the current parent
  QMap<QString, QString> m_currentParentInfo;

  // Our requests.
  // These will be deleted automatically when a new request is made.
  // We must use a std::map here because QMap has errors with unique_ptr
  // as value.
  std::map<QString, std::unique_ptr<GirderRequest> > m_girderRequests;

  // Are there any updates pending?
  QMap<QString, bool> m_folderRequestPending;
  // Did any update errors occur?
  bool m_folderRequestErrorOccurred;

  // Are we currently performing a fetch? If so, we can't perform another one...
  bool m_fetchInProgress;
};

inline void GirderFileBrowserFetcher::setApiUrlAndGirderToken(const QString& url,
  const QString& token)
{
  setApiUrl(url);
  setGirderToken(token);
}

inline void GirderFileBrowserFetcher::finishGettingFolderInformationIfReady()
{
  if (!folderRequestPending() && !folderRequestErrors())
    finishGettingFolderInformation();
}

} // end of namespace

#endif
