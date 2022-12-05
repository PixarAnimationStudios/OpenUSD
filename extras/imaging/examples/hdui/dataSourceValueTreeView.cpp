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
#include "dataSourceValueTreeView.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

#include <QAbstractItemModel>
#include <QHeaderView>

#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

//-----------------------------------------------------------------------------

class Hdui_ValueItemModel : public QAbstractItemModel
{
public:

    Hdui_ValueItemModel(VtValue value, QObject *parent = nullptr)
    : QAbstractItemModel(parent)
    , _value(value)
    {
    }

    // base is good for scalars as we'll use VtValue's call through to << on 
    // the internal type
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
            override {

        if (role != Qt::DisplayRole) {
            return QVariant();
        }

        if (index.row() == 0) {
            if (index.column() == 0) {
                std::ostringstream buffer;
                buffer << _value;
                return QVariant(buffer.str().data());
            }
        }

        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation,
            int role = Qt::DisplayRole) const override {

        if (role == Qt::DisplayRole) {
            if (section == 0) {
                return QVariant(_value.GetTypeName().c_str());
            } else if (section == 1) {
                return QVariant("Index");
            }
        }

        return QVariant();
    }

    QModelIndex parent(const QModelIndex &index) const override {
        return QModelIndex();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        return 1;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if (parent.isValid() || parent.column() > 0) {
            return 0;
        }

        if (_value.IsArrayValued()) {
            return static_cast<int>(_value.GetArraySize());
        }
        return 1;
    }

    QModelIndex index(int row, int column,
            const QModelIndex &parent = QModelIndex()) const override {
        return createIndex(row, column);
    }

protected:
    VtValue _value;
};

//-----------------------------------------------------------------------------

template <typename T>
class Hdui_TypedArrayValueItemModel : public Hdui_ValueItemModel
{
public:
    Hdui_TypedArrayValueItemModel(VtValue value, QObject *parent = nullptr)
    : Hdui_ValueItemModel(value, parent)
    {
        if (_value.IsHolding<VtArray<T>>()) {
            _array = _value.UncheckedGet<VtArray<T>>();
        }
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
            override {

        if (role == Qt::TextAlignmentRole && index.column() == 1) {
            return QVariant(Qt::AlignRight);
        }

        if (role == Qt::ForegroundRole && index.column() == 1) {
            return QVariant(QPalette().brush(
                QPalette::Disabled, QPalette::WindowText));
        }

        if (role != Qt::DisplayRole) {
            return QVariant();
        }

        if (index.column() == 1) {
            return QVariant(index.row());
        } else if (index.column() == 0) {
            if (index.row() < static_cast<int>(_array.size())) {
                std::ostringstream buffer;
                buffer << _array.cdata()[index.row()];
                return QVariant(buffer.str().data());
            }
        }

        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation,
            int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole) {
            if (section == 1) {
                std::ostringstream buffer;
                buffer << _array.size() << " values";
                return QVariant(buffer.str().c_str());
            }
        }

        return Hdui_ValueItemModel::headerData(section, orientation, role);
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        return 2;
    }

private:
    VtArray<T> _array;
};

//-----------------------------------------------------------------------------

class Hdui_UnsupportedTypeValueItemModel : public Hdui_ValueItemModel
{
public:
    Hdui_UnsupportedTypeValueItemModel(VtValue value, QObject *parent = nullptr)
    : Hdui_ValueItemModel(value, parent)
    {}

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
            override {

        if (role != Qt::DisplayRole) {
            return QVariant();
        }

        return QVariant("(unsuppored type)");
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if (parent.isValid() || parent.column() > 0) {
            return 0;
        }
        return 1;
    }
};

//-----------------------------------------------------------------------------

Hdui_ValueItemModel *
Hdui_GetModelFromValue(VtValue value, QObject *parent = nullptr)
{
    if (!value.IsArrayValued()) {
        return new Hdui_ValueItemModel(value, parent);
    }

    if (value.IsHolding<VtArray<int>>()) {
        return new Hdui_TypedArrayValueItemModel<int>(value, parent);
    }

    if (value.IsHolding<VtArray<float>>()) {
        return new Hdui_TypedArrayValueItemModel<float>(value, parent);
    }

    if (value.IsHolding<VtArray<double>>()) {
        return new Hdui_TypedArrayValueItemModel<double>(value, parent);
    }

    if (value.IsHolding<VtArray<TfToken>>()) {
        return new Hdui_TypedArrayValueItemModel<TfToken>(value, parent);
    }

    if (value.IsHolding<VtArray<SdfPath>>()) {
        return new Hdui_TypedArrayValueItemModel<SdfPath>(value, parent);
    }

    if (value.IsHolding<VtArray<GfVec3f>>()) {
        return new Hdui_TypedArrayValueItemModel<GfVec3f>(value, parent);
    }

    if (value.IsHolding<VtArray<GfVec3d>>()) {
        return new Hdui_TypedArrayValueItemModel<GfVec3d>(value, parent);
    }

    if (value.IsHolding<VtArray<GfMatrix4d>>()) {
        return new Hdui_TypedArrayValueItemModel<GfMatrix4d>(value, parent);
    }

    if (value.IsHolding<VtArray<GfVec2f>>()) {
        return new Hdui_TypedArrayValueItemModel<GfVec2f>(value, parent);
    }

    return new Hdui_UnsupportedTypeValueItemModel(value, parent);
}

//-----------------------------------------------------------------------------

HduiDataSourceValueTreeView::HduiDataSourceValueTreeView(QWidget *parent)
: QTreeView(parent)
{
    setUniformRowHeights(true);
    setItemsExpandable(false);
}

void
HduiDataSourceValueTreeView::SetDataSource(
    const HdSampledDataSourceHandle &dataSource)
{
    QAbstractItemModel *existingModel = model();

    _dataSource = dataSource;
    if (_dataSource) {
        setModel(Hdui_GetModelFromValue(_dataSource->GetValue(0.0f), this));

        header()->setSectionResizeMode(0, QHeaderView::Stretch);
        if (header()->count() > 1) {
            header()->setSectionResizeMode(1, QHeaderView::Fixed);
            header()->resizeSection(1,fontMetrics().averageCharWidth() * 10);
            header()->setStretchLastSection(false);
        } else {
            header()->setStretchLastSection(true);
        }
    } else {
        setModel(nullptr);
    }

    delete existingModel;
}

void
HduiDataSourceValueTreeView::Refresh()
{
    SetDataSource(_dataSource);
}

//-----------------------------------------------------------------------------

PXR_NAMESPACE_CLOSE_SCOPE