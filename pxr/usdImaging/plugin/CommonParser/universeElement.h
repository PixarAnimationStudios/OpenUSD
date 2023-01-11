//
// Copyright 2023 Pixar
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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_UNIVERSE_ELEMENT_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_UNIVERSE_ELEMENT_H

#include "pxr/pxr.h"
#include "globals.h"

PXR_NAMESPACE_OPEN_SCOPE
//CommonParserUniverse* BigBang();

#define MAX_PARSERS_IN_UNIVERSE 8

/// \class CommonParserUniverseElement
///
/// The universe implementation in CommonParser module
///
class CommonParserUniverseElement : public CommonParserUniverse
{
public:
    /// The default constructor.
    CommonParserUniverseElement();

    /// The destructor.
    ~CommonParserUniverseElement();

    /// Registers a Parser's Generator, used by the parsing module
    /// when introduced to the universe.
    CommonParserStatus Register(CommonParserGenerator*) override;

    /// Unregisters a Parser's Generator.
    CommonParserStatus Unregister(CommonParserGenerator*) override;

    /// How many parser/generators are registered?
    int RegisteredCount() override;

    /// Gets a parser generator (by position in registration list)
    /// to allow the application to begin a parsing operation.
    /// 0 <= iIndex < RegisteredCount();
    ///
    /// Note: the iIndex is NOT a key, as registration may change
    /// the order of Parser Generators... USE ONLY Name() to
    /// get a persistent key for any specific Parser Generator.
    CommonParserGenerator* GetGenerator(int iIndex) override;

    /// Same as above, but indexed off the CommonParserGenerator::Name()
    /// method.
    CommonParserGenerator* GetGenerator(const CommonParserStRange& sName) override;

private:
    // Cheap implementation, just a hard array.
    // Growing "array list" might be called upon in the future
    // but fortunately, the specifics are hidden behind the interface. ;-)
    CommonParserGenerator* _pRegistrants[MAX_PARSERS_IN_UNIVERSE];
    int _iCount;

    /// Find a generator using the name.
    CommonParserGenerator** _Find(const CommonParserStRange& sName);

    /// Find an empty generator.
    CommonParserGenerator** _FindEmpty();
};

class CommonParserUniverseWrapper
{
public:
    CommonParserUniverseWrapper();
    ~CommonParserUniverseWrapper();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_UNIVERSE_ELEMENT_H
