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
#ifndef SDF_PARSERVALUECONTEXT_H
#define SDF_PARSERVALUECONTEXT_H

#include "pxr/usd/sdf/parserHelpers.h"
#include "pxr/base/vt/array.h"

#include <string>
#include <vector>

// Parses nested arrays of atomic values or tuples of atomic values. Validity
// checks are done while parsing to make sure arrays are "square" and tuples
// are all the same size. Each atomic value (number or string) is accumulated
// during parsing and all atomic values are used to produce a VtValue after
// parsing has finished. Example usage:
//
// SetupFactory("Point[]");
// BeginList();
//     BeginTuple();
//         AppendValue(1);
//         AppendValue(2);
//         AppendValue(3);
//     EndTuple();
//     BeginTuple();
//         AppendValue(2);
//         AppendValue(3);
//         AppendValue(4);
//     EndTuple();
// EndList();
// ProduceValue() == VtArray<Vec3d> { Vec3d(1, 2, 3), Vec3d(2, 3, 4) };
//
// Value factories are retrieved with GetValueFactoryForMenvaName(), which uses
// preprocessor-generated factories from _SDF_VALUE_TYPES.
//
// Ideally this would be self-contained, but the parser currently accesses lots
// of public member variables.
class Sdf_ParserValueContext {
public:
    typedef Sdf_ParserHelpers::Value Value;
    typedef boost::function<void (const std::string &)> ErrorReporter;

    Sdf_ParserValueContext();

    // Sets up this context to produce a value with C++ type determined by
    // the given \p typeName. 
    // 
    // Returns true if the given type is valid and recognized, false
    // otherwise. If false is returned, the context will be unable to
    // produce a value for this type.
    bool SetupFactory(const std::string &typeName);

    // Make a shaped value from parsed context.
    VtValue ProduceValue(std::string *errStrPtr);

    void Clear();

    void AppendValue(const Value& value);

    // Called before each list, corresponds to the '[' token
    void BeginList();

    // Called after each list, corresponds to the ']' token
    void EndList();

    // Called before each tuple, corresponds to the '(' token
    void BeginTuple();

    // Called after each tuple, corresponds to the ')' token
    void EndTuple();

    int dim;
    std::vector<unsigned int> shape;
    int tupleDepth;
    SdfTupleDimensions tupleDimensions;
    std::vector<Value> vars;
    std::vector<unsigned int> workingShape;
    
    // The recorded dim at which we got our first AppendValue.
    // If we get subsequent pushes where dim != pushDim, it is an error
    // (eg [1, 2, [3, 4]]).  Initially it is -1 to indicate we have never
    // appended anything.
    int pushDim;

    // The cached value factory information.
    std::string valueTypeName;
    bool valueTypeIsValid;
    std::string lastTypeName;
    Sdf_ParserHelpers::ValueFactoryFunc valueFunc;
    bool valueIsShaped;
    SdfTupleDimensions valueTupleDimensions;

    // A function to report textual errors as they are encountered. This is set
    // to a function that calls TF_CODING_ERROR() by default, but is customizable
    // so the parser can report parse errors instead.
    ErrorReporter errorReporter;

    // To record a textual representation of the parsed value, call
    // StartRecordingString() before parsing begins and GetRecordedString()
    // after parsing ends. The string will continue to be accumulated until
    // Clear() is called (ProduceValue() calls Clear() automatically).
    void StartRecordingString();
    void StopRecordingString();

    bool IsRecordingString() const;
    std::string GetRecordedString() const;

    // Hook to override the recorded text
    void SetRecordedString(const std::string &text);

private:
    bool _needComma, _isRecordingString;
    std::string _recordedString;
};

#endif
