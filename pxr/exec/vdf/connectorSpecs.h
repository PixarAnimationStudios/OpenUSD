//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_CONNECTOR_SPECS_H
#define PXR_EXEC_VDF_CONNECTOR_SPECS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/inputSpec.h"
#include "pxr/exec/vdf/outputSpec.h"
#include "pxr/exec/vdf/tokens.h"

#include "pxr/base/tf/type.h"
#include "pxr/base/tf/smallVector.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// The classes contained in this file are the containers for input and 
/// output specs.  The typical usage pattern is this:
///
/// \code
///     {
///           ....
///           VdfInputSpecs  inputs;
///           inputs
///              .ReadConnector<GfVec3d>(TfToken("axis"))
///              .ReadConnector<double>(TfToken("length"))
///              .ReadWriteConnector<GfVec3d>>(TfToken("moves"),
///                                            TfToken("out"))
///              ;
///     }
/// \endcode
///


////////////////////////////////////////////////////////////////////////////////
///
/// VdfConnectorSpecs is a container for VdfConnectorSpec objects.  This is a
/// base class for the concrete containers of input connector specs and output
/// connector specs.
///
template < typename T >
class VdfConnectorSpecs
{
    // We choose a small vector with default size 1 to optimize for common
    // cases, including nodes with a single output.
    using _VectorType = TfSmallVector<T*, 1>;

public:
    /// Iterator type.
    using const_iterator = typename _VectorType::const_iterator;

    /// Constructor.
    VdfConnectorSpecs() {}

    /// Copy constructor
    VdfConnectorSpecs(const VdfConnectorSpecs<T> &rhs) {
        // Copy other vector into our new one.
        Append(rhs);
    }

    /// Move constructor
    VdfConnectorSpecs(VdfConnectorSpecs<T> &&rhs)
        : _specs(std::move(rhs._specs)) {
    }

    /// Copy assignment operator
    VdfConnectorSpecs<T> &operator=(const VdfConnectorSpecs<T> &rhs) {
        // Clear our specs, and copy the others into them.
        _ClearSpecs();
        Append(rhs);
        return *this;
    }

    /// Move assignment operator
    VdfConnectorSpecs<T> &operator=(VdfConnectorSpecs<T> &&rhs) {
        // Clear our specs, and move the others into them.
        _ClearSpecs();
        _specs = std::move(rhs._specs);
        return *this;
    }

    /// Appends a set of specs to this one.
    void Append(const VdfConnectorSpecs<T> &specs) {
        _specs.reserve(_specs.size() + specs._specs.size());
        for (const T *spec : specs._specs) {
            _specs.push_back( new T(*spec) );
        }
    }

    /// Allocates space for \p numSpecs connector specs, to avoid re-allocation
    /// when adding specs, when the number of specs is known ahead of time.
    void Reserve(size_t numSpecs) {
        _specs.reserve(numSpecs);
    }

    /// Returns number of connectors in this spec
    size_t GetSize() const { return _specs.size(); }

    /// Returns a const_iterator to the beginning of the connectors.
    const_iterator begin() const { return _specs.begin(); }

    /// Returns a const_iterator to the end of the connectors.
    const_iterator end() const { return _specs.end(); }

protected:

    /// Destructor.
    ///
    /// Note, the destructor is not virtual on purpose (we don't want the
    /// vtable), but we make it protected to prevent destruction through a 
    /// base class pointer.
    ~VdfConnectorSpecs() { 
        _ClearSpecs();
    }

    /// Clears list of specs.
    void _ClearSpecs() {
        const_iterator e = _specs.end();
        for (const_iterator i = _specs.begin(); i != e; ++i) {
            delete *i;
        }
        _specs.clear();
    }

    /// Returns connector spec at index \c idx.
    const T *_GetConnectorSpec(int idx) const 
    { 
        return _specs[idx]; 
    }

    /// Adds a connector to our list.
    void _AddConnector(T *cs) 
    {
        _specs.push_back(cs);
    }

private:

    // A list of specs held in this container class.
    _VectorType _specs;
};


////////////////////////////////////////////////////////////////////////////////
///
/// VdfInputSpecs is a container for VdfInputSpec objects.  
/// These objects are used to describe inputs on a VdfNode.
///
class VdfInputSpecs : public VdfConnectorSpecs<VdfInputSpec>
{
public:

    /// Create a "Read" connector with the name \p inName and optionally
    /// associated with the output named \p outName.
    ///
    /// A read connector that has an associated output, tells the system
    /// that the masks coming in from \p outName can be propogated to 
    /// the input \p inName.
    ///
    template <typename T> VdfInputSpecs &
    ReadConnector(const TfToken &inName,
                  const TfToken &outName = VdfTokens->empty,
                  bool prerequisite = false)
    {
        _AddConnector(VdfInputSpec::New<T>(
            inName, outName, VdfInputSpec::READ,  prerequisite));
        return *this;
    }

    /// Create a "Read/Write" connector with the name \p inName associated
    /// with the output named \p outName.
    ///
    template <typename T> VdfInputSpecs &
    ReadWriteConnector(const TfToken &inName, const TfToken &outName) 
    {
        _AddConnector(VdfInputSpec::New<T>(
            inName, outName, VdfInputSpec::READWRITE, /* prerequisite */false));
        return *this;
    }

    /// Returns connector spec at index \c idx.
    ///
    const VdfInputSpec *GetInputSpec(int idx) const 
    { 
        return _GetConnectorSpec(idx);
    }

    /// Non-templated version of ReadConnector(), the given type must be
    /// registered for runtime type dispatching.
    VdfInputSpecs &
    ReadConnector(
        const TfType &type,
        const TfToken &inName,
        const TfToken &outName = VdfTokens->empty,
        bool prerequisite = false)
    {
        _AddConnector(
            VdfInputSpec::New(
                type, inName, outName, VdfInputSpec::READ, prerequisite));
        return *this;
    }

    /// Non-templated version of ReadWriteConnector(), the given type must be
    /// registered for runtime type dispatching.
    VdfInputSpecs &
    ReadWriteConnector(
        const TfType &type,
        const TfToken &inName,
        const TfToken &outName)
    {
        _AddConnector(
            VdfInputSpec::New(
                type, inName, outName, VdfInputSpec::READWRITE,
                /*prerequisite*/ false));
        return *this;
    }

    /// Returns true if two InputSpecs are equal.
    ///
    VDF_API
    bool operator==(const VdfInputSpecs &rhs) const;
    bool operator!=(const VdfInputSpecs &rhs) const {
        return !(*this == rhs);
    }
};


////////////////////////////////////////////////////////////////////////////////
///
/// VdfOutputSpecs is a container for VdfOutputSpec objects.  
/// These objects are used to describe outputs on a VdfNode.
///
class VdfOutputSpecs : public VdfConnectorSpecs<VdfOutputSpec>
{
public:

    /// Create an "Out" connector with the given name.
    template <typename T> VdfOutputSpecs &
    Connector(const TfToken &name) 
    {
        _AddConnector(VdfOutputSpec::New<T>(name));
        return *this;
    }

    /// Returns connector spec at index \c idx.
    const VdfOutputSpec *GetOutputSpec(int idx) const 
    { 
        return _GetConnectorSpec(idx);
    }

    /// Non-templated version of Connector(), the given type must be
    /// registered for runtime type dispatching.
    VdfOutputSpecs &
    Connector(const TfType &type, const TfToken &name) 
    {
        _AddConnector(VdfOutputSpec::New(type, name));
        return *this;
    }

    /// Returns true if two OutputSpecs are equal.
    ///
    VDF_API
    bool operator==(const VdfOutputSpecs &rhs) const;
    bool operator!=(const VdfOutputSpecs &rhs) const {
        return !(*this == rhs);
    }
};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
