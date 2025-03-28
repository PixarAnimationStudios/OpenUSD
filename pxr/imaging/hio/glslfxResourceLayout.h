//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HIO_GLSLFX_RESOURCE_LAYOUT_H
#define PXR_IMAGING_HIO_GLSLFX_RESOURCE_LAYOUT_H

#include "pxr/pxr.h"

#include "pxr/imaging/hio/types.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/staticTokens.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


#define HIO_GLSLFX_RESOURCE_LAYOUT_TOKENS       \
    (unknown)                                   \
    (block)                                     \
    ((inValue, "in"))                           \
    ((outValue, "out"))                         \
    ((inBlock, "in block"))                     \
    ((outBlock, "out block"))                   \
    ((inValueArray, "in array"))                \
    ((outValueArray, "out array"))              \
    ((inBlockArray, "in block array"))          \
    ((outBlockArray, "out block array"))        \
    ((uniformBlock, "uniform block"))           \
    ((bufferReadOnly, "buffer readOnly"))       \
    ((bufferReadWrite, "buffer readWrite"))     \
    (centroid)                                  \
    (sample)                                    \
    (smooth)                                    \
    (flat)                                      \
    (noperspective)


TF_DECLARE_PUBLIC_TOKENS(HioGlslfxResourceLayoutTokens, HIO_API,
                         HIO_GLSLFX_RESOURCE_LAYOUT_TOKENS);

class VtDictionary;

/// \class HioGlslfxResourceLayout
/// The resource layout for stages in a shader pipeline.
///
/// The main geometric shader pipelines for meshes, curves, points,
/// volumes, as well as compute shaders for subdivision refinement,
/// etc. are expressed as GLSL source code and aspects of the main
/// shader pipeline that are generated at runtime also are expressed
/// as GLSL source code.
///
/// This class provides an intermediate representation for shader
/// resources that are needed to provide access to external data
/// like buffers and textures and also interstage data like input
/// and output variables and input and output interface blocks.
///
/// A method is provided to parse resource data from HioGlslfx
/// resource layout dictionaries so that resource layout definitions
/// can continue to be authored alongside related GLSL shader source.
///
/// The dictionary layouts have been designed to match the concepts
/// and syntax used by GLSL.
///
class HioGlslfxResourceLayout
{
public:
    /// Specifies whether a resource element is a shader input,
    /// a shader output (i.e. an input or output variable or input
    /// or output interface block), or neither (i.e. a buffer or texture).
    enum class InOut {
        NONE,
        STAGE_IN,
        STAGE_OUT,
    };

    /// Specifies the kind of resource element.
    enum class Kind {
        NONE,
        VALUE,
        BLOCK,
        QUALIFIER,
        UNIFORM_VALUE,
        UNIFORM_BLOCK,
        UNIFORM_BLOCK_CONSTANT_PARAMS,
        BUFFER_READ_ONLY,
        BUFFER_READ_WRITE,
    };

    /// Specifies a member of an aggregate resource element.
    struct Member {
        Member(TfToken const & dataType,
               TfToken const & name,
               TfToken const & arraySize = TfToken(),
               TfToken const & qualifiers = TfToken())
            : dataType(dataType)
            , name(name)
            , arraySize(arraySize)
            { }
        TfToken dataType;
        TfToken name;
        TfToken arraySize;
        TfToken qualifiers;
    };
    using MemberVector = std::vector<Member>;

    /// Specifies a resource element.
    struct Element {
        Element(InOut inOut = InOut::NONE,
                Kind kind = Kind::NONE,
                TfToken dataType = HioGlslfxResourceLayoutTokens->unknown,
                TfToken name = HioGlslfxResourceLayoutTokens->unknown,
                TfToken arraySize = TfToken(),
                TfToken qualifiers = TfToken())
            : inOut(inOut)
            , kind(kind)
            , location(-1)
            , dataType(dataType)
            , name(name)
            , qualifiers(qualifiers)
            , arraySize(arraySize)
            , aggregateName()
            , members()
            { }
        InOut inOut;
        Kind kind;
        int location;
        TfToken dataType;
        TfToken name;
        TfToken qualifiers;
        TfToken arraySize;
        TfToken aggregateName;
        MemberVector members;
    };
    using ElementVector = std::vector<Element>;

    /// Specifies the type of a texture element.
    enum class TextureType {
        TEXTURE,                // a texture
        SHADOW_TEXTURE,         // a texture used as a shadow
        ARRAY_TEXTURE,          // e.g. texture1DArray, texture2DArray, etc.
    };

    /// Specifies a texture element.
    struct TextureElement {
        TextureElement(TfToken name,
                int dim,
                int bindingIndex,
                HioFormat format = HioFormatFloat32Vec4,
                TextureType textureType = TextureType::TEXTURE,
                int arraySize = 0)
            : name(name)
            , dim(dim)
            , bindingIndex(bindingIndex)
            , format(format)
            , textureType(textureType)
            , arraySize(arraySize)
            { }
        TfToken name;
        int dim;
        int bindingIndex;
        HioFormat format;
        TextureType textureType;
        int arraySize;
    };
    using TextureElementVector = std::vector<TextureElement>;

    HioGlslfxResourceLayout();
    ~HioGlslfxResourceLayout();

    /// Parses GLSLFX resource layout elements from the specified
    /// \a layoutDict and appends the parsed elements to \a result.
    HIO_API
    static void ParseLayout(
        ElementVector *result,
        TfToken const &shaderStage,
        VtDictionary const &layoutDict);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HIO_GLSLFX_RESOURCE_LAYOUT_H
