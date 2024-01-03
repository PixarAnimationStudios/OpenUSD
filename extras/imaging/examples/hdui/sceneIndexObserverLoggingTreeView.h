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
#ifndef PXR_IMAGING_HDUI_SCENE_INDEX_OBSERVER_LOGGING_TREE_VIEW_H
#define PXR_IMAGING_HDUI_SCENE_INDEX_OBSERVER_LOGGING_TREE_VIEW_H

#include "pxr/imaging/hd/sceneIndex.h"

#include <QTreeView>

PXR_NAMESPACE_OPEN_SCOPE

class HduiSceneIndexObserverLoggingTreeView : public QTreeView
{
    Q_OBJECT;
public:
    HduiSceneIndexObserverLoggingTreeView(QWidget *parent = Q_NULLPTR);

    void SetSceneIndex(HdSceneIndexBaseRefPtr inputSceneIndex);
    bool IsRecording() { return _model.IsRecording(); }

Q_SIGNALS:
    void RecordingStarted();
    void RecordingStopped();

public Q_SLOTS:
    void StartRecording();
    void StopRecording();
    void Clear();

private:

    class _ObserverModel :
        public HdSceneIndexObserver, public QAbstractItemModel
    {
    public:

        _ObserverModel()
        : _isRecording(false)
        {}

        void StartRecording();
        void StopRecording();
        bool IsRecording();
        void Clear();

        // satisfying HdSceneIndexObserver
        void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override;

        void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override;

        void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override;

        void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override;

        // satisfying QAbstractItemModel
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole)
            const override;

        QVariant headerData(int section, Qt::Orientation orientation,
            int role = Qt::DisplayRole) const override;

        QModelIndex parent(const QModelIndex &index) const override;

        int columnCount(
            const QModelIndex &parent = QModelIndex()) const override;

        int rowCount(
            const QModelIndex &parent = QModelIndex()) const override;

        QModelIndex index(int row, int column,
            const QModelIndex &parent = QModelIndex()) const override;

    private:

        bool _isRecording;

        struct _NoticeModelBase
        {
            virtual ~_NoticeModelBase() = default;
            virtual const char * noticeTypeString() = 0;
            virtual int rowCount() = 0;
            virtual QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) = 0;

            size_t _index;
        };

        struct _AddedPrimsNoticeModel : _NoticeModelBase
        {
            const char * noticeTypeString() override;
            int rowCount() override;
            QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) override;

            AddedPrimEntries _entries;
        };

        struct _DirtiedPrimsNoticeModel : _NoticeModelBase
        {
            const char * noticeTypeString() override;
            int rowCount() override;
            QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) override;
            DirtiedPrimEntries _entries;
        };

        struct _RemovedPrimsNoticeModel : _NoticeModelBase
        {
            const char * noticeTypeString() override;
            int rowCount() override;
            QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) override;

            RemovedPrimEntries _entries;
        };

        struct _RenamedPrimsNoticeModel : _NoticeModelBase
        {
            const char * noticeTypeString() override;
            int rowCount() override;
            QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) override;

            RenamedPrimEntries _entries;
        };

        _NoticeModelBase * _GetModelBase(void *ptr) const;

        std::vector<std::unique_ptr<_NoticeModelBase>> _notices;
    };

    _ObserverModel _model;
    HdSceneIndexBaseRefPtr _currentSceneIndex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif

