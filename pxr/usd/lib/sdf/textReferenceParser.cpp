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
///
/// \file sdf/textReferenceParser.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/textReferenceParser.h"
#include "pxr/base/tf/staticTokens.h"

#include <boost/range/iterator_range.hpp>
#include <boost/regex.hpp>

#include <fstream>
#include <sstream>

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_Tokens,
    (baseAsset)
    (payload)
    (references)
    (subLayers)
    );

namespace {
struct _InputLine {
    friend std::istream& operator>>(std::istream& istr, _InputLine& line) {
        std::getline(istr, line._data);
        return istr;
    }
    operator const string& () const { return _data; }
private:
    string _data;
};
}

static void
Sdf_ParseExternalReferences(
    const std::istream_iterator<_InputLine>& begin,
    const std::istream_iterator<_InputLine>& end,
    vector<string>* subLayers,
    vector<string>* references,
    vector<string>* payloads)
{
    string line, type;
    boost::match_results<string::const_iterator> what;

    // Matches and extracts the external reference type.
    boost::regex typeStart(
        "\\b(?:"
            "(baseAsset|payload|references|subLayers)|"
            "(asset|asset\\[\\])\\s+\\S+"
        ")\\s*=");

    // Matches a reference. These regexes are the asset regexes in the
    // text file format parser. Combining them into a single regex here
    // ensures we handle multiple references with different delimiters
    // in a single line properly. 
    //
    // Note that having the same asset path in an external reference
    // statement (with or without different delimiters) will result in
    // duplicates in the corresponding output vector.
    //
    // The order of the regexes is important, otherwise an asset path
    // like @@@foo.sdf@bar@@@ would match the shorter regex and cause
    // the "@bar" part of the path to be ignored.
    boost::regex assetRef(
        "@@@((([^[:cntrl:]@]|@{1,2}[^@]|\\\\@@@)+)?(@{0,2}))@@@"
        "|"
        "@([^[:cntrl:]@]+)?@"
        );

    // Sub-expressions 1 and 5 contain the matched asset paths.
    const int submatches[2] = { 1, 5 };

    for (auto lineIter = begin; lineIter != end; ++lineIter) {
        const string& line = *lineIter;
        // Look for an approximation of the most common kinds of comments, and
        // skip lines that match. This doesn't handle SLASHTERIX style
        // comments, and may also incorrectly identify lines as comments,
        // although this is not typically a problem.
        const size_t c = line.find_first_not_of(' ');
        if (c != string::npos &&
            ((line[c] == '"') ||
             (line[c] == '/') ||
             (line[c] == '#')))
            continue;

        if (boost::regex_search(line, what, typeStart))
            type = what[1].length() ? what[1] : what[2];

        // Skip baseAsset, as it refers to the current file.
        if (type == _Tokens->baseAsset)
            continue;

        boost::sregex_token_iterator it(
            line.begin(), line.end(), assetRef, submatches);
        boost::sregex_token_iterator end;
        for ( ; it != end ; ++it) {
            if (!it->length())
                continue;

            // If we extracted a type, put the path in the correct bucket. Put
            // asset path valued attributes in the reference bucket.
            if (type == _Tokens->subLayers)
                subLayers->push_back(*it);
            else if (type == _Tokens->payload) {
                payloads->push_back(*it);
                type.clear();
            }
            else
                references->push_back(*it);
        }

        if (!type.empty() && line.find(']') != string::npos)
            type.clear();
    }
}

void
SdfExtractExternalReferences(
    const string& filePath,
    vector<string>* subLayers,
    vector<string>* references,
    vector<string>* payloads)
{
    if (filePath.empty()) {
        TF_CODING_ERROR("Empty file path");
        return;
    }

    if (!subLayers) {
        TF_CODING_ERROR("Invalid subLayers pointer");
        return;
    }

    if (!references) {
        TF_CODING_ERROR("Invalid references pointer");
        return;
    }

    if (!payloads) {
        TF_CODING_ERROR("Invalid payloads pointer");
        return;
    }

    std::ifstream ifs(filePath.c_str());
    if (!ifs) {
        TF_RUNTIME_ERROR("Unable to open '%s' for reading.", filePath.c_str());
        return;
    }

    Sdf_ParseExternalReferences(
        std::istream_iterator<_InputLine>(ifs),
        std::istream_iterator<_InputLine>(),
        subLayers, references, payloads);
}

void
SdfExtractExternalReferencesFromString(
    const string& layerData,
    vector<string>* subLayers,
    vector<string>* references,
    vector<string>* payloads)
{
    if (!subLayers) {
        TF_CODING_ERROR("Invalid subLayers pointer");
        return;
    }

    if (!references) {
        TF_CODING_ERROR("Invalid references pointer");
        return;
    }

    if (!payloads) {
        TF_CODING_ERROR("Invalid payloads pointer");
        return;
    }

    std::istringstream istr(layerData);
    Sdf_ParseExternalReferences(
        std::istream_iterator<_InputLine>(istr),
        std::istream_iterator<_InputLine>(),
        subLayers, references, payloads);
}

PXR_NAMESPACE_CLOSE_SCOPE
