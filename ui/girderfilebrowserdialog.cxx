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
#include <QRegularExpression>
#include <QStandardItemModel>

namespace cumulus
{

static const QStringList& ALL_OBJECT_TYPES =
  { "root", "Users", "Collections", "user", "collection", "folder", "item", "file" };

GirderFileBrowserDialog::GirderFileBrowserDialog(QNetworkAccessManager* networkManager,
  QWidget* parent)
  : QDialog(parent)
  , m_networkManager(networkManager)
  , m_ui(new Ui::GirderFileBrowserDialog)
  , m_itemModel(new QStandardItemModel(this))
  , m_girderFileBrowserFetcher(new GirderFileBrowserFetcher(m_networkManager))
  , m_choosableTypes(ALL_OBJECT_TYPES)
  , m_rootPathOffset(0)
  , m_folderIcon(new QIcon(":/icons/folder.png"))
  , m_fileIcon(new QIcon(":/icons/file.png"))
{
  m_ui->setupUi(this);

  m_ui->list_fileBrowser->setModel(m_itemModel.get());

  m_ui->layout_rootPath->setAlignment(Qt::AlignLeft);

  // This is if the user presses the "enter" key... Do the same thing as double-click.
  connect(m_ui->list_fileBrowser,
    &QAbstractItemView::activated,
    this,
    &GirderFileBrowserDialog::rowActivated);

  // Connect buttons
  connect(m_ui->push_goUpDir, &QPushButton::pressed, this, &GirderFileBrowserDialog::goUpDirectory);
  connect(m_ui->push_goHome, &QPushButton::pressed, this, &GirderFileBrowserDialog::goHome);
  connect(m_ui->combo_itemMode,
    static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged),
    this,
    &GirderFileBrowserDialog::changeItemMode);
  connect(
    m_ui->push_chooseObject, &QPushButton::pressed, this, &GirderFileBrowserDialog::chooseObject);

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

  // When the user types in the filter box, update the visible rows
  connect(m_ui->edit_matchesExpression,
    &QLineEdit::textEdited,
    this,
    &GirderFileBrowserDialog::changeVisibleRows);

  // Reset the filter text when we change folders
  connect(this, &GirderFileBrowserDialog::changeFolder, m_ui->edit_matchesExpression, [this]() {
    this->m_ui->edit_matchesExpression->setText("");
    this->m_rowsMatchExpression = "";
  });

  // Reset the root path offset when we change folders
  connect(
    this, &GirderFileBrowserDialog::changeFolder, this, [this]() { this->m_rootPathOffset = 0; });

  // We will start in root
  QMap<QString, QString> parentInfo;
  parentInfo["name"] = "root";
  parentInfo["id"] = "";
  parentInfo["type"] = "root";

  emit changeFolder(parentInfo);
}

GirderFileBrowserDialog::~GirderFileBrowserDialog() = default;

// A convenience function for estimating button width
static int buttonWidth(QPushButton* button)
{
  return std::max(button->fontMetrics().width(button->text()), button->sizeHint().width());
}

void GirderFileBrowserDialog::updateRootPathWidget()
{
  QHBoxLayout* layout = m_ui->layout_rootPath;
  QWidget* parentWidget = layout->parentWidget();

  // Cache the old layout width and make sure we don't exceed it.
  int oldLayoutWidth = layout->geometry().width();

  // Clear the current items from the QLayout
  while (QLayoutItem* item = layout->takeAt(0))
  {
    layout->removeWidget(item->widget());
    item->widget()->deleteLater();
  }

  // The scroll left button
  QPushButton* scrollLeft = new QPushButton("<", parentWidget);
  scrollLeft->setAutoDefault(false);
  int scrollLeftWidth = scrollLeft->fontMetrics().width(scrollLeft->text()) * 2;
  scrollLeft->setFixedWidth(scrollLeftWidth);
  layout->addWidget(scrollLeft);
  connect(scrollLeft, &QPushButton::pressed, this, [this]() {
    ++this->m_rootPathOffset;
    this->updateRootPathWidget();
  });

  // The scroll right button
  QPushButton* scrollRight = new QPushButton(">", parentWidget);
  scrollRight->setAutoDefault(false);
  int scrollRightWidth = scrollRight->fontMetrics().width(scrollRight->text()) * 2;
  scrollRight->setFixedWidth(scrollRightWidth);
  scrollRight->setEnabled(m_rootPathOffset != 0);
  layout->addWidget(scrollRight);
  connect(scrollRight, &QPushButton::pressed, this, [this]() {
    --this->m_rootPathOffset;
    this->updateRootPathWidget();
  });

  // Sum up the total button width and make sure we don't exceed it
  int totalWidgetWidth = scrollLeftWidth;
  totalWidgetWidth += scrollRightWidth;

  // Add the widgets in backwards
  int currentOffset = 0;
  bool rootButtonAdded = false;
  if (m_rootPathOffset == 0)
  {
    // This button doesn't do anything... it is only here for consistency
    QPushButton* firstButton = new QPushButton(currentParentName() + "/", parentWidget);
    firstButton->setAutoDefault(false);
    layout->insertWidget(1, firstButton);

    totalWidgetWidth += buttonWidth(firstButton);

    if (currentParentName() == "root")
      rootButtonAdded = true;
  }
  else
  {
    currentOffset += 1;
  }

  for (auto it = m_currentRootPathInfo.rbegin(); it != m_currentRootPathInfo.rend(); ++it)
  {
    if (currentOffset < m_rootPathOffset)
    {
      ++currentOffset;
      continue;
    }

    const auto& rootPathItem = *it;

    auto callFunc = [this, rootPathItem]() { emit this->changeFolder(rootPathItem); };
    QString name = rootPathItem.value("name");

    QPushButton* button = new QPushButton(name + "/", parentWidget);
    button->setAutoDefault(false);
    connect(button, &QPushButton::pressed, this, callFunc);

    int newButtonWidth = buttonWidth(button);

    // We want to make sure at least one button is added
    if (newButtonWidth + totalWidgetWidth > oldLayoutWidth * 0.92 && layout->count() > 2)
    {
      delete button;
      break;
    }

    if (name == "root")
      rootButtonAdded = true;

    layout->insertWidget(1, button);
    totalWidgetWidth += newButtonWidth;
  }

  // Only enable the scroll left if there is room to offset in that direction
  scrollLeft->setEnabled(!rootButtonAdded);
}

void GirderFileBrowserDialog::resizeEvent(QResizeEvent* event)
{
  updateRootPathWidget();
  QWidget::resizeEvent(event);
}

void GirderFileBrowserDialog::rowActivated(const QModelIndex& index)
{
  int row = index.row();
  if (row < m_cachedRowInfo.size())
  {
    QString parentType = m_cachedRowInfo[row].value("type", "unknown");

    QStringList folderTypes{ "root", "Users", "Collections", "user", "collection", "folder" };

    // If we are to treat items as folders, add items to this list
    using ItemMode = GirderFileBrowserFetcher::ItemMode;
    if (m_girderFileBrowserFetcher->itemMode() == ItemMode::treatItemsAsFolders)
      folderTypes.append("item");

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

void GirderFileBrowserDialog::changeItemMode(const QString& itemModeStr)
{
  using ItemMode = GirderFileBrowserFetcher::ItemMode;
  ItemMode itemMode;
  if (itemModeStr == "Treat Items as Files")
  {
    itemMode = ItemMode::treatItemsAsFiles;
  }
  else if (itemModeStr == "Treat Items as Folders")
  {
    itemMode = ItemMode::treatItemsAsFolders;
  }
  else
  {
    qDebug() << "Warning: ignoring unknown item mode:" << itemModeStr;
    return;
  }

  m_girderFileBrowserFetcher->setItemMode(itemMode);

  // Update the current folder since this may change how we interpret the contents
  emit changeFolder(m_currentParentInfo);
}

void GirderFileBrowserDialog::chooseObject()
{
  QModelIndexList list = m_ui->list_fileBrowser->selectionModel()->selectedIndexes();
  if (list.isEmpty())
    return;

  // We can only choose one object right now
  int row = list[0].row();

  QMap<QString, QString> selectedRowInfo = m_cachedRowInfo[row];

  // If this type is not choosable, just ignore it
  if (!m_choosableTypes.contains(selectedRowInfo["type"]))
    return;

  emit objectChosen(selectedRowInfo);
}

void GirderFileBrowserDialog::changeVisibleRows(const QString& expression)
{
  m_rowsMatchExpression = expression;
  updateVisibleRows();
}

void GirderFileBrowserDialog::updateVisibleRows()
{
  // First, make all rows visible
  for (size_t i = 0; i < m_cachedRowInfo.size(); ++i)
    m_ui->list_fileBrowser->setRowHidden(i, false);

  // First, hide any rows that do not match the type the user is choosing
  // We will always show these types, even if they aren't choosable.
  QStringList showTypes = { "Users", "Collections", "user", "collection", "folder" };
  // Add the choosable types
  showTypes += m_choosableTypes;
  for (size_t i = 0; i < m_cachedRowInfo.size(); ++i)
  {
    if (!showTypes.contains(m_cachedRowInfo[i]["type"]))
    {
      m_ui->list_fileBrowser->setRowHidden(i, true);
    }
  }

  // Next, if there is a matching expression, hide all rows whose name does not match the expression
  if (m_rowsMatchExpression.isEmpty())
    return;

  QRegularExpression regExp(
    ".*" + m_rowsMatchExpression + ".*", QRegularExpression::CaseInsensitiveOption);

  for (size_t i = 0; i < m_cachedRowInfo.size(); ++i)
  {
    // If the row is already hidden, skip it
    if (m_ui->list_fileBrowser->isRowHidden(i))
      continue;

    if (!regExp.match(m_cachedRowInfo[i]["name"]).hasMatch())
    {
      m_ui->list_fileBrowser->setRowHidden(i, true);
    }
  }
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

  updateVisibleRows();
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