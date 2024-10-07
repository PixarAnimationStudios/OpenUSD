//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_NAMESPACE_EDITOR_H
#define PXR_USD_USD_NAMESPACE_EDITOR_H

/// \file usd/namespaceEditor.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/pcp/dependentNamespaceEditUtils.h"
#include "pxr/usd/sdf/namespaceEdit.h"

PXR_NAMESPACE_OPEN_SCOPE

/// @warning
/// This code is a work in progress and should not be used in production 
/// scenarios. It is currently not feature-complete and subject to change.
///
/// Provides namespace editing operations 
class UsdNamespaceEditor 
{
public:
    /// Structure for holding the options for how the namespace editor will
    /// behave when trying to perform edits.
    struct EditOptions {

        /// Whether the namespace editor will allow the authoring of relocates
        /// in order to perform edits that would otherwise not be possible 
        /// because of opinions across composition arcs. By default this is set
        /// to true. If set to false the namespace editor will consider edits
        /// that require relocates as errors and will not apply the edit.
        bool allowRelocatesAuthoring = true;
    };

    USD_API
    explicit UsdNamespaceEditor(const UsdStageRefPtr &stage);

    USD_API
    UsdNamespaceEditor(
        const UsdStageRefPtr &stage, 
        EditOptions &&editOptions);

    USD_API
    UsdNamespaceEditor(
        const UsdStageRefPtr &stage, 
        const EditOptions &editOptions);

    /// \name Dependent Stages
    ///
    /// Dependent stages are additional stages that may have composition 
    /// dependencies on the layer edits made for the editor's primary stage.
    /// By adding dependent stages, the editor can make additional edits so that
    /// affected composition arcs and specs that depend on affected composition
    /// in composed prims on these stages are updated to compose with the moved
    /// prim specs or, in the case of deletions, removed when the specs they 
    /// depend on are removed.
    ///
    /// Dependencies in the dependent stages are based only what is currently
    /// loaded for those stages. In other words, the editor cannot find and
    /// edit dependencies from unloaded payloads, inactive prim children, 
    /// prims that are load mask filtered, unselected variants, etc. The primary
    /// stage of this editor is always a dependent stage, meaning that edits 
    /// will always be made to maintain affected composition dependencies in the
    /// primary stage.
    ///
    /// @{

    /// Adds the given \p stage as a dependent stage of this namespace editor.
    USD_API
    void AddDependentStage(const UsdStageRefPtr &stage);

    /// Removes the given \p stage as a dependent stage of this namespace editor.
    USD_API
    void RemoveDependentStage(const UsdStageRefPtr &stage);

    /// Sets the list of dependent stages for this namespace editor to 
    /// \p stages.
    USD_API
    void SetDependentStages(const UsdStageRefPtrVector &stages);

    /// @}

    /// Adds an edit operation to delete the composed prim at the given \p path 
    /// from this namespace editor's stage.
    ///
    /// Returns true if the path is a valid possible composed prim path; returns
    /// false and emits a coding error if not.
    USD_API
    bool DeletePrimAtPath(
        const SdfPath &path);

    /// Adds an edit operation to move the composed prim at the given \p path
    /// on this namespace editor's stage to instead be at the path \p newPath.   
    ///
    /// Returns true if both paths are valid possible composed prim path; 
    /// returns false and emits a coding error if not.
    USD_API
    bool MovePrimAtPath(
        const SdfPath &path, 
        const SdfPath &newPath);

    /// Adds an edit operation to delete the composed prim at the path of 
    /// \p prim from this namespace editor's stage. This is equivalent to 
    /// calling DeletePrimAtPath(prim.GetPath())
    ///
    /// Returns true if the prim provides a valid possible composed prim path; 
    /// returns false and emits a coding error if not.
    USD_API
    bool DeletePrim(
        const UsdPrim &prim);

    /// Adds an edit operation to rename the composed prim at the path of 
    /// \p prim on this namespace editor's stage to instead have the name
    /// \p newName.
    ///
    /// Returns true if the prim provides a valid possible composed prim path 
    /// and the new name is a valid possible prim name; returns false and emits 
    /// a coding error if not.
    USD_API
    bool RenamePrim(
        const UsdPrim &prim, 
        const TfToken &newName);

    /// Adds an edit operation to reparent the composed prim at the path of
    /// \p prim on this namespace editor's stage to instead be a namespace 
    /// child of the composed prim at the path of \p newParent.
    ///
    /// Returns true if the both the prim and the new parent prim provide a 
    /// valid possible composed prim paths; returns false and emits a coding 
    /// error if not.
    USD_API
    bool ReparentPrim(
        const UsdPrim &prim, 
        const UsdPrim &newParent);

    /// Adds an edit operation to reparent the composed prim at the path of
    /// \p prim on this namespace editor's stage to instead be a prim named
    /// \p newName that is a namespace child of the composed prim at the  
    /// path of \p newParent.
    ///
    /// Returns true if the both the prim and the new parent prim provide a 
    /// valid possible composed prim paths and the new name is a valid prim 
    /// name; returns false and emits a coding error if not.
    USD_API
    bool ReparentPrim(
        const UsdPrim &prim, 
        const UsdPrim &newParent,
        const TfToken &newName);

    /// Adds an edit operation to delete the composed property at the given 
    /// \p path from this namespace editor's stage.
    ///
    /// Returns true if the path is a valid possible composed property path; 
    /// returns false and emits a coding error if not.
    USD_API
    bool DeletePropertyAtPath(
        const SdfPath &path);

    /// Adds an edit operation to move the composed property at the given 
    /// \p path on this namespace editor's stage to instead be at the path 
    /// \p newPath.
    ///
    /// Returns true if both paths are valid possible composed property path; 
    /// returns false and emits a coding error if not.
    USD_API
    bool MovePropertyAtPath(
        const SdfPath &path, 
        const SdfPath &newPath);

    /// Adds an edit operation to delete the composed property at the path of
    /// \p property from this namespace editor's stage. This is equivalent to 
    /// calling DeletePropertyAtPath(property.GetPath())
    ///
    /// Returns true if the property provides a valid possible composed property
    /// path; returns false and emits a coding error if not.
    USD_API
    bool DeleteProperty(
        const UsdProperty &property);

    /// Adds an edit operation to rename the composed property at the path of 
    /// \p property on this namespace editor's stage to instead have the name 
    /// \p newName.
    ///
    /// Returns true if the property provides a valid possible composed property
    /// path and the new name is a valid possible property name; returns false 
    /// and emits a coding error if not.
    USD_API
    bool RenameProperty(
        const UsdProperty &property, 
        const TfToken &newName);

    /// Adds an edit operation to reparent the composed property at the path of
    /// \p property on this namespace editor's stage to instead be a namespace 
    /// child of the composed property at the path of \p newParent.
    ///
    /// Returns true if the both the property and the new parent prim provide a 
    /// valid possible composed paths; returns false and emits a coding 
    /// error if not.
    USD_API
    bool ReparentProperty(
        const UsdProperty &property, 
        const UsdPrim &newParent);

    /// Adds an edit operation to reparent the composed property at the path of
    /// \p property on this namespace editor's stage to instead be a property 
    /// named \p newName that is a namespace child of the composed prim at the 
    /// path of \p newParent.
    ///
    /// Returns true if the both the property and the new parent prim provide a 
    /// valid possible composed paths and the new name is a valid property 
    /// name; returns false and emits a coding error if not.
    USD_API
    bool ReparentProperty(
        const UsdProperty &property, 
        const UsdPrim &newParent,
        const TfToken &newName);

    /// Applies all the added namespace edits stored in this to namespace editor
    /// to its stage by authoring all scene description in the layer stack of 
    /// the current edit target necessary to move or delete the composed 
    /// objects that the edit paths refer to..
    ///
    /// Returns true if all the necessary edits are successfully performed; 
    /// returns false and emits a coding error otherwise. 
    USD_API
    bool ApplyEdits();

    /// Returns whether all the added namespace edits stored in this to 
    /// namespace editor can be applied to its stage. 
    ///
    /// In other words, this returns whether ApplyEdits should be successful if
    /// it were called right now. If this would return false and \p whyNot is 
    /// provided, the reasons ApplyEdits would fail will be copied to whyNot.
    USD_API
    bool CanApplyEdits(std::string *whyNot = nullptr) const;

private:

    // The type of edit that an edit description is describing.
    enum class _EditType {
        Invalid, 

        Delete,
        Rename,
        Reparent
    };

    // Description of an edit added to this namespace editor.
    struct _EditDescription {
        // Path to the existing object.
        SdfPath oldPath;
        // New path of the object after the edit is performed. An empty path 
        // indicates that the edit operation will delete the object.
        SdfPath newPath;

        // Type of the edit as determined by the oldPath and the newPath.
        _EditType editType = _EditType::Invalid;

        // Whether this describes a property edit or, otherwise, a prim edit.
        bool IsPropertyEdit() const { return oldPath.IsPrimPropertyPath(); }
    };

    // Struct representing the Sdf layer edits necessary to apply an edit 
    // description to the stage. We need this to gather all the information we 
    // can about what layer edits need to be performed before we start editing 
    // any specs so that we can avoid partial edits when a composed stage level
    // namespace would fail.
    struct _ProcessedEdit
    {
        // List of errors encountered that would prevent the overall namespace
        // edit of the composed stage object from being completed successfully.
        std::vector<std::string> errors;

        // The edit description of the primary edit.
        _EditDescription editDescription;

        // The list of layers that have specs that need to have the Sdf 
        // namespace edit applied.
        SdfLayerHandleVector layersToEdit;

        // Whether performing the edit will author new relocates.
        bool willAuthorRelocates = false;

        // Layer edits that need to be performed to update connection and 
        // relationship targets of other properties in order to keep them 
        // targeting the same object after applying this processed edit.
        struct TargetPathListOpEdit {
            // Property spec to author the new targets value to. Note that we
            // store the spec handle for the property as the property spec's
            // path could change if the property is moved or deleted by the 
            // primary namespace edit.
            SdfPropertySpecHandle propertySpec;

            // Name of the field that holds the path targets for the property
            // which differs for attributes vs relationships.  
            TfToken fieldName;

            // Updated list op value to set for the property spec.
            SdfPathListOp newFieldValue;
        };
        std::vector<TargetPathListOpEdit> targetPathListOpEdits;

        // Full set of namespace edits that need to be performed for all the
        // dependent stages of this editor as a result of dependencies on the
        // initial spec move edits.
        PcpDependentNamespaceEdits dependentStageNamespaceEdits;

        // List of errors encountered that would prevent connection and 
        // relationship target edits from being performed in response to the
        // namespace edits.
        std::vector<std::string> targetPathListOpErrors;

        // Applies this processed edit, performing the individual edits 
        // necessary to each layer that needs to be updated.
        bool Apply();

        // Returns whether this processed edit can be applied.
        bool CanApply(std::string *whyNot) const;
    };

    // Adds an edit description for a prim delete operation.
    bool _AddPrimDelete(const SdfPath &oldPath);

    // Adds an edit description for a prim rename or reparent operation.
    bool _AddPrimMove(const SdfPath &oldPath, const SdfPath &newPath);

    // Adds an edit description for a property delete operation.
    bool _AddPropertyDelete(const SdfPath &oldPath);

    // Adds an edit description for a property rename or reparent operation.
    bool _AddPropertyMove(const SdfPath &oldPath, const SdfPath &newPath);

    // Clears the current procesed edits.
    void _ClearProcessedEdits();

    // Processes and caches the layer edits necessary for the current edit 
    // operation if there is no cached processecd edit.
    void _ProcessEditsIfNeeded() const;

    // Helper class for _ProcessEditsIfNeeded. Defined entirely in 
    // implementation. Declared here for private access to the editor 
    // structures.
    class _EditProcessor;

    UsdStageRefPtr _stage;
    // Dependent stage order should be arbitrary but we want don't want 
    // duplicates which can cause unnecessary work.
    using _StageSet = std::unordered_set<UsdStageRefPtr, TfHash>;
    _StageSet _dependentStages;
    EditOptions _editOptions;
    _EditDescription _editDescription;
    mutable std::optional<_ProcessedEdit> _processedEdit;   
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_NAMESPACE_EDITOR_H

