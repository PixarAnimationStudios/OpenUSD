#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/sdf/wrapPathJs.h"
#include "pxr/usd/usd/emscriptenSdfToVtValue.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"

#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(UsdStage)

EMSCRIPTEN_BINDINGS(UsdGeomMesh) {
    class_<pxr::UsdGeomBoundable>("UsdGeomBoundable");
    class_<pxr::UsdGeomPointBased, base<pxr::UsdGeomBoundable>>("UsdGeomPointBased");
    class_<pxr::UsdGeomMesh, base<pxr::UsdGeomPointBased>>("UsdGeomMesh")
        .class_function("Define", &pxr::UsdGeomMesh::Define)
        .function("GetPointsAttr", &pxr::UsdGeomPointBased::GetPointsAttr)
        .function("CreatePointsAttr",
                  &SetCustomAttributeFromEmscriptenVal<pxr::UsdGeomPointBased,
                      &pxr::UsdGeomPointBased::CreatePointsAttr>)
        .function("GetFaceVertexCountsAttr", &pxr::UsdGeomMesh::GetFaceVertexCountsAttr)
        .function("CreateFaceVertexCountsAttr",
                  &SetCustomAttributeFromEmscriptenVal<pxr::UsdGeomMesh, &pxr::UsdGeomMesh::CreateFaceVertexCountsAttr>)
        .function("GetFaceVertexIndicesAttr", &pxr::UsdGeomMesh::GetFaceVertexIndicesAttr)
        .function("CreateFaceVertexIndicesAttr",
                  &SetCustomAttributeFromEmscriptenVal<pxr::UsdGeomMesh,
                      &pxr::UsdGeomMesh::CreateFaceVertexIndicesAttr>)
        .function("GetExtentAttr", &pxr::UsdGeomBoundable::GetExtentAttr)
        .function("CreateExtentAttr",
                  &SetCustomAttributeFromEmscriptenVal<pxr::UsdGeomBoundable, &pxr::UsdGeomBoundable::CreateExtentAttr>)
        ;
}