//Maya ASCII 2016 scene
//Name: UsdExportAssemblyEditsTest.ma
//Last modified: Wed, Apr 03, 2019 01:32:25 PM
//Codeset: UTF-8
requires maya "2016";
requires -nodeType "pxrUsdReferenceAssembly" -dataType "pxrUsdStageData" "pxrUsd" "1.0";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2016";
fileInfo "version" "2016";
fileInfo "cutIdentifier" "201603061001-989194";
fileInfo "osv" "Linux 2.6.32-504.el6.x86_64 #1 SMP Tue Sep 16 01:56:35 EDT 2014 x86_64";
createNode transform -s -n "persp";
	rename -uid "B3ABA940-0000-1380-5C9B-ECE500000234";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 156.06924551972861 -238.88631033584707 192.15822152900623 ;
	setAttr ".r" -type "double3" 51.682924915361284 1.5902773407317584e-15 13.594913994747277 ;
createNode camera -s -n "perspShape" -p "persp";
	rename -uid "B3ABA940-0000-1380-5C9B-ECE500000235";
	setAttr -k off ".v" no;
	setAttr ".fl" 34.999999999999993;
	setAttr ".coi" 305.38107849483146;
	setAttr ".imn" -type "string" "persp";
	setAttr ".den" -type "string" "persp_depth";
	setAttr ".man" -type "string" "persp_mask";
	setAttr ".tp" -type "double3" 99.749999761581421 -6.0000000596046448 2.8180193905866284 ;
	setAttr ".hc" -type "string" "viewSet -p %camera";
createNode transform -s -n "top";
	rename -uid "B3ABA940-0000-1380-5C9B-ECE500000236";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 0 1000.1 ;
createNode camera -s -n "topShape" -p "top";
	rename -uid "B3ABA940-0000-1380-5C9B-ECE500000237";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "top";
	setAttr ".den" -type "string" "top_depth";
	setAttr ".man" -type "string" "top_mask";
	setAttr ".hc" -type "string" "viewSet -t %camera";
	setAttr ".o" yes;
createNode transform -s -n "front";
	rename -uid "B3ABA940-0000-1380-5C9B-ECE500000238";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 -1000.1 0 ;
	setAttr ".r" -type "double3" 89.999999999999986 0 0 ;
createNode camera -s -n "frontShape" -p "front";
	rename -uid "B3ABA940-0000-1380-5C9B-ECE500000239";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "front";
	setAttr ".den" -type "string" "front_depth";
	setAttr ".man" -type "string" "front_mask";
	setAttr ".hc" -type "string" "viewSet -f %camera";
	setAttr ".o" yes;
createNode transform -s -n "side";
	rename -uid "B3ABA940-0000-1380-5C9B-ECE50000023A";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 1000.1 0 0 ;
	setAttr ".r" -type "double3" 90 1.2722218725854067e-14 89.999999999999986 ;
createNode camera -s -n "sideShape" -p "side";
	rename -uid "B3ABA940-0000-1380-5C9B-ECE50000023B";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "side";
	setAttr ".den" -type "string" "side_depth";
	setAttr ".man" -type "string" "side_mask";
	setAttr ".hc" -type "string" "viewSet -s %camera";
	setAttr ".o" yes;
createNode transform -n "UsdExportAssemblyEditsTest";
	rename -uid "B3ABA940-0000-1380-5C9B-ED140000025B";
createNode pxrUsdReferenceAssembly -n "ModelNoEdits" -p "UsdExportAssemblyEditsTest";
	rename -uid "B3ABA940-0000-1380-5C9B-ED0400000259";
	setAttr ".isc" yes;
	setAttr ".t" -type "double3" 0 25 0 ;
	setAttr ".fp" -type "string" "./ShapesModel.usda";
	setAttr ".pp" -type "string" "/ShapesModel";
	setAttr ".rns" -type "string" "NS_ModelNoEdits";
createNode pxrUsdReferenceAssembly -n "ModelWithEdits" -p "UsdExportAssemblyEditsTest";
	rename -uid "B3ABA940-0000-1380-5C9B-ED880000025C";
	setAttr ".isc" yes;
	setAttr ".t" -type "double3" 100 25 0 ;
	setAttr ".fp" -type "string" "./ShapesModel.usda";
	setAttr ".pp" -type "string" "/ShapesModel";
	setAttr ".rns" -type "string" "NS_ModelWithEdits";
createNode pxrUsdReferenceAssembly -n "SetNoEdits" -p "UsdExportAssemblyEditsTest";
	rename -uid "B3ABA940-0000-1380-5C9B-EDB10000025E";
	setAttr ".isc" yes;
	setAttr ".t" -type "double3" 0 -25 0 ;
	setAttr ".fp" -type "string" "./Shapes_set.usda";
	setAttr ".pp" -type "string" "/Shapes_set";
	setAttr ".rns" -type "string" "NS_SetNoEdits";
createNode pxrUsdReferenceAssembly -n "SetWithSetEdits" -p "UsdExportAssemblyEditsTest";
	rename -uid "B3ABA940-0000-1380-5C9B-EDCB00000260";
	setAttr ".isc" yes;
	setAttr ".t" -type "double3" 100 -25 0 ;
	setAttr ".fp" -type "string" "./Shapes_set.usda";
	setAttr ".pp" -type "string" "/Shapes_set";
	setAttr ".rns" -type "string" "NS_SetWithSetEdits";
createNode pxrUsdReferenceAssembly -n "SetWithComponentEdits" -p "UsdExportAssemblyEditsTest";
	rename -uid "B3ABA940-0000-1380-5C9B-EDFA00000262";
	setAttr ".isc" yes;
	setAttr ".t" -type "double3" 200 -25 0 ;
	setAttr ".fp" -type "string" "./Shapes_set.usda";
	setAttr ".pp" -type "string" "/Shapes_set";
	setAttr ".rns" -type "string" "NS_SetWithComponentEdits";
createNode lightLinker -s -n "lightLinker1";
	rename -uid "9B3E3940-0000-5CB5-5CA5-184300000265";
	setAttr -s 2 ".lnk";
	setAttr -s 2 ".slnk";
createNode displayLayerManager -n "layerManager";
	rename -uid "9B3E3940-0000-5CB5-5CA5-184300000268";
createNode displayLayer -n "defaultLayer";
	rename -uid "B3ABA940-0000-1380-5C9B-ECE500000254";
createNode renderLayerManager -n "renderLayerManager";
	rename -uid "9B3E3940-0000-5CB5-5CA5-18430000026A";
createNode renderLayer -n "defaultRenderLayer";
	rename -uid "B3ABA940-0000-1380-5C9B-ECE500000256";
	setAttr ".g" yes;
createNode nodeGraphEditorInfo -n "MayaNodeEditorSavedTabsInfo";
	rename -uid "B3ABA940-0000-1380-5C9B-ECED00000258";
	setAttr ".tgi[0].tn" -type "string" "Untitled_1";
	setAttr ".tgi[0].vl" -type "double2" -911.9047256689239 -441.66664911641089 ;
	setAttr ".tgi[0].vh" -type "double2" 874.99996523062555 463.09521969348742 ;
	setAttr ".tgi[0].ni[0].nvs" 18304;
createNode hyperLayout -n "hyperLayout1";
	rename -uid "B3ABA940-0000-1380-5C9B-ED040000025A";
	setAttr ".ihi" 0;
	setAttr -s 2 ".hyp";
createNode hyperLayout -n "hyperLayout2";
	rename -uid "B3ABA940-0000-1380-5C9B-ED880000025D";
	setAttr ".ihi" 0;
createNode hyperLayout -n "hyperLayout3";
	rename -uid "B3ABA940-0000-1380-5C9B-EDB10000025F";
	setAttr ".ihi" 0;
createNode hyperLayout -n "hyperLayout4";
	rename -uid "B3ABA940-0000-1380-5C9B-EDCB00000261";
	setAttr ".ihi" 0;
createNode hyperLayout -n "hyperLayout5";
	rename -uid "B3ABA940-0000-1380-5C9B-EDFA00000263";
	setAttr ".ihi" 0;
createNode editsManager -n "editsManager1";
	rename -uid "A756D940-0000-3F83-5CA5-140900000298";
	setAttr ".edt" -type "sceneEdits" 
		"editsManager1"
		3
		"ModelWithEdits"
		"NS_SetWithComponentEdits:Shapes_3"
		"SetWithSetEdits"
		12
		1 0 2 "NS_ModelWithEdits:Geom|NS_ModelWithEdits:Cone" "rotate" 
		" -type \"double3\" 30 45 60"
		1 0 2 "NS_ModelWithEdits:Geom|NS_ModelWithEdits:Cube" "translate" 
		" -type \"double3\" 7 8 9"
		1 0 2 "NS_ModelWithEdits:Geom|NS_ModelWithEdits:Sphere" "translateZ" 
		" 10"
		1 0 2 "NS_ModelWithEdits:Geom|NS_ModelWithEdits:Torus" "rotateY" 
		" 45"
		1 2 2 "NS_SetWithSetEdits:Shapes_1" "rotate" " -type \"double3\" 30 45 60"
		
		1 2 2 "NS_SetWithSetEdits:Shapes_2" "translate" " -type \"double3\" 7 8 9"
		
		1 2 2 "NS_SetWithSetEdits:Shapes_3" "translateZ" " 10"
		1 2 2 "NS_SetWithSetEdits:Shapes_4" "rotateY" " 45"
		1 1 2 "NS_SetWithComponentEdits:Shapes_3|NS_SetWithComponentEdits:NS_Shapes_3:Geom|NS_SetWithComponentEdits:NS_Shapes_3:Cone" 
		"rotate" " -type \"double3\" 30 45 60"
		1 1 2 "NS_SetWithComponentEdits:Shapes_3|NS_SetWithComponentEdits:NS_Shapes_3:Geom|NS_SetWithComponentEdits:NS_Shapes_3:Cube" 
		"translate" " -type \"double3\" 7 8 9"
		1 1 2 "NS_SetWithComponentEdits:Shapes_3|NS_SetWithComponentEdits:NS_Shapes_3:Geom|NS_SetWithComponentEdits:NS_Shapes_3:Sphere" 
		"translateZ" " 10"
		1 1 2 "NS_SetWithComponentEdits:Shapes_3|NS_SetWithComponentEdits:NS_Shapes_3:Geom|NS_SetWithComponentEdits:NS_Shapes_3:Torus" 
		"rotateY" " 45";
select -ne :hardwareRenderingGlobals;
	setAttr ".otfna" -type "stringArray" 22 "NURBS Curves" "NURBS Surfaces" "Polygons" "Subdiv Surface" "Particles" "Particle Instance" "Fluids" "Strokes" "Image Planes" "UI" "Lights" "Cameras" "Locators" "Joints" "IK Handles" "Deformers" "Motion Trails" "Components" "Hair Systems" "Follicles" "Misc. UI" "Ornaments"  ;
	setAttr ".otfva" -type "Int32Array" 22 0 1 1 1 1 1
		 1 1 1 0 0 0 0 0 0 0 0 0
		 0 0 0 0 ;
	setAttr ".fprt" yes;
select -ne :renderPartition;
	setAttr -s 2 ".st";
select -ne :renderGlobalsList1;
select -ne :defaultShaderList1;
	setAttr -s 4 ".s";
select -ne :defaultRenderingList1;
select -ne :initialShadingGroup;
	setAttr ".ro" yes;
select -ne :initialParticleSE;
	setAttr ".ro" yes;
select -ne :defaultResolution;
	setAttr ".pa" 1;
select -ne :hardwareRenderGlobals;
	setAttr ".ctrs" 256;
	setAttr ".btrs" 512;
connectAttr "hyperLayout1.msg" "ModelNoEdits.hl";
connectAttr "hyperLayout2.msg" "ModelWithEdits.hl";
connectAttr "hyperLayout3.msg" "SetNoEdits.hl";
connectAttr "hyperLayout4.msg" "SetWithSetEdits.hl";
connectAttr "hyperLayout5.msg" "SetWithComponentEdits.hl";
relationship "link" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
connectAttr "layerManager.dli[0]" "defaultLayer.id";
connectAttr "renderLayerManager.rlmi[0]" "defaultRenderLayer.rlid";
connectAttr "defaultRenderLayer.msg" ":defaultRenderingList1.r" -na;
// End of UsdExportAssemblyEditsTest.ma
