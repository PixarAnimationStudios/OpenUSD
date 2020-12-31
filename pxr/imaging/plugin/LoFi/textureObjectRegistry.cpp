//
// Copyright 2020 benmalarter
//
// Unlicensed
//

#include "pxr/imaging/plugin/LoFi/textureObjectRegistry.h"

#include "pxr/imaging/plugin/LoFi/ptexTextureObject.h"
#include "pxr/imaging/plugin/LoFi/textureObject.h"
#include "pxr/imaging/plugin/LoFi/udimTextureObject.h"
#include "pxr/imaging/plugin/LoFi/dynamicUvTextureObject.h"
#include "pxr/imaging/plugin/LoFi/subtextureIdentifier.h"
#include "pxr/imaging/plugin/LoFi/textureIdentifier.h"
#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

LoFiTextureObjectRegistry::LoFiTextureObjectRegistry(
    LoFiResourceRegistry * registry)
  : _totalTextureMemory(0)
  , _resourceRegistry(registry)
{
}

LoFiTextureObjectRegistry::~LoFiTextureObjectRegistry() = default;

bool
static
_IsDynamic(const LoFiTextureIdentifier &textureId)
{
    return
        dynamic_cast<const LoFiDynamicUvSubtextureIdentifier*>(
            textureId.GetSubtextureIdentifier());
}

LoFiTextureObjectSharedPtr
LoFiTextureObjectRegistry::_MakeTextureObject(
    const LoFiTextureIdentifier &textureId,
    const HdTextureType textureType)
{
    switch(textureType) {
    case HdTextureType::Uv:
        if (_IsDynamic(textureId)) {
            return
                std::make_shared<LoFiDynamicUvTextureObject>(textureId, this);
        } else {
            return
                std::make_shared<LoFiAssetUvTextureObject>(textureId, this);
        }
    case HdTextureType::Field:
        return std::make_shared<LoFiFieldTextureObject>(textureId, this);
    case HdTextureType::Ptex:
        return std::make_shared<LoFiPtexTextureObject>(textureId, this);
    case HdTextureType::Udim:
        return std::make_shared<LoFiUdimTextureObject>(textureId, this);
    }

    TF_CODING_ERROR(
        "Texture type not supported by texture object registry.");
    return nullptr;
}

LoFiTextureObjectSharedPtr
LoFiTextureObjectRegistry::AllocateTextureObject(
    const LoFiTextureIdentifier &textureId,
    const HdTextureType textureType)
{
    // Check with instance registry and allocate texture and sampler object
    // if first object.
    HdInstance<LoFiTextureObjectSharedPtr> inst =
        _textureObjectRegistry.GetInstance(TfHash()(textureId));

    if (inst.IsFirstInstance()) {
        LoFiTextureObjectSharedPtr const texture = _MakeTextureObject(
            textureId, textureType);

        inst.SetValue(texture);
        _dirtyTextures.push_back(texture);
        // Note that this is already protected by the lock that inst
        // holds for the _textureObjectRegistry.
        _filePathToTextureObjects[textureId.GetFilePath()].push_back(texture);
    }

    return inst.GetValue();
}

void
LoFiTextureObjectRegistry::MarkTextureFilePathDirty(
    const TfToken &filePath)
{
    _dirtyFilePaths.push_back(filePath);
}

void
LoFiTextureObjectRegistry::MarkTextureObjectDirty(
    LoFiTextureObjectPtr const &texture)
{
    _dirtyTextures.push_back(texture);
}

void
LoFiTextureObjectRegistry::AdjustTotalTextureMemory(const int64_t memDiff)
{
    _totalTextureMemory.fetch_add(memDiff);
}

// Turn a vector into a set, dropping expired weak points.
template<typename T, typename U>
static
void
_Uniquify(const T &objects,
          std::set<std::shared_ptr<U>> *result)
{
    // Creating a std:set might be expensive.
    //
    // Alternatives include an unordered set or a timestamp
    // mechanism, i.e., the registry stores an integer that gets
    // increased on each commit and each texture object stores an
    // integer which gets updated when a texture object is processed
    // during commit so that it can be checked whether a texture
    // object has been already processed when it gets encoutered for
    // the second time in the _dirtyTextures vector.

    TRACE_FUNCTION();
    for (std::weak_ptr<U> const &objectPtr : objects) {
        if (std::shared_ptr<U> const object = objectPtr.lock()) {
            result->insert(object);
        }
    }
}

// Variable left from a time when Hio_StbImage was not thread-safe
// and testUsdImagingGLTextureWrapStormTextureSystem produced
// wrong and non-deterministic results.
//
static const bool _isGlfBaseTextureDataThreadSafe = true;

std::set<LoFiTextureObjectSharedPtr>
LoFiTextureObjectRegistry::Commit()
{
    TRACE_FUNCTION();

    std::set<LoFiTextureObjectSharedPtr> result;

    // Record all textures as dirty corresponding to file paths
    // explicitly marked dirty by client.
    for (const TfToken &dirtyFilePath : _dirtyFilePaths) {
        const auto it = _filePathToTextureObjects.find(dirtyFilePath);
        if (it != _filePathToTextureObjects.end()) {
            _Uniquify(it->second, &result);
        }
    }

    // Also record all textures explicitly marked dirty.
    _Uniquify(_dirtyTextures, &result);

    {
        TRACE_FUNCTION_SCOPE("Loading textures");
        HF_TRACE_FUNCTION_SCOPE("Loading textures");

        if (_isGlfBaseTextureDataThreadSafe) {
            // Loading a texture file of a previously unseen type might
            // require loading a new plugin, so give up the GIL temporarily
            // to the threads loading the images.
            TF_PY_ALLOW_THREADS_IN_SCOPE();

            // Parallel load texture files
            WorkParallelForEach(result.begin(), result.end(),
                                [](const LoFiTextureObjectSharedPtr &texture) {
                                    texture->_Load(); });
        } else {
            for (const LoFiTextureObjectSharedPtr &texture : result) {
                texture->_Load();
            }
        }
    }

    {
        TRACE_FUNCTION_SCOPE("Commiting textures");
        HF_TRACE_FUNCTION_SCOPE("Committing textures");

        // Commit loaded files to GPU.
        for (const LoFiTextureObjectSharedPtr &texture : result) {
            texture->_Commit();
        }
    }

    _dirtyFilePaths.clear();
    _dirtyTextures.clear();

    return result;
}

// Remove all expired weak pointers from vector, return true
// if no weak pointers left.
static
bool
_GarbageCollect(LoFiTextureObjectPtrVector *const vec) {
    // Go from left to right, filling slots that became empty
    // with valid weak pointers from the right.
    size_t last = vec->size();

    for (size_t i = 0; i < last; i++) {
        if ((*vec)[i].expired()) {
            while(true) {
                last--;
                if (i == last) {
                    break;
                }
                if (!(*vec)[last].expired()) {
                    (*vec)[i] = (*vec)[last];
                    break;
                }
            }
        }
    }
    
    vec->resize(last);
    
    return last == 0;
}

static
void _GarbageCollect(
    std::unordered_map<TfToken, LoFiTextureObjectPtrVector,
                       TfToken::HashFunctor> *const filePathToTextureObjects)
{
    for (auto it = filePathToTextureObjects->begin();
         it != filePathToTextureObjects->end(); ) {

        if (_GarbageCollect(&it->second)) {
            it = filePathToTextureObjects->erase(it);
        } else {
            ++it;
        }
    }
}

void
LoFiTextureObjectRegistry::GarbageCollect()
{
    TRACE_FUNCTION();

    _textureObjectRegistry.GarbageCollect();

    _GarbageCollect(&_filePathToTextureObjects);
}

PXR_NAMESPACE_CLOSE_SCOPE
