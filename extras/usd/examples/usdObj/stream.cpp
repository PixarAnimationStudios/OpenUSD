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
#include "stream.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

#include <map>
#include <cstdio>

using std::string;
using std::vector;
using std::map;

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(UsdObjStream::SequenceElem::Verts);
    TF_ADD_ENUM_NAME(UsdObjStream::SequenceElem::UVs);
    TF_ADD_ENUM_NAME(UsdObjStream::SequenceElem::Normals);
    TF_ADD_ENUM_NAME(UsdObjStream::SequenceElem::Groups);
    TF_ADD_ENUM_NAME(UsdObjStream::SequenceElem::Comments);
    TF_ADD_ENUM_NAME(UsdObjStream::SequenceElem::ArbitraryText);
}

UsdObjStream::Face::Face()
    : pointsBegin(0)
    , pointsEnd(0)
{
}

bool operator==(UsdObjStream::Face const &lhs, UsdObjStream::Face const &rhs)
{
    return lhs.pointsBegin == rhs.pointsBegin and
        lhs.pointsEnd == rhs.pointsEnd;
}

bool operator!=(UsdObjStream::Face const &lhs, UsdObjStream::Face const &rhs)
{
    return not (lhs == rhs);
}


UsdObjStream::UsdObjStream()
{
}

////////////////////////////////////////////////////////////////////////
// Verts
int
UsdObjStream::AddVert(GfVec3f const &vert)
{
    _verts.push_back(vert);
    int index = _verts.size()-1;
    _AddVertsInternal(_verts.begin() + index);
    return index;
}

void
UsdObjStream::_AddVertsInternal(vector<GfVec3f>::iterator begin)
{
    _AddSequence(SequenceElem::Verts, std::distance(begin, _verts.end()));
}

vector<GfVec3f> const &
UsdObjStream::GetVerts() const
{
    return _verts;
}

////////////////////////////////////////////////////////////////////////
// UVs
int
UsdObjStream::AddUV(GfVec2f const &uv)
{
    _uvs.push_back(uv);
    int index = _uvs.size()-1;
    _AddUVsInternal(_uvs.begin() + index);
    return index;
}

void
UsdObjStream::_AddUVsInternal(vector<GfVec2f>::iterator begin)
{
    _AddSequence(SequenceElem::UVs, std::distance(begin, _uvs.end()));
}

vector<GfVec2f> const &
UsdObjStream::GetUVs() const {
    return _uvs;
}

////////////////////////////////////////////////////////////////////////
// Normals
int
UsdObjStream::AddNormal(GfVec3f const &normal)
{
    _normals.push_back(normal);
    int index = _normals.size()-1;
    _AddNormalsInternal(_normals.begin() + index);
    return index;
}

void
UsdObjStream::_AddNormalsInternal(vector<GfVec3f>::iterator begin)
{
    _AddSequence(SequenceElem::Normals, std::distance(begin, _normals.end()));
}

vector<GfVec3f> const &
UsdObjStream::GetNormals() const
{
    return _normals;
}

////////////////////////////////////////////////////////////////////////
// Points
void
UsdObjStream::AddPoint(Point const &point)
{
    _points.push_back(point);
}

std::vector<UsdObjStream::Point> const &
UsdObjStream::GetPoints() const
{
    return _points;
}


bool
UsdObjStream::AddGroup(string const &name)
{
    if (not FindGroup(name)) {
        Group g;
        g.name = name;
        _groups.push_back(g);
        _AddSequence(SequenceElem::Groups);
        return true;
    }
    return false;
}

UsdObjStream::Group const *
UsdObjStream::FindGroup(std::string const &name) const
{
    for (vector<Group>::const_iterator i = _groups.begin(), end = _groups.end();
         i != end; ++i) {
        if (i->name == name)
            return &(*i);
    }
    return 0;
}

std::vector<UsdObjStream::Group> const &
UsdObjStream::GetGroups() const
{
    return _groups;
}

void
UsdObjStream::AddFace(Face const &face)
{
    // If there aren't any groups, add one first.
    if (_groups.empty()) {
        AddGroup(string());
    }
    _groups.back().faces.push_back(face);
}

static bool
_IsComment(string const &text)
{
    return text.find('#') < text.find_first_not_of(" \t#");
}

static string
_MakeComment(string const &text)
{
    return _IsComment(text) ? text : ("# " + text);
}

void
UsdObjStream::AppendComments(std::string const &text)
{
    vector<string> lines = TfStringSplit(text, "\n");
    for (const auto& line : lines) {
        _comments.push_back(_MakeComment(line));
    }
    _AddSequence(SequenceElem::Comments, lines.size());
}

void
UsdObjStream::PrependComments(string const &text)
{
    vector<string> lines = TfStringSplit(text, "\n");
    // Mutate all the lines into comments.
    for (const auto& line : lines) {
        line = _MakeComment(line);
    }
    // Insert them at the beginning.
    _comments.insert(_comments.begin(), lines.begin(), lines.end());
    _PrependSequence(SequenceElem::Comments, lines.size());
}

vector<string> const &
UsdObjStream::GetComments() const
{
    return _comments;
}

void
UsdObjStream::AppendArbitraryText(std::string const &text)
{
    vector<string> lines = TfStringSplit(text, "\n");
    for (const auto& line : lines) {
        if (_IsComment(line)) {
            AppendComments(line);
        } else {
            _arbitraryText.push_back(line);
            _AddSequence(SequenceElem::ArbitraryText);
        }
    }
}

void
UsdObjStream::PrependArbitraryText(string const &text)
{
    vector<string> lines = TfStringSplit(text, "\n");
    for (auto lineIter = lines.rbegin(); lineIter != lines.rend(); ++lineIter){
        const auto& line = *lineIter;
        if (_IsComment(line)) {
            PrependComments(line);
        } else {
            _arbitraryText.insert(_arbitraryText.begin(), line);
            _PrependSequence(SequenceElem::ArbitraryText);
        }
    }
}

vector<string> const &
UsdObjStream::GetArbitraryText() const
{
    return _arbitraryText;
}

std::vector<UsdObjStream::SequenceElem> const &
UsdObjStream::GetSequence() const
{
    return _sequence;
}

void
UsdObjStream::_AddSequence(SequenceElem::ElemType type, int repeat)
{
    // Check to see if we can add to an existing sequence, otherwise add a new
    // sequence element.
    if (not _sequence.empty() and _sequence.back().type == type) {
        _sequence.back().repeat += repeat;
    } else {
        _sequence.push_back(SequenceElem(type, repeat));
    }
}

void
UsdObjStream::_PrependSequence(SequenceElem::ElemType type, int repeat)
{
    // Check to see if we can add to an existing sequence, otherwise add a new
    // sequence element.
    if (not _sequence.empty() and _sequence.front().type == type) {
        _sequence.front().repeat += repeat;
    } else {
        _sequence.insert(_sequence.begin(), SequenceElem(type, repeat));
    }
}

void
UsdObjStream::clear()
{
    UsdObjStream().swap(*this);
}

void
UsdObjStream::swap(UsdObjStream &other)
{
    _verts.swap(other._verts);
    _uvs.swap(other._uvs);
    _normals.swap(other._normals);
    _points.swap(other._points);
    _comments.swap(other._comments);
    _arbitraryText.swap(other._arbitraryText);
    _groups.swap(other._groups);

    _sequence.swap(other._sequence);
}

string
UsdObjStream::_GetUniqueGroupName(string const &name) const
{
    string curName = name;
    int serial = 0;
    while (FindGroup(curName))
        curName = TfStringPrintf("%s_%d", name.c_str(), ++serial);
    return curName;
}

static UsdObjStream::Point
OffsetPoint(UsdObjStream::Point p, UsdObjStream::Point offset)
{
    p.vertIndex += p.vertIndex == -1 ? 0 : offset.vertIndex;
    p.uvIndex += p.uvIndex == -1 ? 0 : offset.uvIndex;
    p.normalIndex += p.normalIndex == -1 ? 0 : offset.normalIndex;
    return p;
}

void
UsdObjStream::AddData(UsdObjStream const &other)
{

    UsdObjStream::Point offset(
        GetVerts().size(), GetUVs().size(), GetNormals().size());

    int pointsOffset = GetPoints().size();

    vector<GfVec3f>::const_iterator vertIter = other.GetVerts().begin();
    vector<GfVec2f>::const_iterator uvIter = other.GetUVs().begin();
    vector<GfVec3f>::const_iterator normalIter = other.GetNormals().begin();
    vector<Group>::const_iterator groupIter = other.GetGroups().begin();
    vector<string>::const_iterator commentsIter = other.GetComments().begin();
    vector<string>::const_iterator arbitraryTextIter =
        other.GetArbitraryText().begin();
    vector<Point> const &points = other.GetPoints();

    // Add elements from the other data in sequence.
    for (const auto& elem : other.GetSequence()) {
        switch (elem.type) {
        default:
            TF_CODING_ERROR("Unknown sequence element '%s', aborting",
                            TfStringify(elem.type).c_str());
            return;
        case SequenceElem::Verts:
            for (int i = 0; i != elem.repeat; ++i)
                AddVert(*vertIter++);
            break;
        case SequenceElem::UVs:
            for (int i = 0; i != elem.repeat; ++i)
                AddUV(*uvIter++);
            break;
        case SequenceElem::Normals:
            for (int i = 0; i != elem.repeat; ++i)
                AddNormal(*normalIter++);
            break;
        case SequenceElem::Groups:
            for (int i = 0; i != elem.repeat; ++i, ++groupIter) {
                AddGroup(_GetUniqueGroupName(groupIter->name));
                for (const auto& face : groupIter->faces) {
                    for (int j = face.pointsBegin; j != face.pointsEnd; ++j)
                        AddPoint(OffsetPoint(points[j], offset));

                    AddFace(Face(face.pointsBegin + pointsOffset,
                                 face.pointsEnd + pointsOffset));
                }
            }
            break;
        case SequenceElem::Comments:
            for (int i = 0; i != elem.repeat; ++i)
                AppendComments(*commentsIter++);
            break;
        case SequenceElem::ArbitraryText:
            for (int i = 0; i != elem.repeat; ++i)
                AppendArbitraryText(*arbitraryTextIter++);
            break;
        }

    }
}
