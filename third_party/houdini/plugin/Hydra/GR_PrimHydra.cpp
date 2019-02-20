// DreamWorks Animation LLC Confidential Information.
// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.

#include "GR_PrimHydra.h"
#include "USDtoRE.h"

// Houdini
// #include <GL/gl.h>
#include <GR/GR_Utils.h>
#include <RE/RE_Light.h>
#include <RE/RE_LightList.h>
#include <RE/RE_Render.h>

// Hydra
#include <pxr/imaging/glf/simpleLight.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#if PXR_MAJOR_VERSION || PXR_MINOR_VERSION >= 19
#include <pxr/usdImaging/usdImagingGL/engine.h>
#else
#include <pxr/usdImaging/usdImagingGL/gl.h>
#define UsdImagingGLEngine UsdImagingGL
#define UsdImagingGLRenderParams UsdImagingGL::RenderParams
#define UsdImagingGLCullStyle UsdImagingGL::CullStyle
#define UsdImagingGLDrawMode UsdImagingGL::DrawMode
#endif

namespace {

// Some converters from Houdini types to Pixar types:

pxr::GfVec3f vec3f(const UT_Vector3& v) { return pxr::GfVec3f(v.data()); }
pxr::GfVec4f vec4f(const UT_Vector4& v) { return pxr::GfVec4f(v.data()); }

// These are for colors and sets alpha to 1!
pxr::GfVec4f vec4f(float t) { return pxr::GfVec4f(t,t,t,1.0f); }
pxr::GfVec4f vec4f(const UT_Vector3& v) { auto t = v.data(); return pxr::GfVec4f(t[0],t[1],t[2],1.0f); }

// Compensate for Hydra doing 50% of the selection color
pxr::GfVec4f vec4fna(const UT_Vector4& v) {
    auto t = v.data(); return pxr::GfVec4f(t[0],t[1],t[2],std::min(1.0f,1-(1-t[3])/2));
}

// For viewport rectangle
pxr::GfVec4d vec4d(const UT_DimRect& v) { return pxr::GfVec4d(v.x(), v.y(), v.w(), v.h()); }

typedef const double v4d[4];
pxr::GfMatrix4d mat4d(const UT_Matrix4D& v) { return pxr::GfMatrix4d((v4d*)(v.data())); }

// Truncate matrix entries to float to hide imprecision
bool appxEqual(const UT_Matrix4D& a, const UT_Matrix4D& b)
{
    for (int x=0; x<4; ++x)
        for (int y=0; y<4; ++y)
            if (float(a(x,y)) != float(b(x,y)))
                return false;
    return true;
}

float clamp(float a, float b, float c) { return a < b ? b : a < c ? a : c; }

// You must use a different Hydra renderer for each stage. This provides a lookup table so
// they can be reused as much as possible. cleanup() destroys renders that were not used since
// the last cleanup() provided gcenable() was called. Houdini does not call anything when
// a GR_Primitive stops being shown so this is the only way I found to garbage-collect these.
// If there are multiple scene viewers this may screw up if they are not all showing the same
// thing, but it is not clear if that is possible.
class EngineMap {
    struct Entry {
        std::unique_ptr<pxr::UsdImagingGLEngine> pointer;
        int cleanupId;
    };
    std::map<pxr::UsdStageWeakPtr, Entry> map;
    int cleanupId = 0;
    bool usedSinceCleanup = false;
    std::mutex mutex;
public:
    void cleanup()
    {
        if (not usedSinceCleanup) return;
        std::lock_guard<std::mutex> lock(mutex);
        usedSinceCleanup = false;
        for (auto&& i : map) {
            if (i.second.cleanupId != cleanupId)
                i.second.pointer.reset();
        }
        ++cleanupId;
    }

    pxr::UsdImagingGLEngine* get(const pxr::UsdStageWeakPtr& stage)
    {
        std::lock_guard<std::mutex> lock(mutex);
        Entry& entry = map[stage];
        entry.cleanupId = cleanupId;
        if (not entry.pointer)
            entry.pointer.reset(new pxr::UsdImagingGLEngine());
        return entry.pointer.get();
    }

    void gcenable() { usedSinceCleanup = true; }

    // It would be nice to call this if we know Hydra is not being used
    // Destructor does this so renderers are cleaned up on exit.
    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex);
        map.clear();
        usedSinceCleanup = false;
    }

    size_t size() const
    {
        size_t n = 0;
        for (auto&& i : map) if (i.second.pointer) ++n;
        return n;
    }

} engineMap;

// Hash to identify unique usd prims
// This is different from hash_value(UsdObject) as it has stage in it, does not have type
static size_t hashValue(const pxr::UsdPrim& prim)
{
    return hash_value(prim.GetPath()) + hash_value(prim.GetStage());
}

// Hydra refuses to draw children of point instancers even if they are directly handed to it.
// Detect these and draw RE_Geometry instead.
bool inPointInstancer(const pxr::UsdPrim& prim)
{
    pxr::UsdPrim parent(prim.GetParent());
    if (not parent) return false;
    static std::map<size_t, bool> imap; // the answers are cached because it is slow
    auto pair(imap.emplace(hashValue(parent), false));
    bool& r = pair.first->second;
    if (pair.second)
        r = parent.IsA<pxr::UsdGeomPointInstancer>() || inPointInstancer(parent);
    return r;
}

// Houdini shaders
RE_ShaderHandle theConstShader("material/GL32/constant.prog");
RE_ShaderHandle theMatteShader("material/GL32/matte_tri.prog");
RE_ShaderHandle theLitShader("material/GL32/beauty_lit.prog");
RE_ShaderHandle theFlatShader("material/GL32/beauty_flat_lit.prog");
RE_ShaderHandle theUnlitShader("material/GL32/beauty_unlit.prog");
RE_ShaderHandle theWireShader("basic/GL32/wire_color.prog");

// copy select color to wire color but make it opaque
void
pushSelectWireColor(RE_Render* r)
{
    const float* data = r->getUniform(RE_UNIFORM_SELECT_COLOR)->getVector4().data();
    float f[4] = {data[0], data[1], data[2], 1.0f};
    r->pushUniformData(RE_UNIFORM_WIRE_COLOR, f);
}

// Work around RE instance group bug that is fixed in 16.0.826 (confirmed fixed in 16.0.877)
void
drawEverything(const std::unique_ptr<RE_Geometry>& geo, RE_Render* r, int group, unsigned n)
{
    static bool fixed = false;
    if (!fixed && n && geo->getInstanceGroupCount(group) == (int)n) fixed = true;
    if (fixed) {
        geo->setInstanceGroupDrawEverything(r, group);
    } else {
        UT_IntArray indices(n,n);
        for (unsigned k=0; k<n; ++k) indices[k] = k;
        geo->setInstanceGroupIndexList(r, group, false, &indices);
    }
}

// PickBuffer that is shared by all GR_PrimHydra instances. It is a good idea
// to share it as it can be huge if instances are used. Calling this repeatedly
// with a size <= previous size is very fast. Note that this leaks on exit.
RE_Geometry*
getPickBuffer(RE_Render* r, unsigned size)
{
    static unsigned previousSize = 0;
    static RE_Geometry* pickBuffer = nullptr;
    if (size > previousSize) {
        previousSize = size;
        if (not pickBuffer) {
            pickBuffer = new RE_Geometry(size);
            pickBuffer->createAttribute(r, "pickID", RE_GPU_INT32, size,
                                        nullptr, RE_ARRAY_POINT, 0,
                                        RE_BUFFER_READ_FREQUENT);
        } else {
            pickBuffer->setNumPoints(size);
        }
        // if (size > 100) // print this for debugging problem if it gets huge
        //     std::cout << "PickBuffer size " << size << std::endl;
    }
    return pickBuffer;
}

} // end of anonymous namespace

// See hydra.cc for installation of the hook.
int GR_PrimHydra::disable = 0;
bool GR_PrimHydra::postpass = true;

GR_Primitive*
GR_PrimHydraHook::createPrimitive(const GT_PrimitiveHandle& handle,
                                  const GEO_Primitive* hprim,
                                  const GR_RenderInfo* info,
                                  const char* cache_name,
                                  GR_PrimAcceptResult& processed)
{
    GR_PrimHydra* p = new GR_PrimHydra(info, cache_name);
    processed = p->acceptPrimitive(handle ? (GT_PrimitiveType)(handle->getPrimitiveType()) : GT_GEO_PRIMITIVE,
                                   hprim ? hprim->getTypeId().get() : 0,
                                   handle, hprim);
    return p;
}

GR_PrimAcceptResult
GR_PrimHydra::acceptPrimitive(GT_PrimitiveType t,
                              int typeId,
                              const GT_PrimitiveHandle& ph,
                              const GEO_Primitive* hprim)
{
    engineMap.cleanup();
    if (t != GT_PrimHydra::typeId())
        return GR_NOT_PROCESSED;
    this->ph = ph;
    return GR_PROCESSED;
}

void
GR_PrimHydra::cleanup(RE_Render*)
{
    engineMap.cleanup();
}

GR_PrimHydra::~GR_PrimHydra()
{
    engineMap.cleanup();
}

static std::map<const RE_Window*, GR_PrimHydra*> lastPrim; // last one render() called on per-window
static GR_PrimHydra* newLast; // new value for lastPrim
static bool sawLast; // true if visiting a prim after the "last" one

void
GR_PrimHydra::update(RE_Render* r,
                     const GT_PrimitiveHandle& ph,
                     const GR_UpdateParms& p)
{
    //GR_Utils::printUpdateReason(p.reason); std::cout << std::endl;
    if (p.reason & (GR_GEO_CHANGED | GR_GEO_PRIMITIVE_CHANGED)) {
        updateGeo = true;
        boxes.update = true;
        updateSelection = true;
        hasXform.clear(); // make it rebuild xform array
    }
    if (p.reason & GR_GEO_SELECTION_CHANGED) {
        updateSelection = true;
        if (GR_Utils::inPrimitiveSelection(p, GT()->pids, selected) == GR_SELECT_NONE)
            selected.clear();
    }
    // ghosted ones are never the lastPrim
    if ((p.reason & GR_OBJECT_MODE_CHANGED) && p.dopts.drawGhosted())
        for (auto&& i : lastPrim)
            if (i.second == this) i.second = nullptr;
}

// Parameters for Hydra renderer (a few added atop the pixar structure)
struct Parameters : public pxr::UsdImagingGLRenderParams
{
    UT_Matrix4D usdTransform; // inverse(usd)*prim transforms
    bool drawWireframe = false; // run a second pass to draw wireframe overlay
    bool noColor = false; // fill pass only sets the z buffer for hidden line removal
    bool noPostPass = false; // renders that must be run immediately
    bool operator==(const Parameters& p) const {
        return
            pxr::UsdImagingGLRenderParams::operator==(p) &&
            appxEqual(usdTransform, p.usdTransform) &&
            noPostPass == p.noPostPass &&
            drawWireframe == p.drawWireframe &&
            noColor == p.noColor;
    }
    bool operator!=(const Parameters& p) const { return not (*this == p); }
};

// All data needed to actually run a Hydra render
struct GR_PrimHydra::Hydra
{
    Parameters params;
    pxr::UsdStageWeakPtr stage; // we must make a new render for each stage
    std::vector<const pxr::GusdGU_PackedUSD*> prims; // prims to add to renderer
    pxr::SdfPathVector selectedPaths; // paths of selected prims
    std::set<size_t> hashes; // detect path inserted multiple times

    // Attempt to add a prim. Return true if successful. This tests for a number
    // of conflicts that indicate a new renderer is needed
    bool add(const pxr::GusdGU_PackedUSD* packedUSD, const Parameters& iparams, bool sel) {
        if (prims.empty()) { init(packedUSD, iparams, sel); return true; }
        if (params != iparams) return false;
        pxr::UsdPrim prim(packedUSD->getUsdPrim());
        if (stage != prim.GetStage()) return false;
        // Hydra cannot draw the same path twice. Assume they are the same prim and since
        // the transforms are the same ignore the second one.
        if (not hashes.insert(hash_value(prim.GetPath())).second) return true;
        prims.emplace_back(packedUSD);
        if (sel) selectedPaths.emplace_back(prim.GetPrimPath());
        return true;
    }

    // Add a prim to a newly-created Hydra
    void init(const pxr::GusdGU_PackedUSD* packedUSD, const Parameters& iparams, bool sel) {
        params = iparams;
        pxr::UsdPrim prim(packedUSD->getUsdPrim());
        stage = prim.GetStage();
        hashes.insert(hash_value(prim.GetPath()));
        prims.emplace_back(packedUSD);
        if (sel) selectedPaths.emplace_back(prim.GetPrimPath());
    }
};

static int postPassId = -1;
static std::vector<GR_PrimHydra::Hydra> hydras;
static bool lightsSet = false;

void
GR_PrimHydra::render(RE_Render* r,
                     GR_RenderMode renderMode,
                     GR_RenderFlags flags,
                     GR_DrawParms dp)
{
    ////////////////////////////////////////////////////////////////////////////////
    if (hasXform.size() == 0) {
        if (GT()->empty()) return;
        // One-time initialization to identify multiple instances, choose box/re/hydra for
        // each prim, and record transforms

        hasXform.setSize(size());
        drawType.resize(size());
        instanceOf.resize(size());
        hasRE = false;
        boxes.instances = 0;
        boxes.update = true;
        badPrims = 0;
        std::map<size_t, size_t> hashToInstance; // match up instances

        for (size_t i = 0; i < size(); ++i) {
            pxr::UsdPrim prim(getUsdPrim(i));
            if (not prim) {
                ++badPrims;
                drawType[i] = DrawType::HIDDEN;
                continue;
            }
            pxr::UsdGeomImageable imageable(prim);
            if (imageable &&
                imageable.ComputeVisibility(pxr::UsdTimeCode(getFrame(i))) ==
                pxr::UsdGeomTokens->invisible) {
                drawType[i] = DrawType::HIDDEN;
                continue;
            }
            switch (getViewportLOD(i)) {
            case GEO_VIEWPORT_HIDDEN:
            case GEO_VIEWPORT_POINTS:
                drawType[i] = DrawType::HIDDEN;
                continue;
            case GEO_VIEWPORT_CENTROID:
                drawType[i] = DrawType::CENTROID;
                continue;
            case GEO_VIEWPORT_BOX:
                drawType[i] = DrawType::BOX;
                boxes.instances++;
                continue;
            default:
                break;
            }
            // Handle HYDRA_HOUDINI_DISABLE=2
            if (disable) {
                drawType[i] = DrawType::RE;
                hasRE = true;
                continue;
            }
            // Detect if a prim is drawn more than once, use instance drawing
            size_t& iEntry(hashToInstance[hashValue(prim) + std::hash<double>()(getFrame(i))]);
            if (iEntry) {
                instanceOf[i] = instanceOf[iEntry-1] = iEntry;
                drawType[i] = drawType[iEntry-1] = DrawType::RE;
                hasRE = true;
                continue;
            }
            iEntry = i+1; // record first of possible instances
            // Any child of PointInstancer (such as a prototype) does not work in Hydra,
            // See if they fix it as this check is expensive!
            if (inPointInstancer(prim)) {
                drawType[i] = DrawType::RE;
                hasRE = true;
                continue;
            }
            // we now know Hydra will be used
            drawType[i] = DrawType::HYDRA;

            // compute the inverse xforms needed to move USD transform to local transform
            // getFullTransform4 is garbage for non-imageable, just ignore it
            if (imageable) {
                UT_Matrix4D hxform; getPrimPacked(i)->getFullTransform4(hxform);
                const UT_Matrix4D& usdxform(getPackedUSD(i)->getUsdTransform());
                if (not appxEqual(hxform, usdxform)) {
                    xforms.resize(size());
                    UT_Matrix4D inverse; usdxform.invert(inverse);
                    xforms[i] = inverse * hxform;
                    hasXform.setBitFast(i, true);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Decide how to draw based on renderMode, set up Hydra params

    enum { // which prims to use RE_Geometry instead of Hydra for
        RE_ONLY, // only ones where Hydra does not work (instances)
        GOOD_RE, // all "good" ones (meshes), Hydra can draw the "bad" ones
        ALL_RE // as much as possible, Hydra does not work for this renderMode
    } whichRE = RE_ONLY;

    RE_ShaderHandle* polyShader = &theConstShader; // shader to use for RE_Geometry polygons
    RE_ShaderHandle* wireShader = &theWireShader; // shader to use for wireframe + bounding boxes

    UT_Matrix4D objTransform(r->getUniform(RE_UNIFORM_OBJECT_MATRIX)->getMatrix4());

    Parameters params;
    /* default values:
       frame(UsdTimeCode::Default()),
       complexity(1.0),
       drawMode(DRAW_SHADED_SMOOTH),
       showGuides(false),
       showProxy(true),
       showRender(false),
       forceRefresh(false),
       flipFrontFacing(false),
       cullStyle(CULL_STYLE_NOTHING),
       enableIdRender(false),
       enableLighting(true),
       enableSampleAlphaToCoverage(false),
       applyRenderState(true),
       gammaCorrectColors(true),
       highlight(false),
       overrideColor(.0f, .0f, .0f, .0f),
       wireframeColor(.0f, .0f, .0f, .0f),
       alphaThreshold(-1),
       clipPlanes(),
       enableHardwareShading(true)
    */
    // lod is clamped to shut up Hydra warnings and to avoid hanging on huge complexity:
    params.complexity = clamp(dp.opts->common().LOD(), 1.0f, 1.4f);
    if (dp.opts->common().removeBackface())
        params.cullStyle = pxr::UsdImagingGLCullStyle::CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED;
    params.gammaCorrectColors = false; // nyi in usd code, but I believe we want this off
    params.enableSampleAlphaToCoverage = true; // usdView does this
    params.applyRenderState = false; // not necessary
    params.enableLighting = false; // most passes want this off
    bool tempSelections = false;
    bool drawPoly = true;
    params.noPostPass = not postpass || dp.opts->drawGhosted();

    // Set up params that vary per pass
    // For other passes draw Houdini geo if it works, then return
    switch (renderMode) {
    case GR_RENDER_BEAUTY:
    case GR_RENDER_MATERIAL:
        params.drawWireframe = (flags & GR_RENDER_FLAG_WIRE_OVER);
        params.enableLighting = not (flags & GR_RENDER_FLAG_UNLIT);
        if (flags & GR_RENDER_FLAG_FLAT_SHADED) {
            if (params.enableLighting) {
                params.drawMode = pxr::UsdImagingGLDrawMode::DRAW_SHADED_FLAT;
                polyShader = &theFlatShader;
            } else {
                polyShader = &theUnlitShader;
            }
            params.complexity = 1.0f; // flat ignores subdivision, make wireframe match
        } else {
            params.drawMode = pxr::UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
            polyShader = params.enableLighting ? &theLitShader : &theUnlitShader;
        }
        // disable the colored lines for the wireframe pass:
        params.wireframeColor = vec4fna(r->getUniform(RE_UNIFORM_WIRE_COLOR)->getVector4());
        break;
    case GR_RENDER_WIREFRAME:
    case GR_RENDER_MATERIAL_WIREFRAME:
        params.drawMode = pxr::UsdImagingGLDrawMode::DRAW_WIREFRAME;
        params.drawWireframe = true;
        drawPoly = false;
        // Houdini ignores backface culling for wireframe, but it seems useful so I keep it
        // params.cullStyle = pxr::UsdImagingGLCullStyle::CULL_STYLE_NOTHING;
        break;
    case GR_RENDER_XRAY_LINE: // wireframe ghost (nyi, reuse hidden line render)
    case GR_RENDER_HIDDEN_LINE:
        params.drawMode = pxr::UsdImagingGLDrawMode::DRAW_GEOM_ONLY;
        params.drawWireframe = true;
        params.noColor = true;
        break;
    case GR_RENDER_GHOST_LINE: // hidden line ghost
        params.drawMode = pxr::UsdImagingGLDrawMode::DRAW_GEOM_ONLY;
        params.overrideColor = vec4f(r->getUniform(RE_UNIFORM_CONST_COLOR)->getVector4());
        params.drawWireframe = true;
        break;
    case GR_RENDER_DEPTH: // used by marquee selection (!)
        // Using Hydra works if select-visible-only is turned off, but I can't find a way to detect that
        whichRE = ALL_RE;
        polyShader = wireShader = &theConstShader;
        break;
    case GR_RENDER_XRAY:
        // Render wireframe behind the depth map bound to sampler2D glH_DepthMap
        // this should draw the wireframe with glDepthFunc(GL_GREATER) but Hydra resets it
        return;
    case GR_RENDER_DEPTH_CUBE:
        // Render a shadowmap pass to the 6 faces of the current
        // cube map attached to the framebuffer. Can be done using layered
        // renderer, or one by one. Depth should be the unprojected depth
        // value (eye space, linear).
        return;
    case GR_RENDER_DEPTH_LINEAR:
        // Render a linear depth map to the 2D texture attached to the current
        // framebuffer.
        return;
    case GR_RENDER_OBJECT_PICK:
        // Used for click selection, must render into integer pick buffer the
        // RE_UNIFORM_PICK_BASE_ID value. The shader apparently is already set up for this.
        whichRE = ALL_RE;
        polyShader = wireShader = nullptr;
        break;
    case GR_RENDER_CONSTANT: // I have not seen this pass called
    case GR_RENDER_SHADER_AS_IS: // we can't do anything with shaders
    case GR_RENDER_BBOX: // not used by modern Houdini
        return;
    case GR_RENDER_MATTE:
        // Render a constant, solid matte of the object in front of
        // the beauty pass depth texture (bound to sampler2D glH_DepthMap).
        // Used for selection preview highlight.
        polyShader = wireShader = &theMatteShader;
        tempSelections = dp.opts->common().showTempSelections();
        whichRE = tempSelections ? ALL_RE : GOOD_RE;
        // Simulate "bad" geo using Hydra. I cannot get occlusion or partial
        // transparency to work, but a line drawing overlay looks pretty good. Only
        // works for object selection preview, as I don't have access to what
        // parts have preview selection (it is in shader variables).
        params.drawMode = pxr::UsdImagingGLDrawMode::DRAW_WIREFRAME;
        params.overrideColor = vec4f(r->getUniform(RE_UNIFORM_CONST_COLOR)->getVector4());
        params.cullStyle = pxr::UsdImagingGLCullStyle::CULL_STYLE_BACK;
        params.noPostPass = true;
        break;
    case GR_RENDER_POST_PASS: // run batched Hydra renders
        if (myInfo->getPostPassID() != postPassId) return;
        postPassId = -1;
        //std::cout << hydras.size() << " renders of " << engineMap.size() << " stages\n";
        for (Hydra& h : hydras)
            runHydra(r, h, dp);
        hydras.clear();
        lightsSet = false;
        // Detect if anything was drawn in the last post pass
        // if (lastPrim[r->getCurrentWindow()] != newLast && not sawLast) { possibly incorrect render }
        lastPrim[r->getCurrentWindow()] = newLast;
        sawLast = false;
        return;
    default:
        std::cerr << "Unexpected renderMode " << renderMode << std::endl;
        return;
    }
    // selected objects always draw yellow wireframe
    if (isObjectSelection())
        params.wireframeColor = vec4fna(r->getUniform(RE_UNIFORM_SELECT_COLOR)->getVector4());

    ////////////////////////////////////////////////////////////////////////////////
    // Draw text for null prims
    if (badPrims && (renderMode == GR_RENDER_BEAUTY || renderMode == GR_RENDER_MATERIAL)) {
        fpreal32 v[3] = {0, 0, 0};
        static const UT_Color red("red");
        r->drawViewportString(v, "Invalid USD Prim(s)", &red);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Fill polygons of prims that must be drawn with Houdini geometry:
    bool hasHydra = true; // will be true if anything needes to be drawn with Hydra
    if (drawPoly && (hasRE || whichRE)) {
        hasHydra = false; // this will be turned back on if something is not drawn using RE_Geometry
        //     r->printBuiltInUniforms(false);
        if (params.drawWireframe) {
            r->polygonOffset(true);
            if (params.noColor)
                r->disableColorBufferWriting();
        }
        if (polyShader) {
            r->pushShader(*polyShader);
            if (dp.materials) {
                RE_MaterialPtr mat = dp.materials->getDefaultMaterial();
                if (not mat) mat = dp.materials->getFactoryMaterial();
                if (mat) {
                    RE_Shader* shader(r->getShader());
                    if (shader && dp.opts->getLightList())
                        dp.opts->getLightList()->bindForShader(r, shader);
                    mat->updateShaderForMaterial(r, 0, true, true, RE_SHADER_TARGET_TRIANGLE, shader);
                }
            }
        }
        for (size_t i = 0; i < size(); ++i) {
            if (drawType[i] != DrawType::RE) {
                if (drawType[i] != DrawType::HYDRA) continue;
                if (whichRE == RE_ONLY) { hasHydra = true; continue; }
            }
            const auto& geo = buildGeo(r, i);
            if (not geo.geo) continue;

            if (geo.instances) {
                if (showSelections() && updateSelection) {
                    std::vector<int> sel;
                    for (size_t j = i; j < size(); ++j)
                        if (instanceOf[j] == i+1)
                            sel.emplace_back(selected[j]);
                    RE_VertexArray* va = geo.geo->findCachedInstanceGroupAttrib(
                        r, 0, "InstSelection", RE_GPU_INT32, 1, 1, geo.instances, true);
                    va->setArray(r, &sel[0]);
                }
                if (tempSelections) {
                    r->assignUniformInt(RE_UNIFORM_USE_INSTANCE_PRIM_ID, 1);
                    r->assignUniformInt(RE_UNIFORM_PRIM_CONSTANT_ID, 0);
                }
                geo.geo->drawInstanceGroup(r, RE_GEO_SHADED_IDX, 0);
            } else {
                if (whichRE == GOOD_RE && not geo.good) { hasHydra = true; continue; }
                if (showSelections()) {
                    r->assignUniformInt(RE_UNIFORM_SELECT_MODE,
                                        selected[i] ? GR_SELECT_PRIM_FULL : GR_SELECT_NONE);
                }
                if (tempSelections) {
                    r->assignUniformInt(RE_UNIFORM_USE_INSTANCE_PRIM_ID, 0);
                    r->assignUniformInt(RE_UNIFORM_PRIM_CONSTANT_ID,
                                        getPrimPacked(i)->getMapIndex()+1);
                }
                geo.geo->draw(r, RE_GEO_SHADED_IDX);
            }
        }
        if (showSelections()) {
            updateSelection = false;
            r->assignUniformInt(RE_UNIFORM_SELECT_MODE, GR_SELECT_NONE);
        }
        if (tempSelections) {
            r->assignUniformInt(RE_UNIFORM_USE_INSTANCE_PRIM_ID, 0);
            r->assignUniformInt(RE_UNIFORM_PRIM_CONSTANT_ID, 0);
        }

        if (polyShader) r->popShader();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Wireframe polygons of prims that must be drawn with Houdini geometry:
    // Selection colors don't work in the wire render shader, so we have to do it ourselves.
    if (params.drawWireframe && hasRE) {
        if (params.noColor)
            r->enableColorBufferWriting();
        r->polygonOffset(false);
        r->pushBlendState();
        r->blendAlpha(1);
        //r->pushLineWidth(dp.opts->common().wireWidth()); //ignore to match Hydra
        r->pushShader(*wireShader);
        hasHydra = false; // turn it on if we see any hydra geometry
        for (size_t i = 0; i < size(); ++i) {
            if (drawType[i] != DrawType::RE) {
                if (drawType[i] == DrawType::HYDRA) hasHydra = true;
                continue;
            }
            const auto& geo = buildGeo(r, i);
            if (not geo.geo) continue;
            if (geo.instances) {
                if (showSelections()) {
                    UT_IntArray unsel(geo.instances, 0);
                    UT_IntArray sel(geo.instances, 0);
                    int k = 0;
                    for (size_t j = i; j < size(); ++j) {
                        if (instanceOf[j] == i+1) {
                            if (selected[j]) sel.emplace_back(k);
                            else unsel.emplace_back(k);
                            ++k;
                        }
                    }
                    if (not sel.isEmpty()) {
                        if (not unsel.isEmpty()) {
                            geo.geo->setInstanceGroupIndexList(r, 0, false, &unsel);
                            geo.geo->drawInstanceGroup(r, RE_GEO_WIRE_IDX, 0);
                        }
                        pushSelectWireColor(r);
                        geo.geo->setInstanceGroupIndexList(r, 0, false, &sel);
                        geo.geo->drawInstanceGroup(r, RE_GEO_WIRE_IDX, 0);
                        r->popUniform(RE_UNIFORM_WIRE_COLOR);
                        drawEverything(geo.geo, r, 0, geo.instances);
                        continue;
                    }
                }
                geo.geo->drawInstanceGroup(r, RE_GEO_WIRE_IDX, 0);
            } else {
                if (showSelections() && selected[i]) {
                    pushSelectWireColor(r);
                    geo.geo->draw(r, RE_GEO_WIRE_IDX);
                    r->popUniform(RE_UNIFORM_WIRE_COLOR);
                } else {
                    geo.geo->draw(r, RE_GEO_WIRE_IDX);
                }
            }
        }
        r->popShader();
        r->popBlendState();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Wireframe bounding boxes
    // Selection colors don't work in the wire render shader, so we have to do it ourselves.
    if (boxes.instances) {
        buildBoxes(r);
        r->pushBlendState();
        if (wireShader) r->pushShader(*wireShader);
        if (wireShader == &theMatteShader) {
            // the matte pass does not like wireframes, so fill the bounding boxes. Looks ok...
            if (tempSelections) {
                r->assignUniformInt(RE_UNIFORM_USE_INSTANCE_PRIM_ID, 1);
                r->assignUniformInt(RE_UNIFORM_PRIM_CONSTANT_ID, 0);
            }
            boxes.geo->drawInstanceGroup(r, RE_GEO_SHADED_IDX, 0);
            if (tempSelections)
                r->assignUniformInt(RE_UNIFORM_USE_INSTANCE_PRIM_ID, 0);
        } else {
            if (wireShader) r->blendAlpha(1);
            r->pushLineWidth(dp.opts->common().wireWidth());
            if (showSelections()) {
                UT_IntArray unsel(boxes.instances, 0);
                UT_IntArray sel(boxes.instances, 0);
                int k = 0;
                for (size_t j = 0; j < size(); ++j) {
                    if (drawType[j] == DrawType::BOX) {
                        if (selected[j]) sel.emplace_back(k);
                        else unsel.emplace_back(k);
                        ++k;
                    }
                }
                if (not sel.isEmpty()) {
                    if (not unsel.isEmpty()) {
                        boxes.geo->setInstanceGroupIndexList(r, 0, false, &unsel);
                        boxes.geo->drawInstanceGroup(r, RE_GEO_WIRE_IDX, 0);
                    }
                    pushSelectWireColor(r);
                    boxes.geo->setInstanceGroupIndexList(r, 0, false, &sel);
                    boxes.geo->drawInstanceGroup(r, RE_GEO_WIRE_IDX, 0);
                    r->popUniform(RE_UNIFORM_WIRE_COLOR);
                    drawEverything(boxes.geo, r, 0, boxes.instances);
                } else {
                    boxes.geo->drawInstanceGroup(r, RE_GEO_WIRE_IDX, 0);
                }
            } else {
                boxes.geo->drawInstanceGroup(r, RE_GEO_WIRE_IDX, 0);
            }
            r->popLineWidth();
        }
        if (wireShader) r->popShader();
        r->popBlendState();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Render hydra prims
    if (not hasHydra) return;

    if (params.drawWireframe) {
        // turn this flag off if main draw mode does it
        if (params.drawMode == pxr::UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH) {
            params.drawMode = pxr::UsdImagingGLDrawMode::DRAW_WIREFRAME_ON_SURFACE;
            params.drawWireframe = false;
        } else if (params.drawMode == pxr::UsdImagingGLDrawMode::DRAW_WIREFRAME) {
            params.drawWireframe = false;
        }
    }

    Hydra* hp = nullptr; // cache last one that we could use

    // Only run gc if we are drawing all hydra objects
    if (not whichRE) engineMap.gcenable();

    for (size_t i = 0; i < size(); ++i) {
        if (drawType[i] != DrawType::HYDRA) continue;
        if (whichRE == GOOD_RE && myGeo[i].good) continue; // already drawn

        const pxr::GusdGU_PackedUSD* p = getPackedUSD(i);

        params.frame = p->intrinsicFrame();
        auto&& purposes = p->getPurposes();
        params.showGuides = purposes & pxr::GUSD_PURPOSE_GUIDE;
        params.showProxy = purposes & pxr::GUSD_PURPOSE_PROXY;
        params.showRender = purposes & pxr::GUSD_PURPOSE_RENDER;

        if (hasXform[i])
            params.usdTransform = xforms[i] * objTransform;
        else
            params.usdTransform = objTransform;

        bool sel = selected[i] && showSelections();

        // Make a new Hydra renderer if it needs to change
        if (not (hp && hp->add(p, params, sel))) {
            hp = nullptr;
            for (Hydra& h : hydras)
                if (h.add(p, params, sel)) { hp = &h; break; }
            if (not hp) {
                hydras.emplace_back();
                hp = &hydras.back();
                hp->init(p, params, sel);
            }
        }
    }

    if (params.enableLighting)
        setupLighting(r, dp);

    if (params.noPostPass) {
        // Run ghosting and matte passes now, as Houdini composites the result immediately.
        for (auto i = hydras.begin(); i != hydras.end(); ) {
            Hydra& h = *i;
            if (h.params.noPostPass) {
                runHydra(r, h, dp);
                i = hydras.erase(i);
            } else {
                ++i;
            }
        }
    } else {
        // Try to run all the renders in the last normal pass rather than post pass.
        // This is to correct composition with volume renders that use the post pass
        // to make sure they are drawn last.
        GR_PrimHydra* last = lastPrim[r->getCurrentWindow()];
        if (this == last || not last || sawLast) {
            sawLast = true;
            for (Hydra& h : hydras)
                runHydra(r, h, dp);
            hydras.clear();
        }
        newLast = this;
    }
    // Create the post pass if not done already
    if (postPassId == -1)
        postPassId = myInfo->requestRenderPostPass();
}

// Copy the lighting setup from Houdini viewer. This information is not available
// during the post pass so it has to be cached from the main pass. Assume lights
// are the same for all render calls.
static pxr::GlfSimpleLightVector lights;
static pxr::GfVec4f ambient;
static pxr::GlfSimpleMaterial material;
void
GR_PrimHydra::setupLighting(RE_Render* r, GR_DrawParms& dp)
{
    if (lightsSet) return;
    lightsSet = true;

    lights.clear();
    ambient.Set(0,0,0,0); // match Houdini, usdView defaults to .1f
    
    bool emissionEnable = *(const float*)(r->getUniform(RE_UNIFORM_EMISSION)->getValue());
    bool specularEnable = *(const float*)(r->getUniform(RE_UNIFORM_SPECULAR)->getValue());
    bool diffuseEnable = *(const float*)(r->getUniform(RE_UNIFORM_DIFFUSE)->getValue());
    bool ambientEnable = *(const float*)(r->getUniform(RE_UNIFORM_AMBIENT)->getValue());

    if (const RE_LightList* hlights = dp.opts->getLightList()) {
        for (int i = 0; i < hlights->getNumLights(); ++i) {
            if (not hlights->isLightEnabled(i)) continue;
            const RE_Light& hlight = *hlights->getLight(i);
            if (hlight.isAmbient()) {
                if (ambientEnable) {
                    ambient += vec4f(hlight.getColor());
                    ambient[3] = 1;
                }
                continue;
            }
            pxr::GlfSimpleLight light;
            if (diffuseEnable)
                light.SetDiffuse(vec4f(hlight.getColor()));
            else
                light.SetDiffuse(pxr::GfVec4f(0));
            light.SetAmbient(pxr::GfVec4f(0));
            if (specularEnable && hlight.isSpecular())
                light.SetSpecular(vec4f(hlight.getColor()));
            else
                light.SetSpecular(pxr::GfVec4f(0));
            //light.SetIsCameraSpaceLight(hlight.isHeadlight()); // it already places it correctly
            if (hlight.isInfinite()) {
                const float* f = hlight.getDirection().data();
                light.SetPosition(pxr::GfVec4f(-f[0],-f[1],-f[2],0));
            } else {
                light.SetPosition(vec4f(hlight.getPosition()));
            }
            if (hlight.isCone()) {
                light.SetSpotDirection(vec3f(hlight.getDirection()));
                float a = hlight.getConeAngle();
                float b = hlight.getConeDelta();
                // Houdini does interpolation between a/2 and a/2+b
                // Hydra truncates at cutoff, and falloff is power to raise cos(p dot axis) to
                // I set the cutoff to a/2+b where Houdini goes to black, and compute
                // falloff so that a/2+b/2 is set to .5
                light.SetSpotCutoff(std::min(a/2+b, 180.0f));
                float c = cosf((a+b)/2*M_PI/180);
                light.SetSpotFalloff(c > 0 ? logf(0.5f)/logf(c) : 0.0f);
            }
            float atten[3]; hlight.getAttenuation(atten);
            light.SetAttenuation(pxr::GfVec3f(atten));
            // Shadows don't work, not clear what you need to do to get them in Hydra
            // if (hlight.isShadowed()) {
            //     light.SetShadowMatrix;
            //     light.SetShadowResolution(hlight.getShadowMapSize());
            //     light.SetShadowBias(hlight.getShadowBias());
            //     light.SetShadowBlur(hlight.getShadowBlur());
            //     light.SetHasShadow(true);
            //     light.SetShadowIndex(++shadowIndex);
            // }
            lights.emplace_back(light);
        }
    }

    const RE_MaterialPtr& m(dp.materials->getFactoryMaterial());
    material.SetAmbient(vec4f(m->amb()*m->diff()));
    material.SetDiffuse(vec4f(m->diff())); // overridden by object color in Hydra
    material.SetSpecular(vec4f(m->spec()+m->diff())); // Houdini seems to ignore this...
    material.SetEmission(emissionEnable ? vec4f(m->emit()) : vec4f(0));
    material.SetShininess(0.5 * pow(m->roughness(),-4));
}

// Run a single Hydra render
// This writes over h.params so it cannot be run again.
void
GR_PrimHydra::runHydra(RE_Render* r, Hydra& h, GR_DrawParms& dp)
{
    if (h.prims.empty()) return;

    r->pushShader(nullptr);

    // the renderer to use, pulled from per-stage cache
    pxr::UsdImagingGLEngine* engine = engineMap.get(h.stage);
    engine->PrepareBatch(h.stage->GetPseudoRoot(), h.params);
    pxr::SdfPathVector paths;

    for (auto&& p : h.prims) {
        // add the prim to the renderer
        pxr::UsdPrim prim(p->getUsdPrim());
        //engine->PrepareBatch(prim, h.params);
        paths.emplace_back(prim.GetPrimPath());
    }

    engine->SetCameraState(
        mat4d(r->getUniform(RE_UNIFORM_VIEW_MATRIX)->getMatrix4()),
        mat4d(r->getUniform(RE_UNIFORM_PROJECT_MATRIX)->getMatrix4()),
        vec4d(r->getViewport2DI()));

    engine->SetRootTransform(mat4d(h.params.usdTransform));

    if (h.params.enableLighting)
        engine->SetLightingState(lights, material, ambient);

    if (not h.selectedPaths.empty()) {
        h.params.highlight = true;
        engine->SetSelectionColor(
            vec4fna(r->getUniform(RE_UNIFORM_SELECT_COLOR)->getVector4()));
        engine->SetSelected(h.selectedPaths);
    }

    if (h.params.drawWireframe) {
        r->polygonOffset(true);
        if (h.params.noColor) r->disableColorBufferWriting();
    }

    engine->RenderBatch(paths, h.params);

    if (h.params.drawWireframe) {
        if (h.params.noColor) r->enableColorBufferWriting();
        r->polygonOffset(false);
        h.params.drawMode = pxr::UsdImagingGLDrawMode::DRAW_WIREFRAME;
        h.params.overrideColor = pxr::GfVec4f(0); // this overrides wireframeColor, turn it off
        if (h.params.wireframeColor[3]) h.params.enableLighting = false; // don't light solid colors
        engine->RenderBatch(paths, h.params);
    }

    // Hydra changed the shader so cached values in RE_Render must be cleared
    r->requestFixedFunction(); // forget about cached current shader
    r->getBoundUniformBlocks().zero(); // forget about cached uniform settings
    r->popShader();

    r->printAllGLErrors("Hydra");
}

// Subclass so the protected geo member can be changed
class MyPickRender : public GR_PickRender
{
public:
    MyPickRender(RE_Render* r, const GR_DisplayOption* opts, const GR_RenderInfo* info,
                 RE_Geometry* geo): GR_PickRender(r, opts, info, geo) {}
    void setGeo(RE_Geometry* geo) { this->geo = geo; }
};

int
GR_PrimHydra::renderPick(RE_Render* r,
                         const GR_DisplayOption* opt,
                         unsigned int pick_type,
                         GR_PickStyle pick_style,
                         bool has_pick_map)
{
    if (pick_type != GR_PICK_PRIMITIVE)
        return 0;

    // Pick buffer must be non-null for MULTI_VISIBLE or it crashes. It is
    // only actually used for MULTI_FRUSTUM
    RE_Geometry* pick_buffer = nullptr;
    if (pick_style & GR_PICK_MULTI_FLAG)
        pick_buffer = ::getPickBuffer(r, 1);

    MyPickRender picker(r, opt, myInfo, nullptr);
    int totalpicks = 0;
    int idData[3] = { 0, 0, 0};
    int pickData[6] = {
        GR_PICK_PRIMITIVE,
        ((const int*)(r->getUniform(RE_UNIFORM_PICK_BASE_ID)->getValue()))[1],
        0, 0, 0, 0};

    for (size_t i = 0; i < size(); ++i) {
        if (drawType[i] < DrawType::RE) continue; // not visible
        auto& geo = buildGeo(r, i);
        if (not geo.geo) continue;
        picker.setGeo(geo.geo.get());

        if (not geo.instances) {
            idData[0] = getPrimPacked(i)->getMapIndex() + 1;
            r->assignUniformData(RE_UNIFORM_PICK_COMPONENT_ID, idData);
        } else if (pick_style & GR_PICK_MULTI_FRUSTUM) {
            // multi frustum puts one entry into the pick buffer for each prim drawn.
            // So we must make the pick buffer much larger, so we get at least one
            // prim from the last one drawn:
            pick_buffer = getPickBuffer(r, geo.prims * (geo.instances-1) + 1);
        }
        int npicks = picker.renderFacePrims(RE_GEO_SHADED_IDX, 1,
                                            geo.instances ? GR_PICK_INSTANCE_ID : GR_PICK_CONSTANT_ID,
                                            pick_style, has_pick_map, false,
                                            GR_SELECT_PRIM_FULL, -1,
                                            pick_buffer);
        if (npicks > 0 && (pick_style & GR_PICK_MULTI_FRUSTUM)) {
            if (geo.instances) {
                totalpicks += accumulatePickIDs(r, npicks, pick_buffer);
            } else {
                // trivally faster as it does not read back the pick buffer
                pickData[3] = idData[0];
                myInfo->getPickArray().emplace_back(pickData);
                totalpicks++;
            }
        }
    }

    if (boxes.instances) {
        buildBoxes(r);
        picker.setGeo(boxes.geo.get());
        if (pick_style & GR_PICK_MULTI_FRUSTUM) // see above
            pick_buffer = getPickBuffer(r, boxes.prims * (boxes.instances-1) + 1);
        int npicks = picker.renderLinePrims(RE_GEO_WIRE_IDX, 1,
                                            GR_PICK_INSTANCE_ID,
                                            pick_style, has_pick_map, false,
                                            GR_SELECT_PRIM_FULL, -1,
                                            pick_buffer);
        if (npicks > 0 && (pick_style & GR_PICK_MULTI_FRUSTUM))
            totalpicks += accumulatePickIDs(r, npicks, pick_buffer);
    }

    r->printAllGLErrors("renderPick");
    return totalpicks;
}

const GR_PrimHydra::RE_Geo&
GR_PrimHydra::_buildGeo(RE_Render* r, size_t i)
{
    if (updateGeo) {
        updateGeo = false;
        myGeo.resize(size());
        for (auto& g : myGeo) {g.good = g.update = true;}
    }

    RE_Geo& geo = myGeo[i];
    geo.update = false;

    size_t instance = instanceOf[i];
    if (instance && instance != i+1) { // not first one in an instance set, don't draw anything
        geo.geo.reset(); // delete left-over geometry of other instances
        geo.good = myGeo[instance-1].good;
        geo.instances = 0;
        return geo;
    }

    static const pxr::GfMatrix4d identity(1);

    // Translate the GusdPurposeSet to a TfTokenVector used by the usd library:
    auto&& purposeSet = getPurposes(i);
    pxr::TfTokenVector purposes;
    //if (purposeSet & GUSD_PURPOSE_DEFAULT) // Hydra acts like this is true always, match it
        purposes.push_back(pxr::UsdGeomTokens->default_);
    if (purposeSet & pxr::GUSD_PURPOSE_PROXY)
        purposes.push_back(pxr::UsdGeomTokens->proxy);
    if (purposeSet & pxr::GUSD_PURPOSE_RENDER)
        purposes.push_back(pxr::UsdGeomTokens->render);
    if (purposeSet & pxr::GUSD_PURPOSE_GUIDE)
        purposes.push_back(pxr::UsdGeomTokens->guide);

    pxr::UsdPrim prim(getUsdPrim(i));
    geo.good = USDtoRE(prim, getFrame(i), identity, purposes, r, geo.geo,
                       &geo.prims, drawType[i]==DrawType::RE);
    geo.instances = 0;
    if (not geo.geo) return geo;

    // non-imagables have garbage in the local xform:
    UT_Matrix4D local;
    if (prim.IsA<pxr::UsdGeomImageable>())
        getPrimPacked(i)->getFullTransform4(local);
    else
        local.identity();
    if (instance) { // build per-instance attributes if there is more than one
        std::vector<UT_Matrix4F> xforms;
        xforms.emplace_back(local);
        std::vector<int> ids;
        ids.emplace_back(getPrimPacked(i)->getMapIndex());
        for (size_t j = i+1; j < size(); ++j)
            if (instanceOf[j] == instance) {
                getPrimPacked(j)->getFullTransform4(local);
                xforms.emplace_back(local);
                ids.emplace_back(getPrimPacked(j)->getMapIndex());
            }
        if (xforms.size() > 1) {
            geo.instances = xforms.size();
            geo.geo->findCachedInstanceGroupAttrib(
                r, 0, "InstTransform", RE_GPU_FLOAT32, 4, 1, geo.instances*4, true
            )->setArray(r, xforms[0].data());
            geo.geo->findCachedInstanceGroupAttrib(
                r, 0, "InstID", RE_GPU_INT32, 1, 1, geo.instances, true
            )->setArray(r, &ids[0]);
            drawEverything(geo.geo, r, 0, geo.instances);
            local.identity();
        }
    }
    geo.geo->setConstInstanceGroupTransform(0, local, not geo.instances);

    return geo;
}

void
GR_PrimHydra::buildBoxes(RE_Render* r)
{
    if (not boxes.update || not boxes.instances) return;
    boxes.update = false;
    RE_Geometry* geo = boxes.geo.get();
    if (not geo) {
        // build 1x1x1 wireframe cube centered on origin
        boxes.geo.reset((geo = new RE_Geometry(8, false)));
        static const fpreal32 P[] = {
            -.5,-.5,-.5, -.5,-.5,+.5, -.5,+.5,+.5, -.5,+.5,-.5, 
            +.5,-.5,-.5, +.5,-.5,+.5, +.5,+.5,+.5, +.5,+.5,-.5};
        geo->createAttribute(r, "P", RE_GPU_FLOAT32, 3, P);
        static const float color[4] = {0,0,0,1};
        geo->createConstAttribute(r, "Cd", RE_GPU_FLOAT32, 3, color);
        geo->createConstAttribute(r, "Alpha", RE_GPU_FLOAT32, 1, color+3);
        // two triangles for each side
        static const unsigned J[] = {
            0,3,2, 0,2,1,
            0,1,5, 0,5,4,
            4,5,6, 4,6,7,
            7,6,2, 7,2,3,
            1,2,6, 1,6,5,
            0,4,7, 0,7,3
        };
        geo->connectIndexedPrims(r, RE_GEO_SHADED_IDX, RE_PRIM_TRIANGLES, 6*2*3, J, nullptr, true);
        // edge lines
        static const unsigned I[] = {
            0,1, 1,2, 2,3, 3,0,
            4,5, 5,6, 6,7, 7,4,
            0,4, 1,5, 2,6, 3,7
        };
        geo->connectIndexedPrims(r, RE_GEO_WIRE_IDX, RE_PRIM_LINES, 2*12, I, nullptr, true);
        boxes.prims = 12; // coincidentally same number of triangles and edges
    }
    std::vector<UT_Matrix4F> xforms;
    std::vector<int> ids;
    for (size_t j = 0; j < size(); ++j) {
        if (drawType[j] != DrawType::BOX) continue;
        UT_Matrix4D local;
        if (getUsdPrim(j).IsA<pxr::UsdGeomImageable>())
            getPrimPacked(j)->getFullTransform4(local);
        else
            local.identity();
        UT_BoundingBox box;
        getPrimPacked(j)->getUntransformedBounds(box);
        local.pretranslate(box.centerX(), box.centerY(), box.centerZ());
        local.prescale(box.sizeX(), box.sizeY(), box.sizeZ());
        xforms.emplace_back(local);
        ids.emplace_back(getPrimPacked(j)->getMapIndex());
    }
    assert(boxes.instances == xforms.size());
    geo->findCachedInstanceGroupAttrib(
        r, 0, "InstTransform", RE_GPU_FLOAT32, 4, 1, boxes.instances*4, true
    )->setArray(r, xforms[0].data());
    geo->findCachedInstanceGroupAttrib(
        r, 0, "InstID", RE_GPU_INT32, 1, 1, boxes.instances, true
    )->setArray(r, &ids[0]);
    drawEverything(boxes.geo, r, 0, boxes.instances);
}

// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.
