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
#ifndef USDOBJ_STREAM_H
#define USDOBJ_STREAM_H

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"

#include <string>
#include <vector>

/// \class UsdObjStream
///
/// A representation of an OBJ geometry data stream.  A data object contains a
/// collection of verts, UVs, and normals.  It also contains Faces, which are
/// made up of lists of Points.  A Point is a triple of indexes into the verts,
/// UVs, and normals.  An OBJ can also contain comments and other arbitrary
/// text.
///
/// A data object maintains an overall ordering of its sequence of elements.
/// That is, the order of vertex declarations, uvs, normals, groups, comments,
/// arbitrary text, etc.  This is important since the order can have semantic
/// meaning.  For example, a comment may have some relevance associated with
/// subsequent verts, uvs, or groups.
///
/// Deserialization is also supported.  See \relates UsdObjReadDataFromFile
/// \relates UsdObjReadDataFromStream.
///
class UsdObjStream
{
public:
    /// A "Point" identifies a vertex, a uv, and a normal by indexes into
    /// arrays.  NOTE!  These indexes are 0-based, unlike in the OBJ file format
    /// where indexes are 1-based.  A point can have -1 for its normal and uv.
    /// This indicates that the point has no normal or uv.
    struct Point {
        int vertIndex;
        int uvIndex;
        int normalIndex;

        /// Default constructor leaves all indexes invalid.
        Point() : vertIndex(-1), uvIndex(-1), normalIndex(-1) {}

        /// Construct with indexes \a v, \a uv, and \a n.
        Point(int v, int uv, int n)
            : vertIndex(v), uvIndex(uv), normalIndex(n) {}
    };

    /// A face is a pair of indices denoting a range in a vector of Points.  The
    /// first element indexes the first Point in the face, the second element is
    /// one past the last Point in the face.
    struct Face {
        int pointsBegin;
        int pointsEnd;

        /// Default constructor leaves range == [0, 0).
        Face();

        /// Construct with range specified by [\a begin, \a end).
        Face(int begin, int end) : pointsBegin(begin), pointsEnd(end) {}

        /// Return the number of points in this face.
        inline int size() const { return pointsEnd - pointsBegin; }

        /// Return true if \a lhs has the same range as \a rhs.
        friend bool operator==(Face const &lhs, Face const &rhs);

        /// Return true if \a lhs has a different range from \a rhs.
        friend bool operator!=(Face const &lhs, Face const &rhs);
    };

    /// A group is a sequence of faces wtih a name.
    struct Group {
        std::string name;
        std::vector<Face> faces;
    };

    /// A sequence element, indicating a series of one or more data elements in
    /// order.
    struct SequenceElem {
        /// Data element type.
        enum ElemType {
            Verts,
            UVs,
            Normals,
            Groups,
            Comments,
            ArbitraryText,
        };

        // This element's type.
        ElemType type;

        /// Number of \a type elements in order.  For example, 100 verts would
        /// be type: ElemVerts, repeat: 100.
        int repeat;

        /// Default constructor leaves members uninitialized.
        SequenceElem() {}

        /// Construct with \a ElemType \p t, and an optional repeat count \a r.
        explicit SequenceElem(ElemType t, int r = 1) : type(t), repeat(r) {}
    };

    /// Construct with an optional epsilon value.
    explicit UsdObjStream();

    /// Clear the contents of this data object.  Leaves no verts, uvs, points,
    /// or groups.
    void clear();

    /// Swap the contents of this data object with \a other.
    void swap(UsdObjStream &other);

    /// Add the contents of \a data into this data object.  Group names are
    /// uniqued if necessary, by adding numerical suffixes, e.g. groupName ->
    /// groupName_1.
    void AddData(UsdObjStream const &other);

    ////////////////////////////////////////////////////////////////////////
    // Verts

    /// Unconditionally add \a vert and return the new index.
    int AddVert(GfVec3f const &vert);

    /// Add a range of vertices.
    template <class Iter>
    void AddVerts(Iter begin, Iter end) {
        size_t oldSize = _verts.size();
        _verts.insert(_verts.end(), begin, end);
        _AddVertsInternal(_verts.begin() + oldSize);
    }

    /// Return a const reference to the verts in this data object.
    std::vector<GfVec3f> const &GetVerts() const;

    ////////////////////////////////////////////////////////////////////////
    // UVs

    /// Unconditionally add \a UV and return the new index.
    int AddUV(GfVec2f const &uv);

    /// Add a range of UVs.
    template <class Iter>
    void AddUVs(Iter begin, Iter end) {
        size_t oldSize = _uvs.size();
        _uvs.insert(_uvs.end(), begin, end);
        _AddUVsInternal(_uvs.begin() + oldSize);
    }

    /// Return a const reference to the UVs in this data object.
    std::vector<GfVec2f> const &GetUVs() const;

    ////////////////////////////////////////////////////////////////////////
    // Normals

    /// Unconditionally add \a normal and return the new index.
    int AddNormal(GfVec3f const &normal);

    /// Add a range of normals.
    template <class Iter>
    void AddNormals(Iter begin, Iter end) {
        size_t oldSize = _normals.size();
        _normals.insert(_normals.end(), begin, end);
        _AddNormalsInternal(_normals.begin() + oldSize);
    }

    /// Return a const reference to the normals in this data object.
    std::vector<GfVec3f> const &GetNormals() const;

    ////////////////////////////////////////////////////////////////////////
    // Points

    /// Add a single point.
    void AddPoint(Point const &point);

    /// Add a range of points.
    template <class Iter>
    void AddPoints(Iter begin, Iter end) {
        _points.insert(_points.end(), begin, end);
    }

    /// Return a const reference to the points in this data object.
    std::vector<Point> const &GetPoints() const;

    ////////////////////////////////////////////////////////////////////////
    // Groups

    /// Append \a group with \a name.  Return true if the group was successfully
    /// appended.  Do nothing and return false if there already exists a group
    /// with \a name in this data object.
    bool AddGroup(std::string const &name);

    /// Add \a face to the most recently appended group.  If no group has been
    /// appended, append one with an empty name.
    void AddFace(Face const &face);

    /// Find a group by name and return a pointer to it.  Return null if no such
    /// group exists.
    Group const *FindGroup(std::string const &name) const;

    /// Return all the groups in this data object in order.
    std::vector<Group> const &GetGroups() const;

    ////////////////////////////////////////////////////////////////////////
    // Comments

    /// Append a comment with \a text.  Prepend '#' to each line of \a text
    /// whose first non-whitespace character is not '#'.
    void AppendComments(std::string const &text);

    /// Prepend a comment with \a text.  Prepend '#' to each line of \a text
    /// whose first non-whitespace character is not '#'.
    void PrependComments(std::string const &text);

    /// Return the comments in this data object in order.
    std::vector<std::string> const &GetComments() const;

    ////////////////////////////////////////////////////////////////////////
    // Arbitrary Text

    /// Append arbitrary text.  Append any lines of \a text that are comments
    /// (first non-whitespace character is '#') as comments instead.
    void AppendArbitraryText(std::string const &text);

    /// Prepend arbitrary text.  Append any lines of \a text that are comments
    /// (first non-whitespace character is '#') as comments instead.
    void PrependArbitraryText(std::string const &text);

    /// Return all the arbitrary text in this data object in order.
    std::vector<std::string> const &GetArbitraryText() const;

    ////////////////////////////////////////////////////////////////////////
    // Sequence elements.
    std::vector<SequenceElem> const &GetSequence() const;

private:

    // Helper functions that add ranges of newly inserted elements.
    void _AddVertsInternal(std::vector<GfVec3f>::iterator begin);
    void _AddUVsInternal(std::vector<GfVec2f>::iterator begin);
    void _AddNormalsInternal(std::vector<GfVec3f>::iterator begin);

    // Helper functions to add sequence elements.
    void _AddSequence(SequenceElem::ElemType type, int repeat = 1);
    void _PrependSequence(SequenceElem::ElemType type, int repeat = 1);

    // Helper function to produce a unique group name.
    std::string _GetUniqueGroupName(std::string const &name) const;

    // Data members storing geometry.
    std::vector<GfVec3f> _verts;
    std::vector<GfVec2f> _uvs;
    std::vector<GfVec3f> _normals;
    std::vector<Point> _points;
    std::vector<std::string> _comments;
    std::vector<std::string> _arbitraryText;
    std::vector<Group> _groups;

    // Order of objects specified.
    std::vector<SequenceElem> _sequence;
};


#endif // USDOBJ_STREAM_H
