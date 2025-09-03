//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_OBJECT_H
#define PXR_EXEC_VDF_OBJECT_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"

#include "pxr/base/tf/token.h"

#include <set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfConnection;
class VdfInput;
class VdfNode;
class VdfOutput;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfObjectPtr
///
/// An universal class to represent pointers to various Vdf types.
///
///XXX: We are considering using a base class within Vdf, but we want to 
///     measure the speed/size impact first.
///
///XXX: We could pack everything into 8 bytes and use it as value type.
///
class VdfObjectPtr
{
public:

    /// Type of object.
    enum Type
    {
        Undefined = -1, // Marks the undefined type.

        Node,           // The object is a VdfNode.
        Connection,     // The object is a VdfConnection.
        Input,          // The object is a VdfInput.
        Output          // The object is a VdfOutput.
    };

    /// Ctor to create the NULL object.
    ///
    VdfObjectPtr()
    :   _node(NULL),
        _type(Undefined),
        _isConst(false) {}

    /// Ctor to create an anchor from a \p node.
    ///
    VdfObjectPtr(VdfNode *node)
    :   _node(node),
        _type(Node),
        _isConst(false) {}

    /// Ctor to create an anchor from a const \p node.
    ///
    VdfObjectPtr(const VdfNode *node)
    :   _constNode(node),
        _type(Node),
        _isConst(true) {}

    /// Ctor to create an anchor from a \p connection.
    ///
    VdfObjectPtr(VdfConnection *connection)
    :   _connection(connection),
        _type(Connection),
        _isConst(false) {}

    /// Ctor to create an anchor from a const \p connection.
    ///
    VdfObjectPtr(const VdfConnection *connection)
    :   _constConnection(connection),
        _type(Connection),
        _isConst(true) {}

    /// Ctor to create an anchor from a \p input.
    ///
    VdfObjectPtr(VdfInput *input)
    :   _input(input),
        _type(Input),
        _isConst(false) {}

    /// Ctor to create an anchor from a \p input.
    ///
    VdfObjectPtr(const VdfInput *input)
    :   _constInput(input),
        _type(Input),
        _isConst(true) {}

    /// Ctor to create an anchor from a \p output.
    ///
    VdfObjectPtr(VdfOutput *output)
    :   _output(output),
        _type(Output),
        _isConst(false) {}

    /// Ctor to create an anchor from a \p output.
    ///
    VdfObjectPtr(const VdfOutput *output)
    :   _constOutput(output),
        _type(Output),
        _isConst(true) {}

    /// Assignment operator.
    ///
    VdfObjectPtr &operator=(const VdfObjectPtr &rhs)
    {
        _node    = rhs._node;
        _type    = rhs._type;
        _isConst = rhs._isConst;

        return *this;
    }

    /// Less than operator used for map.
    ///
    bool operator<(const VdfObjectPtr &rhs) const
    {
        return _node < rhs._node;
    }

    bool operator<=(const VdfObjectPtr &rhs) const
    {
        return !(rhs < *this);
    }

    /// More than operator used for map.
    ///
    bool operator>(const VdfObjectPtr &rhs) const
    {
        return rhs < *this;
    }

    bool operator>=(const VdfObjectPtr &rhs) const
    {
        return !(*this < rhs);
    }

    /// Equality operator. Note that constness doesn't matter.
    ///
    bool operator==(const VdfObjectPtr &rhs) const
    {
        return _type == rhs._type &&
               _node == rhs._node;
    }

    /// Not equal operator.
    ///
    bool operator!=(const VdfObjectPtr &rhs) const
    {
        return !(*this == rhs);
    }

    /// Functor to use for hash maps.
    ///
    struct HashFunctor
    {
        size_t operator()(const VdfObjectPtr &obj) const
        {
            return (size_t)obj._node;
        }
    };

    /// Returns false, if the object holds the NULL object.
    ///
    operator bool() const
    {
        return _node;
    }

    /// Returns the type of the object.
    ///
    Type GetType() const
    {
        return _type;
    }

    /// Returns true, if the object being hold is const.
    ///
    bool IsConst() const
    {
        return _isConst;
    }

    /// Returns true if the object is a node.
    ///
    bool IsNode() const
    {
        return _type == Node;
    }

    /// Returns true if the object is a node.
    ///
    bool IsConnection() const
    {
        return _type == Connection;
    }

    /// Returns true if the object is a node.
    ///
    bool IsInput() const
    {
        return _type == Input;
    }

    /// Returns true if the object is a node.
    ///
    bool IsOutput() const
    {
        return _type == Output;
    }

    /// Returns a non-const pointer to a node. Fails if object is const or 
    /// not a node (cf. GetIfNode()).
    ///
    VdfNode *GetNonConstNode() const
    {   
        TF_AXIOM(IsNode());
        TF_AXIOM(!IsConst());
        return _node;
    }

    /// Returns a non-const pointer to a connection. Fails if object is const 
    /// or not a connection (cf. GetIfNode()).
    ///
    VdfConnection *GetNonConstConnection() const
    {   
        TF_AXIOM(IsConnection());
        TF_AXIOM(!IsConst());
        return _connection;
    }

    /// Returns a non-const pointer to an input. Fails if object is const or 
    /// not an input (cf. GetIfNode()).
    ///
    VdfInput *GetNonConstInput() const
    {   
        TF_AXIOM(IsInput());
        TF_AXIOM(!IsConst());
        return _input;
    }

    /// Returns a non-const pointer to an output. Fails if object is const or 
    /// not an output (cf. GetIfNode()).
    ///
    VdfOutput *GetNonConstOutput() const
    {   
        TF_AXIOM(IsOutput());
        TF_AXIOM(!IsConst());
        return _output;
    }

    /// Returns a const reference to a node. Fails if object is not a node.
    ///
    const VdfNode &GetNode() const
    {   
        TF_AXIOM(IsNode());
        return *_constNode;
    }

    /// Returns a const reference to a connection. Fails if object is not a
    /// connection.
    ///
    const VdfConnection &GetConnection() const
    {   
        TF_AXIOM(IsConnection());
        return *_constConnection;
    }

    /// Returns a const reference to an input. Fails if object is not an input.
    ///
    const VdfInput &GetInput() const
    {   
        TF_AXIOM(IsInput());
        return *_constInput;
    }

    /// Returns a const reference to an output. Fails if object is not an
    /// output.
    ///
    const VdfOutput &GetOutput() const
    {   
        TF_AXIOM(IsOutput());
        return *_constOutput;
    }

    /// Returns a pointer to a VdfNode if the object is holding a node,
    /// NULL otherwise.
    ///
    const VdfNode *GetIfNode() const
    {   
        return IsNode() ? _constNode : NULL;
    }

    /// Returns a pointer to a VdfConnection if the object is holding a
    /// connection, NULL otherwise.
    ///
    const VdfConnection *GetIfConnection() const
    {   
        return IsConnection() ? _constConnection : NULL;
    }

    /// Returns a pointer to a VdfInput if the object is holding a input,
    /// NULL otherwise.
    ///
    const VdfInput *GetIfInput() const
    {   
        return IsInput() ? _constInput : NULL;
    }

    /// Returns a pointer to a VdfOutput if the object is holding a output,
    /// NULL otherwise.
    ///
    const VdfOutput *GetIfOutput() const
    {   
        return IsOutput() ? _constOutput : NULL;
    }

    /// Returns a pointer to a VdfNode if the object is holding a non-const node,
    /// NULL otherwise.
    ///
    VdfNode *GetIfNonConstNode() const
    {   
        return (!IsConst() && IsNode()) ? _node : NULL;
    }

    /// Returns a pointer to a VdfConnection if the object is holding a
    /// non-const connection, NULL otherwise.
    ///
    VdfConnection *GetIfNonConstConnection() const
    {   
        return (!IsConst() && IsConnection()) ? _connection : NULL;
    }

    /// Returns a pointer to a VdfInput if the object is holding a non-const
    /// input, NULL otherwise.
    ///
    VdfInput *GetIfNonConstInput() const
    {   
        return (!IsConst() && IsInput()) ? _input : NULL;
    }

    /// Returns a pointer to a VdfOutput if the object is holding a non-const
    /// output, NULL otherwise.
    ///
    VdfOutput *GetIfNonConstOutput() const
    {   
        return (!IsConst() && IsOutput()) ? _output : NULL;
    }

    /// Returns the owning node of this object if object is an input or output.
    /// Returns the node itself if object is a node and returns NULL if object
    /// is a connection.
    ///
    VDF_API
    const VdfNode *GetOwningNode() const;

    /// Returns a debug name for this object.
    ///
    VDF_API
    std::string GetDebugName() const;

    /// Returns the identity of this object as a void *.
    ///
    const void *GetIdentity() const
    {
        // Note: Since all different types are stored in an union, we just
        //       pick the first const type to return.
        return _constNode;
    }

// -----------------------------------------------------------------------------

private:

    // Stores pointers as needed for the different object types.
    union
    {
        VdfNode             *_node;
        VdfConnection       *_connection;
        VdfInput            *_input;
        VdfOutput           *_output;

        const VdfNode       *_constNode;
        const VdfConnection *_constConnection;
        const VdfInput      *_constInput;
        const VdfOutput     *_constOutput;
    };

    // Holds the type of the object.
    Type    _type;

    // If set, the object being held is const.
    bool    _isConst;
};

/// An object vector.
///
typedef std::vector<VdfObjectPtr> VdfObjectPtrVector;

/// An object set.
///
typedef std::set<VdfObjectPtr> VdfObjectPtrSet;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
