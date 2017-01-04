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
#include "pxr/usd/sdf/parserValueContext.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/usd/sdf/fileIO_Common.h"

struct Sdf_ToStringVisitor : boost::static_visitor<std::string>
{
    template <typename T>
    std::string operator () (const T &value)
    {
        return TfStringify(value);
    }

    std::string operator () (const std::string &value)
    {
        return Sdf_FileIOUtility::Quote(value);
    }
};

static void ReportCodingError(const std::string &text)
{
    TF_CODING_ERROR(text);
}

Sdf_ParserValueContext::Sdf_ParserValueContext() :
    valueTypeIsValid(false),
    valueIsShaped(false),
    errorReporter(ReportCodingError)
{
    Clear();
}

bool
Sdf_ParserValueContext::SetupFactory(const std::string &typeName)
{
    if (typeName != lastTypeName) {
        const Sdf_ParserHelpers::ValueFactory &factory =
            Sdf_ParserHelpers::GetValueFactoryForMenvaName(
                typeName, &valueTypeIsValid);

        valueTypeName = typeName;

        if (!valueTypeIsValid) {
            valueFunc = Sdf_ParserHelpers::ValueFactoryFunc();
            valueIsShaped = false;
            valueTupleDimensions = SdfTupleDimensions();
        }
        else {
            valueFunc = factory.func;
            valueIsShaped = factory.isShaped;
            valueTupleDimensions = factory.dimensions;
        }

        lastTypeName = typeName;
    }

    return valueTypeIsValid;
}

VtValue
Sdf_ParserValueContext::ProduceValue(std::string *errStrPtr)
{
    VtValue ret;
    if (_isRecordingString) {
        ret = SdfUnregisteredValue(GetRecordedString());
    }
    else {
        if (!valueFunc) {
            // CODE_COVERAGE_OFF
            // We will already have detected a bad typename as we tried to
            // create the attribute for this value.  We should not hit this
            // here.
            errorReporter(TfStringPrintf("Unrecognized type name '%s'",
                                         valueTypeName.c_str()).c_str());
            return VtValue();
            // CODE_COVERAGE_ON
        }

        size_t index = 0;
        ret = valueFunc(shape, vars, index, errStrPtr);
    }

    Clear();
    return ret;
}

void
Sdf_ParserValueContext::Clear()
{
    dim = 0;
    pushDim = -1;
    shape.clear();

    // Every time we parse a value, we call ProduceValue() which, in turn,
    // calls Clear().
    //
    // Note that we're NOT resetting the following variables here:
    // 
    // valueTypeName 
    // valueTypeIsValid
    // valueFunc
    // valueIsShaped
    // valueTupleDimensions
    //
    // This is because we often parse several values in a row 
    // (e.g. AnimSpline keyframes), and we don't want the extra overhead of
    // resetting the above variables just so that we can set them again 
    // before parsing the next value. Instead, whenever we parse a new 
    // attribute type, we call SetupFactory() which caches these variables.
    // This allows us to skip over them here.

    tupleDepth = 0;
    vars.clear();
    workingShape.clear();

    _isRecordingString = false;
    _needComma = false;
}

void
Sdf_ParserValueContext::AppendValue(const Value& value)
{
    if (_isRecordingString) {
        if (_needComma) {
            _recordedString += ", ";
        }
        Sdf_ToStringVisitor v;
        _recordedString += value.ApplyVisitor(v);
        _needComma = true;
    }
    else {
        vars.push_back(value);
    }

    if (pushDim == -1) {
        pushDim = dim;
    } else {
        if (pushDim != dim) {
            errorReporter("Non-square shaped value");
            return;
        }
    }

    // if inside a list (dim>0) and not inside a tuple (tupleDepth==0)
    if (tupleDepth == 0 && dim)
        workingShape[dim-1] += 1;

    // If we're at the deepest level of the tuple, keep track of the number of
    // elements added along the current dimension so that EndTuple() can
    // validate the completed tuple dimensions with the correct tuple
    // dimensions from the factory.
    if (tupleDepth && static_cast<size_t>(tupleDepth) == valueTupleDimensions.size) {
        --tupleDimensions.d[tupleDepth-1];
    }
}

void
Sdf_ParserValueContext::BeginList()
{
    if (_isRecordingString) {
        if (_needComma) {
            _needComma = false;
            _recordedString += ", ";
        }
        _recordedString += '[';
    }

    // Dim starts at 1, so the current shape index is dim - 1.
    ++dim;
    // Check if the shape is big enough for dim values
    if (static_cast<size_t>(dim) > shape.size()) {
        shape.push_back(0);
        workingShape.push_back(0);
    }
}

void
Sdf_ParserValueContext::EndList()
{
    if (_isRecordingString) {
        _recordedString += ']';
        _needComma = true;
    }

    if (dim == 0) {
        // CODE_COVERAGE_OFF
        // This can't happen unless there's a bug in the parser
        errorReporter("Mismatched [ ] in shaped value");
        return;
        // CODE_COVERAGE_ON
    }
    if(shape[dim-1] == 0) {
        // This is the first time we've completed a run in this
        // dimension, so store the size of this dimension into
        // our discovered shape vector.
        shape[dim-1] =
            workingShape[dim-1];
        if (shape[dim-1] == 0) {
            // CODE_COVERAGE_OFF
            // This can't happen unless there's a bug in the parser
            errorReporter("Shaped value with a zero dimension");
            return;
            // CODE_COVERAGE_ON
        }
    } else {
        // We've seen a run in this dimension before, so check
        // that the size is the same as before.
        if (shape[dim-1] !=
            workingShape[dim-1]) {
            errorReporter("Non-square shaped value");
            return;
        }
    }

    // XXX Dim should always be valid here due to check above?
    if (dim) {
        // Reset our counter for the dimension we just finished
        // parsing
        workingShape[dim-1] = 0;
        --dim;
        // And increment the tally for the containing dimension.
        if (dim > 0)
            ++workingShape[dim-1];
    }
}

void
Sdf_ParserValueContext::BeginTuple()
{
    if (_isRecordingString) {
        if (_needComma) {
            _needComma = false;
            _recordedString += ", ";
        }
        _recordedString += '(';
    }

    if (static_cast<size_t>(tupleDepth) >= valueTupleDimensions.size) {
        errorReporter(TfStringPrintf("Tuple nesting too deep! Should not be "
            "deeper than %d for attribute of type %s.",
            tupleDepth, valueTypeName.c_str()));
        return;
    }
    tupleDimensions.d[tupleDepth] =
        valueTupleDimensions.d[tupleDepth];

    ++tupleDepth;
}

void
Sdf_ParserValueContext::EndTuple()
{
    if (_isRecordingString) {
        _recordedString += ')';
        _needComma = true;
    }

    if (tupleDepth == 0) {
        // CODE_COVERAGE_OFF
        // This can't happen unless there's a bug in the parser
        errorReporter(TfStringPrintf("Mismatched ( ) for attribute "
                                     "of type %s.", valueTypeName.c_str()));
        return;
        // CODE_COVERAGE_ON
    }

    --tupleDepth;

    if (tupleDimensions.d[tupleDepth] != 0) {
        errorReporter(TfStringPrintf("Tuple dimensions error for "
            "attribute of type %s.",
            valueTypeName.c_str()));
        return;
    }
    if (tupleDepth > 0) {
        --tupleDimensions.d[tupleDepth-1];
    }
    // If we're working on a shaped type and we popped out of a tuple,
    // add another element to the working shape here.
    if (tupleDepth == 0 && dim)
        ++workingShape[dim-1];
}

void
Sdf_ParserValueContext::StartRecordingString()
{
    _needComma = false;
    _isRecordingString = true;
    _recordedString.clear();
}

void 
Sdf_ParserValueContext::StopRecordingString()
{
    _isRecordingString = false;
}

bool 
Sdf_ParserValueContext::IsRecordingString() const
{
    return _isRecordingString;
}

std::string
Sdf_ParserValueContext::GetRecordedString() const
{
    return _recordedString;
}

void
Sdf_ParserValueContext::SetRecordedString(const std::string &text)
{
    _recordedString = text;
}
