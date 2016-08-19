//
// Copyright 2016 Pixar
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
#ifndef SDF_NOTICE_H
#define SDF_NOTICE_H

/// \file sdf/notice.h

#include "pxr/usd/sdf/changeList.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/notice.h"

SDF_DECLARE_HANDLES(SdfLayer);

/// \class SdfNotice
///
/// Wrapper class for Sdf notices.
///
class SdfNotice {
public:
    /// \class Base
    ///
    /// Base notification class for scene.  Only useful for type hierarchy
    /// purposes.
    ///
    class Base : public TfNotice {
    public:
        ~Base();
    };

    /// \class BaseLayersDidChange
    ///
    /// Base class for LayersDidChange and LayersDidChangeSentPerLayer.
    ///
    class BaseLayersDidChange {
    public:
        BaseLayersDidChange(const SdfLayerChangeListMap &changeMap,
                            size_t serialNumber)
            : _map(&changeMap)
            , _serialNumber(serialNumber)
            {}

        /// A list of layers changed.
        SdfLayerHandleVector GetLayers() const;

        /// A map of layers to the changes that occurred to them.
        const SdfLayerChangeListMap &GetChangeListMap() const { return *_map; }

        /// The the serial number for this round of change processing.
        size_t GetSerialNumber() const { return _serialNumber; }

    private:
        const SdfLayerChangeListMap *_map;
        const size_t _serialNumber;
    };

    /// \class LayersDidChangeSentPerLayer
    ///
    /// Notice sent per-layer indicating all layers whose contents have changed
    /// within a single round of change processing.  If more than one layer
    /// changes in a single round of change processing, we send this notice once
    /// per layer with the same changeMap and serialNumber.  This is so clients
    /// can listen to notices from only the set of layers they care about rather
    /// than listening to the global LayersDidChange notice.
    ///
    class LayersDidChangeSentPerLayer 
        : public Base, public BaseLayersDidChange {
    public:
        LayersDidChangeSentPerLayer(const SdfLayerChangeListMap &changeMap,
                                    size_t serialNumber)
            : BaseLayersDidChange(changeMap, serialNumber) {}
        virtual ~LayersDidChangeSentPerLayer();
    };

    /// \class LayersDidChange
    ///
    /// Global notice sent to indicate that layer contents have changed.
    ///
    class LayersDidChange
        : public Base, public BaseLayersDidChange {
    public:
        LayersDidChange(const SdfLayerChangeListMap &changeMap,
                        size_t serialNumber)
            : BaseLayersDidChange(changeMap, serialNumber) {}
        virtual ~LayersDidChange();
    };

    /// \class LayerInfoDidChange
    ///
    /// Sent when the (scene spec) info of a layer have changed.
    ///
    class LayerInfoDidChange : public Base {
    public:
        LayerInfoDidChange( const TfToken &key ) :
            _key(key) {}
        ~LayerInfoDidChange();

        /// Return the key affected.
        const TfToken & key() const { return _key; }
    private:
        TfToken _key;
    };

    /// \class LayerIdentifierDidChange
    ///
    /// Sent when the identifier of a layer has changed.
    ///
    class LayerIdentifierDidChange : public Base {
    public:
        LayerIdentifierDidChange(const std::string& oldIdentifier,
                                 const std::string& newIdentifier);
        ~LayerIdentifierDidChange();

        /// Returns the old identifier for the layer.
        const std::string& GetOldIdentifier() const { return _oldId; }

        /// Returns the new identifier for the layer.
        const std::string& GetNewIdentifier() const { return _newId; }

    private:
        std::string _oldId;
        std::string _newId;
    };
    
    /// \class LayerDidReplaceContent
    ///
    /// Sent after a menv layer has been loaded from a file.
    ///
    class LayerDidReplaceContent : public Base {
    public:
        ~LayerDidReplaceContent();
    };

    /// \class LayerDidReloadContent
    /// Sent after a layer is reloaded.
    class LayerDidReloadContent : public LayerDidReplaceContent {
    public:
        virtual ~LayerDidReloadContent();
    };
    
    /// \class LayerDidSaveLayerToFile
    ///
    /// Sent after a layer is saved to file.
    ///
    class LayerDidSaveLayerToFile : public Base {
    public:
        ~LayerDidSaveLayerToFile();
    };

    /// \class LayerDirtinessChanged
    ///
    /// Similar behavior to LayersDidChange, but only gets sent if a change
    /// in the dirty status of a layer occurs.
    ///
    class LayerDirtinessChanged : public Base {
    public:
        ~LayerDirtinessChanged();
    };

    /// \class LayerMutenessChanged
    ///
    /// Sent after a layer has been added or removed from the set of
    /// muted layers. Note this does not necessarily mean the specified
    /// layer is currently loaded.
    ///
    class LayerMutenessChanged : public Base {
    public:
        LayerMutenessChanged(const std::string& layerPath, bool wasMuted)
            : _layerPath(layerPath)
            , _wasMuted(wasMuted)
        { }

        ~LayerMutenessChanged();

        /// Returns the path of the layer that was muted or unmuted.
        const std::string& GetLayerPath() const { return _layerPath; }

        /// Returns true if the layer was muted, false if unmuted.
        bool WasMuted() const { return _wasMuted; }

    private:
        std::string _layerPath;
        bool _wasMuted;
    };
};

#endif
