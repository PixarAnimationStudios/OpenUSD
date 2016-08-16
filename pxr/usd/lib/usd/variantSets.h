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
#ifndef USD_VARIANTSETS_H
#define USD_VARIANTSETS_H

#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/editTarget.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/sdf/declareHandles.h"

#include <string>
#include <vector>

SDF_DECLARE_HANDLES(SdfLayer);
SDF_DECLARE_HANDLES(SdfPrimSpec);
SDF_DECLARE_HANDLES(SdfVariantSetSpec);

class SdfPath;

/// \class UsdVariantSet
///
/// A UsdVariantSet represents a single VariantSet in USD
/// (e.g. modelingVariant or shadingVariant), which can have multiple
/// variations that express different sets of opinions about the scene
/// description rooted at the prim that defines the VariantSet.
///
/// (More detailed description of variants to follow)
///
class UsdVariantSet {
public:
#ifndef doxygen
    typedef const std::string UsdVariantSet::*_UnspecifiedBoolType;
#endif

    /// Author a variant spec for \a variantName in this VariantSet at the
    /// stage's current EditTarget.  Return true if the spec was successfully
    /// authored, false otherwise.
    ///
    /// This will create the VariantSet itself, if necessary, so as long as
    /// UsdPrim "prim" is valid, the following should always work:
    /// \code
    /// UsdVariantSet vs = prim.GetVariantSet("myVariantSet");
    /// vs.FindOrCreateVariant("myFirstVariation");
    /// vs.SetVariantSelection("myFirstVariation");
    /// {
    ///     UsdEditContext ctx(vs.GetVariantEditContext());
    ///     // Now all of our subsequent edits will go "inside" the 
    ///     // 'myFirstVariation' variant of 'myVariantSet'
    /// }
    /// \endcode
    bool FindOrCreateVariant(const std::string& variantName);

    /// Return the composed variant names for this VariantSet, ordered
    /// lexicographically.
    std::vector<std::string> GetVariantNames() const;

    /// Returns true if this VariantSet already possesses a variant 
    // named \p variantName in any layer.
    bool HasAuthoredVariant(const std::string& variantName) const;

    /// Return the the variant selection for this VariantSet.  If there is
    /// no selection, return the empty string.
    std::string GetVariantSelection() const;

    /// Returns true if there is a selection authored for this VariantSet
    /// in any layer.
    ///
    /// If requested, the variant selection (if any) will be returned in
    /// \p value .
    bool HasAuthoredVariantSelection(std::string *value = NULL) const;

    /// Author a variant selection for this VariantSet, setting it to
    /// \a variantName in the stage's current EditTarget.  Return true if the
    /// selection was successfully authored, false otherwise.
    bool SetVariantSelection(const std::string &variantName);

    /// Clear any selection for this VariantSet from the current EditTarget.
    /// Return true on success, false otherwise.
    bool ClearVariantSelection();

    /// Return a \a UsdEditTarget that edits the currently selected variant in
    /// this VariantSet in \a layer.  If there is no currently
    /// selected variant in this VariantSet, return an invalid EditTarget.
    ///
    /// If \a layer is unspecified, then we will use the layer of our prim's
    /// stage's current UsdEditTarget.
    ///
    /// Currently, we require \a layer to be in the stage's local LayerStack
    /// (see UsdStage::HasLocalLayer()), and will issue an error and return
    /// an invalid EditTarget if \a layer is not.  We may relax this
    /// restriction in the future, if need arises, but it introduces several
    /// complications in specification and behavior.
    UsdEditTarget
    GetVariantEditTarget(const SdfLayerHandle &layer = SdfLayerHandle()) const;

    /// Helper function for configuring a UsdStage's EditTarget to author
    /// into the currently selected variant.  Returns configuration for a
    /// UsdEditContext
    ///
    /// To begin editing into VariantSet \em varSet's currently selected
    /// variant:
    ///
    /// In C++, we would use the following pattern:
    /// \code
    /// {
    ///     UsdEditContext ctxt(varSet.GetVariantEditContext());
    ///
    ///     // All Usd mutation of the UsdStage on which varSet sits will
    ///     // now go "inside" the currently selected variant of varSet
    /// }
    /// \endcode
    ///
    /// In python, the pattern is:
    /// \code{.py}
    ///     with varSet.GetVariantEditContext():
    ///         # Now sending mutations to current variant
    /// \endcode
    ///
    /// See GetVariantEditTarget() for discussion of \p layer parameter
    std::pair<UsdStagePtr, UsdEditTarget>
    GetVariantEditContext(const SdfLayerHandle &layer = SdfLayerHandle()) const;


    /// Return this VariantSet's held prim.
    UsdPrim const &GetPrim() const { return _prim; }


    /// Return this VariantSet's name
    std::string const &GetName() const { return _variantSetName; }


    /// Is this UsdVariantSet object usable?  If not, calling any of
    /// its other methods is likely to crash.
    bool IsValid() const {
        return _prim;
    }

#ifdef doxygen
    /// Safe bool-conversion operator.  Equivalent to IsValid().
    operator unspecified-bool-type() const();
#else
    operator _UnspecifiedBoolType() const {
        return IsValid() ? &UsdVariantSet::_variantSetName : NULL;
    }
#endif // doxygen

private:
    UsdVariantSet(const UsdPrim &prim,
                  const std::string &variantSetName)
        : _prim(prim)
        , _variantSetName(variantSetName)
    {
    }

    SdfPrimSpecHandle _CreatePrimSpecForEditing(const SdfPath &path);
    SdfVariantSetSpecHandle _FindOrCreateVariantSet();

    UsdPrim _prim;
    std::string _variantSetName;

    friend class UsdPrim;
    friend class UsdVariantSets;
};


// TODO:
// VariantSet Names are stored as SdListOps, but a VariantSet is an actual spec
// (like a Prim). Is it important to make that distinction here?

/// \class UsdVariantSets
///
/// UsdVariantSets represents the collection of
/// \ref UsdVariantSet "VariantSets" that are present on a UsdPrim.
///
/// A UsdVariantSets object, retrieved from a prim via 
/// UsdPrim::GetVariantSets(), provides the API for interrogating and modifying
/// the composed list of VariantSets active defined on the prim, and also
/// the facility for authoring a VariantSet \em selection for any of those
/// VariantSets.
///
class UsdVariantSets {
public:

    /// Find an existing, or create a new VariantSet on the originating UsdPrim,
    /// named \p variantSetName.
    ///
    /// This step is not always necessary, because if this UsdVariantSets
    /// object is valid, then 
    /// \code
    /// varSetsObj.GetVariantSet(variantSetName).FindOrCreateVariant(variantName);
    /// \endcode
    /// will always succeed, creating the VariantSet first, if necessary.  This
    /// method exists for situations in which you want to create a VariantSet
    /// without necessarily populating it with variants.
    UsdVariantSet FindOrCreate(const std::string& variantSetName);

    // TODO: don't we want remove and reorder, clear, etc. also?

    /// Compute a list of all VariantSets authored on the originiating UsdPrim.
    /// Always return true.
    bool GetNames(std::vector<std::string>* names) const;

    /// Return a list of all VariantSets authored on the originiating UsdPrim.
    std::vector<std::string> GetNames() const;

    UsdVariantSet operator[](const std::string& variantSetName) const {
        return GetVariantSet(variantSetName);
    }

    /// Return a UsdVariantSet object for \p variantSetName.  This always
    /// succeeds, although the returned VariantSet will be invalid if
    /// the originating prim is invalid
    UsdVariantSet GetVariantSet(const std::string& variantSetName) const;

    /// Does a VariantSet named \p variantSetName exist on the originating
    /// prim?
    ///
    /// Note that VariantSet membership can be list-edited across composition
    /// arcs, so a return value of \c false indicates only that 
    /// \p variantSetName is not present in the stage's composed view - it
    /// may have been defined in referenced/inherited scene description, but
    /// pruned from consideration in stronger layers/arcs.
    bool HasVariantSet(const std::string& variantSetName) const;

    /// Return the composed variant selection for the VariantSet named
    /// \a variantSetName.  If there is no selection, (or \p variantSetName
    /// does not exist) return the empty string.
    std::string GetVariantSelection(const std::string& variantSetName) const;

    bool SetSelection(const std::string& variantSetName,
                      const std::string& variantName);

private:
    explicit UsdVariantSets(const UsdPrim& prim) 
        : _prim(prim)
    {
        /* NOTHING */
    }

    UsdPrim _prim;

    friend class UsdPrim;
};

#endif //USD_VARIANTSETS_H
