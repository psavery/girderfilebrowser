//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include "girderfilebrowserdialog.h"
#include "ui_girderfilebrowserdialog.h"

#include "girderfilebrowserfetcher.h"

#include <QLabel>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QStandardItemModel>

namespace cumulus
{

GirderFileBrowserDialog::GirderFileBrowserDialog(QNetworkAccessManager* networkManager,
  QWidget* parent)
  : QDialog(parent)
  , m_networkManager(networkManager)
  , m_ui(new Ui::GirderFileBrowserDialog)
  , m_itemModel(new QStandardItemModel(this))
  , m_girderFileBrowserFetcher(new GirderFileBrowserFetcher(m_networkManager))
  , m_folderIcon(new QIcon(":/icons/folder.png"))
  , m_fileIcon(new QIcon(":/icons/file.png"))
{
  m_ui->setupUi(this);

  m_ui->list_fileBrowser->setModel(m_itemModel.get());

  m_ui->layout_rootPath->setAlignment(Qt::AlignLeft);

  // Upon item double-clicked...
  connect(m_ui->list_fileBrowser,
    &QAbstractItemView::doubleClicked,
    this,
    &GirderFileBrowserDialog::itemDoubleClicked);

  // This is if the user presses the "enter" key... Do the same thing as double-click.
  connect(m_ui->list_fileBrowser,
    &QAbstractItemView::activated,
    this,
    &GirderFileBrowserDialog::itemDoubleClicked);

  // Connect buttons
  connect(m_ui->push_goUpDir, &QPushButton::pressed, this, &GirderFileBrowserDialog::goUpDirectory);
  connect(m_ui->push_goHome, &QPushButton::pressed, this, &GirderFileBrowserDialog::goHome);

  connect(this,
    &GirderFileBrowserDialog::changeFolder,
    m_girderFileBrowserFetcher.get(),
    &GirderFileBrowserFetcher::getFolderInformation);
  connect(this,
    &GirderFileBrowserDialog::goHome,
    m_girderFileBrowserFetcher.get(),
    &GirderFileBrowserFetcher::getHomeFolderInformation);
  connect(m_girderFileBrowserFetcher.get(),
    &GirderFileBrowserFetcher::folderInformation,
    this,
    &GirderFileBrowserDialog::finishChangingFolder);
  connect(m_girderFileBrowserFetcher.get(),
    &GirderFileBrowserFetcher::error,
    this,
    &GirderFileBrowserDialog::errorReceived);

  // We will start in root
  QMap<QString, QString> parentInfo;
  parentInfo["name"] = "root";
  parentInfo["id"] = "";
  parentInfo["type"] = "root";

  emit changeFolder(parentInfo);
}

GirderFileBrowserDialog::~GirderFileBrowserDialog() = default;

void GirderFileBrowserDialog::updateRootPathWidget()
{
  // Clear the current items from the QLayout
  QHBoxLayout* layout = m_ui->layout_rootPath;
  QWidget* parentWidget = layout->parentWidget();
  while (QLayoutItem* item = layout->takeAt(0))
  {
    layout->removeWidget(item->widget());
    item->widget()->deleteLater();
  }

  int startInd = (m_currentRootPathInfo.size() > 2 ? m_currentRootPathInfo.size() - 2 : 0);

  for (int i = startInd; i < m_currentRootPathInfo.size(); ++i)
  {
    const auto& rootPathItem = m_currentRootPathInfo[i];

    auto callFunc = [this, rootPathItem]() { emit this->changeFolder(rootPathItem); };
    QString name = rootPathItem.value("name");

    QPushButton* button = new QPushButton(name + "/", parentWidget);
    connect(button, &QPushButton::pressed, this, callFunc);

    layout->addWidget(button);
  }

  // This button doesn't do anything... it is only here for consistency
  layout->addWidget(new QPushButton(currentParentName() + "/", parentWidget));

  // Adjust the layout size at the top by hiding earlier path items
  adjustRootPathWidgetSize();
}

void GirderFileBrowserDialog::resizeEvent(QResizeEvent* event)
{
  adjustRootPathWidgetSize();
  QWidget::resizeEvent(event);
}

void GirderFileBrowserDialog::adjustRootPathWidgetSize()
{
  QHBoxLayout* layout = m_ui->layout_rootPath;
  int layoutWidth = layout->geometry().width();
  int layoutWidgetsWidth = 0;
  for (int i = 0; i < layout->count(); ++i)
    layoutWidgetsWidth += layout->itemAt(i)->widget()->width();

  double ratio = static_cast<double>(layoutWidgetsWidth) / layoutWidth;
  //qDebug() << "layoutWidth is " << layoutWidth;
  //qDebug() << "layoutWidgetsWidth is" << layoutWidgetsWidth;
  //qDebug() << "ratio is" << ratio;
}

void GirderFileBrowserDialog::itemDoubleClicked(const QModelIndex& index)
{
  int row = index.row();
  if (row < m_cachedRowInfo.size())
  {
    QString parentType = m_cachedRowInfo[row].value("type", "unknown");

    QStringList folderTypes{ "root", "Users", "Collections", "user", "collection", "folder" };

    if (folderTypes.contains(parentType))
      emit changeFolder(m_cachedRowInfo[row]);
  }
}

void GirderFileBrowserDialog::goUpDirectory()
{
  QString parentName, parentId, parentType;
  if (currentParentType() == "root")
  {
    // Do nothing
    return;
  }

  QMap<QString, QString> newParentInfo;
  newParentInfo["name"] = m_currentRootPathInfo.back().value("name");
  newParentInfo["id"] = m_currentRootPathInfo.back().value("id");
  newParentInfo["type"] = m_currentRootPathInfo.back().value("type");

  emit changeFolder(newParentInfo);
}

void GirderFileBrowserDialog::finishChangingFolder(const QMap<QString, QString>& newParentInfo,
  const QList<QMap<QString, QString> >& folders,
  const QList<QMap<QString, QString> >& files,
  const QList<QMap<QString, QString> >& rootPath)
{
  m_currentParentInfo = newParentInfo;
  m_currentRootPathInfo = rootPath;

  size_t numRows = folders.size() + files.size();
  m_itemModel->setRowCount(numRows);
  m_itemModel->setColumnCount(1);

  m_cachedRowInfo.clear();

  int currentRow = 0;

  // Folders
  for (int i = 0; i < folders.size(); ++i)
  {
    QString name = folders[i].value("name");

    m_itemModel->setItem(currentRow, 0, new QStandardItem(*m_folderIcon, name));
    m_cachedRowInfo.append(folders[i]);
    ++currentRow;
  }

  // Files
  for (int i = 0; i < files.size(); ++i)
  {
    QString name = files[i].value("name");

    m_itemModel->setItem(currentRow, 0, new QStandardItem(*m_fileIcon, name));
    m_cachedRowInfo.append(files[i]);
    ++currentRow;
  }

  updateRootPathWidget();
}

void GirderFileBrowserDialog::errorReceived(const QString& message)
{
  qDebug() << "Error changing folders:\n" << message;
}

void GirderFileBrowserDialog::setApiUrl(const QString& url)
{
  m_girderFileBrowserFetcher->setApiUrl(url);
}

void GirderFileBrowserDialog::setGirderToken(const QString& token)
{
  m_girderFileBrowserFetcher->setGirderToken(token);
}

void GirderFileBrowserDialog::setApiUrlAndGirderToken(const QString& url, const QString& token)
{
  setApiUrl(url);
  setGirderToken(token);
}

} // end namespace
