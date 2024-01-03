//
// Copyright 2022 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "sceneIndexObserverLoggingTreeView.h"

#include <QHeaderView>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

HduiSceneIndexObserverLoggingTreeView::HduiSceneIndexObserverLoggingTreeView(
    QWidget *parent)
: QTreeView(parent)
{
    setMinimumWidth(512);
    setUniformRowHeights(true);
    setModel(&_model);
    header()->resizeSection(0, 384);
}

void
HduiSceneIndexObserverLoggingTreeView::SetSceneIndex(
    HdSceneIndexBaseRefPtr inputSceneIndex)
{
    //TODO
    if (_currentSceneIndex) {
        _currentSceneIndex->RemoveObserver(HdSceneIndexObserverPtr(&_model));
    }

    _currentSceneIndex = inputSceneIndex;

    if (_currentSceneIndex) {
        _currentSceneIndex->AddObserver(HdSceneIndexObserverPtr(&_model));
    }
}

void
HduiSceneIndexObserverLoggingTreeView::StartRecording()
{
    if (_model.IsRecording()) {
        return;
    }
    _model.StartRecording();
    Q_EMIT RecordingStarted();
}

void
HduiSceneIndexObserverLoggingTreeView::StopRecording()
{
    if (!_model.IsRecording()) {
        return;
    }
    _model.StopRecording();
    Q_EMIT RecordingStopped();
}

void
HduiSceneIndexObserverLoggingTreeView::Clear()
{
    _model.Clear();
}

//-----------------------------------------------------------------------------

void
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::StartRecording()
{
    _isRecording = true;
}

void
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::StopRecording()
{
    _isRecording = false;
}

bool
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::IsRecording()
{
    return _isRecording;
}

void HduiSceneIndexObserverLoggingTreeView::_ObserverModel::Clear()
{
    beginResetModel();
    _notices.clear();
    endResetModel();
}

//-----------------------------------------------------------------------------

void
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    if (!_isRecording) {
        return;
    }

    std::unique_ptr<_AddedPrimsNoticeModel> notice(new _AddedPrimsNoticeModel);
    notice->_entries = entries;
    notice->_index = _notices.size();

    beginInsertRows(QModelIndex(), _notices.size(), _notices.size());
    _notices.push_back(std::move(notice));
    endInsertRows();
}

void
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    if (!_isRecording) {
        return;
    }

    std::unique_ptr<_RemovedPrimsNoticeModel> notice(
        new _RemovedPrimsNoticeModel);
    notice->_entries = entries;
    notice->_index = _notices.size();

    beginInsertRows(QModelIndex(), _notices.size(), _notices.size());
    _notices.push_back(std::move(notice));
    endInsertRows();
}

void
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    if (!_isRecording) {
        return;
    }

    std::unique_ptr<_DirtiedPrimsNoticeModel> notice(
        new _DirtiedPrimsNoticeModel);
    notice->_entries = entries;
    notice->_index = _notices.size();

    beginInsertRows(QModelIndex(), _notices.size(), _notices.size());
    _notices.push_back(std::move(notice));
    endInsertRows();
}

void
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::PrimsRenamed(
    const HdSceneIndexBase &sender,
    const RenamedPrimEntries &entries)
{
    if (!_isRecording) {
        return;
    }

    std::unique_ptr<_RenamedPrimsNoticeModel> notice(
        new _RenamedPrimsNoticeModel);
    notice->_entries = entries;
    notice->_index = _notices.size();

    beginInsertRows(QModelIndex(), _notices.size(), _notices.size());
    _notices.push_back(std::move(notice));
    endInsertRows();
}

//-----------------------------------------------------------------------------

HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_NoticeModelBase *
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_GetModelBase(
    void *ptr) const
{
    if (!ptr) {
        return nullptr;
    }

    return reinterpret_cast<_NoticeModelBase*>(ptr);
}

QVariant
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::data(
    const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (_NoticeModelBase *modelPtr = _GetModelBase(index.internalPointer())) {
        return modelPtr->data(index, role);
    } else {
        if (index.column() == 0) {
            return _notices[index.row()]->noticeTypeString();
        }
    }

    return QVariant();
}

QVariant
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (section == 0) {
            return QVariant("Notice Type/ Prim Path");
        } else if (section == 1) {
            return QVariant("Value");
        }
    }

    return QVariant();
}

QModelIndex
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::index(
        int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        // safeguard against child of child
        if (parent.internalPointer()) {
            return QModelIndex();
        }

        // children of notice values get a pointer to their
        return createIndex(row, column, _notices[parent.row()].get());
    }

    // top-level items don't as they can rely on row to retrieve the right
    // notice. That's how we distinguish between the two.
    if (row >= 0 && row < static_cast<int>(_notices.size())) {
        return createIndex(row, column, nullptr);
    }

    return QModelIndex();
}

QModelIndex
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::parent(
    const QModelIndex &index) const
{
    // top-level items have no pointer so won't have a parent
    if (!index.internalPointer()) {
        return QModelIndex();
    }

    if (_NoticeModelBase *noticePtr = _GetModelBase(index.internalPointer())) {
        return createIndex(noticePtr->_index, index.row(), nullptr);
    }

    return QModelIndex();
}

int
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::columnCount(
    const QModelIndex &parent) const
{
    return 2;
}

int
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::rowCount(
    const QModelIndex &parent) const
{
    if (parent.column() > 0) {
        return 0;
    }

    if (parent.isValid()) {
        if (parent.internalPointer()) {
            return 0;
        } else {
            return _notices[parent.row()]->rowCount();
        }
    }

    return static_cast<int>(_notices.size());
}

//-----------------------------------------------------------------------------

const char *
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_AddedPrimsNoticeModel
::noticeTypeString()
{
    static const char *s = "Added";
    return s;
}

int
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_AddedPrimsNoticeModel
::rowCount()
{
    return static_cast<int>(_entries.size());
}

QVariant
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_AddedPrimsNoticeModel
::data(const QModelIndex &index, int role)
{
    if (index.row() >= static_cast<int>(_entries.size())) {
        return QVariant();
    }

    if (index.column() == 0) {
        return QVariant(
            QString(_entries[index.row()].primPath.GetString().c_str()));
    }

    //TODO, represent type token

    return QVariant();
}

//-----------------------------------------------------------------------------

const char *
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_DirtiedPrimsNoticeModel
::noticeTypeString()
{
    static const char *s = "Dirtied";
    return s;
}

int
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_DirtiedPrimsNoticeModel
::rowCount()
{
    return static_cast<int>(_entries.size());
}

QVariant
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_DirtiedPrimsNoticeModel
::data(const QModelIndex &index, int role)
{
    if (index.row() >= static_cast<int>(_entries.size())) {
        return QVariant();
    }

    if (index.column() == 0) {
        return QVariant(
            QString(_entries[index.row()].primPath.GetString().c_str()));
    } else if (index.column() == 1) {
        std::ostringstream buffer;

        for (const HdDataSourceLocator &locator :
                _entries[index.row()].dirtyLocators) {
            buffer << locator.GetString() << ",";
        }
        
        return QVariant(QString(buffer.str().c_str()));
    }

    return QVariant();
}

//-----------------------------------------------------------------------------

const char *
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_RemovedPrimsNoticeModel
::noticeTypeString()
{
    static const char *s = "Removed";
    return s;
}

int
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_RemovedPrimsNoticeModel
::rowCount()
{
    return static_cast<int>(_entries.size());
}

QVariant
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_RemovedPrimsNoticeModel
::data(const QModelIndex &index, int role)
{
    if (index.row() >= static_cast<int>(_entries.size())) {
        return QVariant();
    }

    if (index.column() == 0) {
        return QVariant(
            QString(_entries[index.row()].primPath.GetString().c_str()));
    }

    return QVariant();
}

//-----------------------------------------------------------------------------

const char *
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_RenamedPrimsNoticeModel
::noticeTypeString()
{
    static const char *s = "Renamed";
    return s;
}

int
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_RenamedPrimsNoticeModel
::rowCount()
{
    return static_cast<int>(_entries.size());
}

QVariant
HduiSceneIndexObserverLoggingTreeView::_ObserverModel::_RenamedPrimsNoticeModel
::data(const QModelIndex &index, int role)
{
    if (index.row() >= static_cast<int>(_entries.size())) {
        return QVariant();
    }

    if (index.column() == 0) {
        return QVariant(
            QString(_entries[index.row()].oldPrimPath.GetString().c_str()));
    }

    if (index.column() == 1) {
        return QVariant(
            QString(_entries[index.row()].newPrimPath.GetString().c_str()));
    }

    return QVariant();
}

PXR_NAMESPACE_CLOSE_SCOPE
