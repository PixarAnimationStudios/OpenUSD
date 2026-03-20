//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "dataSourceValueTreeView.h"

#include "pxr/base/vt/visitValue.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/primOriginSchema.h"
#include "pxr/usd/sdf/path.h"

// Include everything in VT_VALUE_TYPES; see pxr/base/vt/types.h
// This is required for TfStringify() on T elements of VtArray<T>.
#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/dualQuatf.h"
#include "pxr/base/gf/dualQuath.h"
#include "pxr/base/gf/dualQuath.h"
#include "pxr/base/gf/dualQuatd.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/interval.h"
#include "pxr/base/gf/matrix2f.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/quath.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/range1f.h"
#include "pxr/base/gf/range1d.h"
#include "pxr/base/gf/range2f.h"
#include "pxr/base/gf/range2d.h"
#include "pxr/base/gf/range3f.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/rect2i.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"

#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QMenu>
#include <QString>

#include <optional>
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

        if (role == Qt::ToolTipRole) {
            return GetToolTipText(index);
        }
        if (role != Qt::DisplayRole) {
            return QVariant();
        }

        if (index.row() == 0) {
            if (index.column() == 0) {
                std::ostringstream buffer;
                if (_value.IsHolding<SdfPathVector>()) {
                    // Special case for SdfPathVector.
                    //
                    // Arguably, this would be even more useful defined as
                    // SdfPath::operator<<(); however, it is unclear what
                    // formatting would make the most sense there, whereas
                    // in hdui a newline separator is best for readability.
                    SdfPathVector paths = _value.Get<SdfPathVector>();
                    for (SdfPath const& path: paths) {
                        buffer << path << "\n";
                    }
                } else if (_value.IsHolding<HdPrimOriginSchema::OriginPath>()) {
                    // Get the wrapped path.
                    buffer <<
                        _value.UncheckedGet<HdPrimOriginSchema::OriginPath>()
                            .GetPath();

                } else {
                    buffer << _value;
                }
                std::string str = buffer.str();
                return QVariant(QString::fromUtf8(str.data(), str.size()));
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
    QVariant GetToolTipText(const QModelIndex &index) const {
        // XXX Probably better to define a custom view delegate to drive this.
        constexpr int tooltipMinChars = 24;
        QVariant displayValue = this->data(index, Qt::DisplayRole);
        QString str = displayValue.toString();
        if (str.length() <= tooltipMinChars) {
            return QVariant();
        }

        // Show only the last 24 characters, with ... before if longer
        constexpr int tooltipMaxTrailingChars = 24;
        if (str.length() > tooltipMaxTrailingChars) {
            str = "..." + str.right(tooltipMaxTrailingChars);
        }
        return str;
    }

protected:
    const VtValue _value;
};

//-----------------------------------------------------------------------------

template <typename T>
class Hdui_TypedArrayValueItemModel : public Hdui_ValueItemModel
{
public:
    Hdui_TypedArrayValueItemModel(
        VtArray<T> const& array,
        QObject *parent = nullptr)
    : Hdui_ValueItemModel(VtValue(array), parent)
    , _array(array)
    {
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

        if (role == Qt::ToolTipRole) {
            return GetToolTipText(index);
        }

        if (role != Qt::DisplayRole) {
            return QVariant();
        }

        if (index.column() == 1) {
            return QVariant(index.row());
        } else if (index.column() == 0) {
            if (index.row() < static_cast<int>(_array.size())) {
                std::string str = TfStringify( _array.cdata()[index.row()] );
                return QVariant(QString::fromUtf8(str.data(), str.size()));
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
                std::string str = buffer.str();
                return QVariant(QString::fromUtf8(str.data(), str.size()));
            }
        }

        return Hdui_ValueItemModel::headerData(section, orientation, role);
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        return 2;
    }

private:
    const VtArray<T> _array;
};

//-----------------------------------------------------------------------------

struct _VtValueToItemModel
{
    QObject *parent;

    using ResultType = Hdui_ValueItemModel*;

    template <typename T>
    ResultType operator()(VtArray<T> const& array) const {
        return new Hdui_TypedArrayValueItemModel<T>(array, parent);
    }
    ResultType operator()(const VtValue &value) const {
        // VtArray<SdfPath> is not in VT_VALUE_TYPES, so must handle explicitly
        if (value.IsHolding< VtArray<SdfPath> >()) {
            return operator()( value.UncheckedGet<VtArray<SdfPath> >() );
        }
        return new Hdui_ValueItemModel(value, parent);
    }
};

Hdui_ValueItemModel *
Hdui_NewModelFromValue(VtValue value, QObject *parent = nullptr)
{
    return VtVisitValue(value, _VtValueToItemModel{parent});
}

//-----------------------------------------------------------------------------

HduiDataSourceValueTreeView::HduiDataSourceValueTreeView(QWidget *parent)
: QTreeView(parent)
{
    setUniformRowHeights(true);
    setItemsExpandable(false);

    using _OptPath = std::optional<SdfPath>;
    auto getPathValue = [](const QModelIndex &index) -> _OptPath {
        if (!index.isValid()) {
            return std::nullopt;
        }

        QVariant value = index.model()->data(index, Qt::DisplayRole);
        if (value.canConvert<QString>()) {
            const auto str = value.toString().toStdString();
            if (SdfPath::IsValidPathString(str)) {
                return SdfPath(str);
            }
        }
        return std::nullopt;
    };

    
    // Create a jump to prim action that can be triggered via Ctrl+J without
    // opening a context menu. The item needs to be selected for this to work.
    {
        QAction *jumpToPrimAction = new QAction("Jump to Prim", this);
        jumpToPrimAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
        jumpToPrimAction->setShortcutContext(Qt::WidgetShortcut);
        addAction(jumpToPrimAction);
    
        connect(jumpToPrimAction, &QAction::triggered, this,
            [this, getPathValue]() {
                if (auto optPath = getPathValue(currentIndex())) {
                    Q_EMIT JumpToPrim(*optPath);
                }
            });
    }

    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QTreeView::customContextMenuRequested, this,
        [this, getPathValue](const QPoint &pos) {
            QModelIndex index = indexAt(pos);
            if (!index.isValid())
                return;
            QMenu menu(this);

            // "Copy Value" action. Note that Ctrl+C (or Cmd+C on Mac) already
            // copies the selected item's value by default. It may not be
            // obvious to users, so we add an explicit action here.
            //
            QAction *copyAction = menu.addAction("Copy Value");
            connect(copyAction, &QAction::triggered, this, [this, index]() {
                QVariant value = model()->data(index, Qt::DisplayRole);
                QApplication::clipboard()->setText(value.toString());
            });

            // "Jump to Prim" action, if the value at the index is a valid path.
            // Note that we already have a Ctrl+J shortcut for this action
            // (above) so users can trigger it without opening the context menu.
            //
            if (auto optPath = getPathValue(index)) {
                QAction *jumpToPrimAction = menu.addAction("Jump to Prim");
                connect(jumpToPrimAction, &QAction::triggered, this,
                    [this, optPath]() {
                        Q_EMIT JumpToPrim(*optPath);
                    });
            }

            menu.exec(viewport()->mapToGlobal(pos));
        });
}

void
HduiDataSourceValueTreeView::SetDataSource(
    const HdSampledDataSourceHandle &dataSource)
{
    QAbstractItemModel *existingModel = model();

    _dataSource = dataSource;
    if (_dataSource) {
        setModel(Hdui_NewModelFromValue(_dataSource->GetValue(0.0f), this));

        header()->setSectionResizeMode(0, QHeaderView::Stretch);
        if (header()->count() > 1) {
            header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
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
