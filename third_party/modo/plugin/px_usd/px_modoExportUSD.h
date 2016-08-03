//
//  px_modoExportUSD.h
//
//  Created by Michael B. Johnson on 10/31/14.
//


#include <stdio.h>
#include <iostream>
#include <map>
#include <vector>
#include <utility>
#include <string>
#include <iostream>
#include <fstream>

#include <lx_io.hpp>
#include <lxu_scene.hpp>
#include <lx_locator.hpp>
#include <lx_log.hpp>
#include <lxidef.h>

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdUtils/pipeline.h"
#include "pxr/base/gf/range3f.h"


static const double	s_meters2mm = 1000.0;
static const double	s_meters2cm = 100.0;

namespace ModoExportUSD {

    class CUSDSaver : public CLxSceneSaver, CLxBinaryFormat {

        public:
            CUSDSaver();
            ~CUSDSaver() { /* not sure what we should let go of here... */ }

            virtual CLxFileFormat*	ss_Format() { return this; }

            virtual LxResult ss_Save();
            virtual void	 ss_Edge();
            virtual void	 ss_Point();
            virtual void	 ss_Polygon();

            static LXtTagInfoDesc descInfo[];

        protected:
            bool SaveItemAtParentPath(CLxUser_Item& item, 
                                      std::string parentPath, 
                                      GfMatrix4d parentMatrixInverse, 
                                      std::vector<std::string>& usedNames);
            bool WriteItemAtParentPath(CLxUser_Item& item, 
                                       std::string parentPath, 
                                       bool* actuallyWroteOut, 
                                       GfMatrix4d localMatrix, 
                                       std::string safeName);
            bool IsItemMeshInstanced(CLxUser_Item& item);

            bool WriteItemMeshAtParentPath(CLxUser_Item& item, 
                                           std::string parentPath, 
                                           GfMatrix4d localMatrix, 
                                           std::string safeName);
            bool WriteItemMeshInstanceAtParentPath(CLxUser_Item& item, 
                                                   std::string parentPath, 
                                                   GfMatrix4d localMatrix, 
                                                   std::string safeName);
            void WriteItemInstance(std::string meshPath, 
                                   std::string referencePath, 
                                   GfMatrix4d localXform);
            bool WriteItemTransformAtParentPath(CLxUser_Item& item, 
                                                std::string parentPath, 
                                                GfMatrix4d localMatrix, 
                                                std::string safeName);
            bool WriteItemCameraAtParentPath(CLxUser_Item& item, 
                                             std::string parentPath, 
                                             GfMatrix4d localMatrix, 
                                             std::string safeName);

            std::string& SafeName(const char* name);
            bool IsSafeName(const char* proposedName);
            std::string 
            SafeNameFromExclusionList(std::string initialName, 
                                      std::vector<std::string> namesToExclude);

            bool GetTransformOfCurrentItem(LXtMatrix xfrm, LXtVector offset);
            GfMatrix4d GetWorldTransformOfCurrentItem();
            GfMatrix4d 
            GetLocalTransformOfCurrentItem(GfMatrix4d parentMatrixInverse);
            void GatherColors();

            UsdStageRefPtr  myStage;

            enum {
                POLYGON_EXPORT_PASS_UVS,
                POLYGON_EXPORT_PASS_VERTICES,
                POLYGON_EXPORT_PASS_COLOR
            };
            int polygonExportPassType;
            int upAxis;

            LXtItemType		m_typeCamera;
            LXtItemType		m_typeMesh;
            LXtItemType		m_typeMeshInst;
            LXtItemType		m_typeGroupLocator;
            LXtItemType		m_typeLocator;

            GfMatrix4d theIdentityMatrix44;

            std::map<std::string, GfVec3f> materialColorMap;

            // for a given mesh item, this is the final name we'll 
            // use in USD for it
            std::map<CLxUser_Item, std::string> meshItemNameMap;

            std::map<CLxUser_Item, 
                     std::vector<CLxUser_Item>> meshMasterMeshInstancesMap;

            LXtItemType	meshType;
            unsigned int pointCount;
            bool exportingASubdiv;

            std::map<LXtPointID, unsigned int>	pointIndexMap;

            VtArray<GfVec3f> usdPoints;
            VtArray<int> usdFaceVertexCounts;
            VtArray<int> usdFlattenedFaceVertexIndices;
            VtArray<GfVec3f> usdFaceVertexRGBs;

            std::vector<std::string> mapNames;
            std::string uvName;
            std::map<std::string, VtArray<GfVec2f> > namedUVs;
            std::vector<bool> faceIsSubdiv;

            std::vector<int> weightedCornerPointIndices;
            std::vector<float> cornerWeights;
            // for USD custom data that's modo specific:
            std::string usd_modoNamespace;
            std::string usd_modo_originalItemName;
            std::string usd_modo_originalUVName;

            struct EdgeVert {
                unsigned		v0, v1;
            };
            struct EdgeVertCompare {
                bool operator() (EdgeVert v1, EdgeVert v2) const {
                    return memcmp(&v1, &v2, sizeof v1) < 0;
                }
            };
            typedef std::pair<EdgeVert, float> EdgePair;
            typedef std::map<EdgeVert, float, EdgeVertCompare>	EdgeMap;
            EdgeMap edgeMap;

            // for modo, the value of all elements will be 2 
            // and edgeMap.size() long
            VtArray<int> usdEdgeCreaseLengths; 
            // this will be 2 * edgeMap.size() long
            VtArray<int> usdFlattenedEdgeCreasePointIndices;  
            VtArray<float> usdFlattenedEdgeCreaseSharpnesses;
    };

    LXtTagInfoDesc	 CUSDSaver::descInfo[] = {
            { LXsSAV_OUTCLASS, LXa_SCENE	},
            { LXsSAV_DOSTYPE, "usda"		},
            { LXsSRV_USERNAME, "Pixar USD ASCII"},
            { LXsSRV_LOGSUBSYSTEM, "io-status"	},
            { 0 }
        };

    class CUSDMapNameVisitor : public CLxImpl_AbstractVisitor {
        private:
            CLxUser_MeshMap	_meshMap;
            std::vector<std::string> _names;

            LxResult Evaluate()	LXx_OVERRIDE {
                const char	*name = NULL;
                if (!_meshMap.test()) {
                    return LXe_FAILED;
                }
                if (LXx_OK(_meshMap.Name(&name))) {
                    _names.push_back(std::string(name));
                }
                return LXe_OK;
            }

        public:
            CUSDMapNameVisitor(CLxUser_MeshMap meshMap);
            unsigned MapCount();
            LxResult ByIndex(unsigned index, std::string &name);
            std::vector<std::string> names();
    };
};

