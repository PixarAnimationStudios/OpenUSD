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
#include "streamIO.h"
#include "stream.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iterator.hpp>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

using std::string;
using std::vector;


bool
UsdObjReadDataFromFile(std::string const &fileName,
                      UsdObjStream *stream, std::string *error)
{
    // try and open the file
    std::ifstream fin(fileName.c_str());
    if (!fin.is_open()) {
        if (error) {
            *error = TfStringPrintf("Could not open file: (%s)\n",
                                    fileName.c_str());
        }
        return false;
    }

    return UsdObjReadDataFromStream(fin, stream, error);
}

static UsdObjStream::Point
_ParsePoint(string const &str)
{
    using boost::iterator_range;
    using boost::make_iterator_range;
    using boost::lexical_cast;

    UsdObjStream::Point result;

    // Break the string up into up to 3 segments separated by slashes.
    iterator_range<char const *> ranges[3];
    int numRanges = 0;
    char const *from = str.data(),
        *pos = str.data(), *end = str.data() + strlen(str.data());
    while (numRanges != 3) {
        pos = std::find(from, end, '/');
        ranges[numRanges++] = make_iterator_range(from, pos);
        if (pos == end)
            break;
        from = pos + 1;
    }

    // Now pull indexes out of the segments.  Subtract one since indexes are
    // 1-based in the file, but we store them 0-based in the data structure.
    int *indexes[] = {&result.vertIndex, &result.uvIndex, &result.normalIndex};
    for (int i = 0; i != numRanges; ++i) {
        if (not ranges[i].empty())
            *indexes[i] = lexical_cast<int>(ranges[i]) - 1;
    }

    return result;
}

bool
UsdObjReadDataFromStream(std::istream &input,
                         UsdObjStream *stream, std::string *error)
{
    string line;
    std::istringstream lineStream;

    UsdObjStream::Group curGroup;

    while (getline(input, line)) {
        if (line.empty())
            continue;

        // check the first character for the type
        lineStream.clear();
        lineStream.str(line);

        string type;
        lineStream >> type;

        if (type == "v") {
            GfVec3f newVert;
            lineStream >> newVert[0] >> newVert[1] >> newVert[2];
            stream->AddVert(newVert);
        } else if (type == "vt") {
            GfVec2f newUV;
            lineStream >> newUV[0] >> newUV[1];
            stream->AddUV(newUV);
        } else if (type == "vn") {
            GfVec3f newNormal;
            lineStream >> newNormal[0] >> newNormal[1] >> newNormal[2];
            stream->AddNormal(newNormal);
        } else if (type == "f") {
            int pointsBegin = int(stream->GetPoints().size()), pointsEnd;
            std::string indexStr;
            while (lineStream >> indexStr)
                stream->AddPoint(_ParsePoint(indexStr));

            pointsEnd = int(stream->GetPoints().size());
            stream->AddFace(UsdObjStream::Face(pointsBegin, pointsEnd));
        } else if (type == "g") {
            // Create new group.
            string groupName;
            lineStream >> groupName;
            stream->AddGroup(groupName);
        } else {
            // Add arbitrary text (or comment).
            stream->AppendArbitraryText(line);
        }
    }

    return true;
}
