//Maya ASCII 2016 scene
//Name: UsdExportUVSetsTest.ma
//Last modified: Thu, Oct 12, 2017 12:29:35 PM
//Codeset: UTF-8
requires maya "2016";
requires -nodeType "slimPartition" "px_slimmgr" "1.0";
requires -nodeType "mentalrayFramebuffer" -nodeType "mentalrayOptions" -nodeType "mentalrayGlobals"
		 -nodeType "mentalrayItemsList" -dataType "byteArray" "Mayatomr" "2016.0 - 3.13.1.10 ";
requires -nodeType "px_renderGlobals" "px_render" "1.0";
requires "stereoCamera" "10.0";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2016";
fileInfo "version" "2016";
fileInfo "cutIdentifier" "201610262200-1005964";
fileInfo "osv" "Linux 3.10.0-514.21.1.el7.x86_64 #1 SMP Sat Apr 22 02:41:35 EDT 2017 x86_64";
createNode transform -s -n "persp";
	rename -uid "109AE860-0000-148A-5708-3ED200000244";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 56.104252811372127 83.139523294239041 96.968093697256563 ;
	setAttr ".r" -type "double3" 42.682924915370414 0 848.79491399450217 ;
	setAttr ".rp" -type "double3" 1.6184267492025825e-15 -1.1095384915099624e-15 0 ;
	setAttr ".rpt" -type "double3" -1.085403572049829e-14 1.1373559648624964e-14 -9.8662122047328517e-15 ;
createNode camera -s -n "perspShape" -p "persp";
	rename -uid "109AE860-0000-148A-5708-3ED200000245";
	setAttr -k off ".v" no;
	setAttr ".ovr" 1.3;
	setAttr ".fl" 50;
	setAttr ".coi" 125.10680011092975;
	setAttr ".imn" -type "string" "persp";
	setAttr ".den" -type "string" "persp_depth";
	setAttr ".man" -type "string" "persp_mask";
	setAttr ".tp" -type "double3" -10 30 5 ;
	setAttr ".hc" -type "string" "viewSet -p %camera";
	setAttr ".dgm" no;
	setAttr ".dr" yes;
createNode transform -s -n "top";
	rename -uid "109AE860-0000-148A-5708-3ED200000246";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 0 107.30688039333857 ;
createNode camera -s -n "topShape" -p "top";
	rename -uid "109AE860-0000-148A-5708-3ED200000247";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 100.1;
	setAttr ".ow" 76.417438265352175;
	setAttr ".imn" -type "string" "top";
	setAttr ".den" -type "string" "top_depth";
	setAttr ".man" -type "string" "top_mask";
	setAttr ".hc" -type "string" "viewSet -t %camera";
	setAttr ".o" yes;
createNode transform -s -n "front";
	rename -uid "109AE860-0000-148A-5708-3ED200000248";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 -100.8782606252536 2.2204460492503131e-14 ;
	setAttr ".r" -type "double3" 89.999999999999986 0 0 ;
createNode camera -s -n "frontShape" -p "front";
	rename -uid "109AE860-0000-148A-5708-3ED200000249";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 100.1;
	setAttr ".ow" 87.305714812425137;
	setAttr ".imn" -type "string" "front";
	setAttr ".den" -type "string" "front_depth";
	setAttr ".man" -type "string" "front_mask";
	setAttr ".hc" -type "string" "viewSet -f %camera";
	setAttr ".o" yes;
createNode transform -s -n "side";
	rename -uid "109AE860-0000-148A-5708-3ED20000024A";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 100.97114707446906 -2.3092638912203256e-14 2.3092638912203256e-14 ;
	setAttr ".r" -type "double3" 90 4.7708320221952805e-14 89.999999999999986 ;
createNode camera -s -n "sideShape" -p "side";
	rename -uid "109AE860-0000-148A-5708-3ED20000024B";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 100.1;
	setAttr ".ow" 99.510321740747656;
	setAttr ".imn" -type "string" "side";
	setAttr ".den" -type "string" "side_depth";
	setAttr ".man" -type "string" "side_mask";
	setAttr ".hc" -type "string" "viewSet -s %camera";
	setAttr ".o" yes;
createNode transform -n "UsdExportUVSetsTest";
	rename -uid "109AE860-0000-148A-5708-3ED20000024C";
createNode transform -n "Geom" -p "UsdExportUVSetsTest";
	rename -uid "109AE860-0000-148A-5708-3ED20000024D";
createNode transform -n "CubeMeshes" -p "Geom";
	rename -uid "C81B5860-0000-05FF-57A1-039900000277";
createNode transform -n "EmptyDefaultUVSetCube" -p "CubeMeshes";
	rename -uid "20BEA8C0-0000-52C2-57B6-5BC700000290";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr ".rp" -type "double3" 0 0 5 ;
	setAttr ".sp" -type "double3" 0 0 5 ;
	setAttr ".allowPerFaceDisplayColors" yes;
createNode mesh -n "EmptyDefaultUVSetCubeShape" -p "EmptyDefaultUVSetCube";
	rename -uid "20BEA8C0-0000-52C2-57B6-5BC700000291";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr -k off ".v";
	setAttr -s 6 ".iog[0].og";
	setAttr ".iog[0].og[0].gcl" -type "componentList" 1 "f[2]";
	setAttr ".iog[0].og[1].gcl" -type "componentList" 1 "f[3]";
	setAttr ".iog[0].og[2].gcl" -type "componentList" 1 "f[4]";
	setAttr ".iog[0].og[3].gcl" -type "componentList" 1 "f[1]";
	setAttr ".iog[0].og[4].gcl" -type "componentList" 1 "f[5]";
	setAttr ".iog[0].og[5].gcl" -type "componentList" 1 "f[0]";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -5 -5 10 5 -5 10 -5 5 10 5 5 10 -5 5 0 5 5 0
		 -5 -5 0 5 -5 0;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		f 4 1 7 -3 -7
		f 4 2 9 -4 -9
		f 4 3 11 -1 -11
		f 4 -12 -10 -8 -6
		f 4 10 4 6 8;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".mgi" -type "string" "ID_2e5c42d3-5795-4c1e-a34e-0664b3adb423";
	setAttr ".allowPerFaceDisplayColors" yes;
createNode transform -n "DefaultUVSetCube" -p "CubeMeshes";
	rename -uid "C81B5860-0000-05FF-57A1-03BE00000281";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr ".t" -type "double3" 0 20 0 ;
	setAttr ".rp" -type "double3" 0 0 5 ;
	setAttr ".sp" -type "double3" 0 0 5 ;
	setAttr ".allowPerFaceDisplayColors" yes;
createNode mesh -n "DefaultUVSetCubeShape" -p "DefaultUVSetCube";
	rename -uid "C81B5860-0000-05FF-57A1-03BE00000282";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr -k off ".v";
	setAttr -s 6 ".iog[0].og";
	setAttr ".iog[0].og[0].gcl" -type "componentList" 1 "f[2]";
	setAttr ".iog[0].og[1].gcl" -type "componentList" 1 "f[3]";
	setAttr ".iog[0].og[2].gcl" -type "componentList" 1 "f[4]";
	setAttr ".iog[0].og[3].gcl" -type "componentList" 1 "f[1]";
	setAttr ".iog[0].og[4].gcl" -type "componentList" 1 "f[5]";
	setAttr ".iog[0].og[5].gcl" -type "componentList" 1 "f[0]";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -5 -5 10 5 -5 10 -5 5 10 5 5 10 -5 5 0 5 5 0
		 -5 -5 0 5 -5 0;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".mgi" -type "string" "ID_e7947aa6-f8ff-485d-93c0-579bb727d5b3";
	setAttr ".allowPerFaceDisplayColors" yes;
createNode transform -n "OneMissingFaceCube" -p "CubeMeshes";
	rename -uid "415B68C0-0000-16E9-57B7-9BB700000277";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr ".t" -type "double3" 0 40 0 ;
	setAttr ".rp" -type "double3" 0 0 5 ;
	setAttr ".sp" -type "double3" 0 0 5 ;
	setAttr ".allowPerFaceDisplayColors" yes;
createNode mesh -n "OneMissingFaceCubeShape" -p "OneMissingFaceCube";
	rename -uid "415B68C0-0000-16E9-57B7-9BB700000278";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".mgi" -type "string" "ID_00dcb39c-ae2c-4239-b0d8-945f9db8e612";
	setAttr ".allowPerFaceDisplayColors" yes;
createNode mesh -n "polySurfaceShape1" -p "OneMissingFaceCube";
	rename -uid "415B68C0-0000-16E9-57B7-9C390000027F";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr -s 6 ".iog[0].og";
	setAttr ".iog[0].og[0].gcl" -type "componentList" 1 "f[2]";
	setAttr ".iog[0].og[1].gcl" -type "componentList" 1 "f[3]";
	setAttr ".iog[0].og[2].gcl" -type "componentList" 1 "f[4]";
	setAttr ".iog[0].og[3].gcl" -type "componentList" 1 "f[1]";
	setAttr ".iog[0].og[4].gcl" -type "componentList" 1 "f[5]";
	setAttr ".iog[0].og[5].gcl" -type "componentList" 1 "f[0]";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -5 -5 10 5 -5 10 -5 5 10 5 5 10 -5 5 0 5 5 0
		 -5 -5 0 5 -5 0;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".mgi" -type "string" "ID_4741eed5-971c-474c-a167-c4218cf55b05";
	setAttr ".allowPerFaceDisplayColors" yes;
createNode transform -n "OneAssignedFaceCube" -p "CubeMeshes";
	rename -uid "415B68C0-0000-16E9-57B7-9C7200000280";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr ".t" -type "double3" 0 60 0 ;
	setAttr ".rp" -type "double3" 0 0 5 ;
	setAttr ".sp" -type "double3" 0 0 5 ;
	setAttr ".allowPerFaceDisplayColors" yes;
createNode mesh -n "OneAssignedFaceCubeShape" -p "OneAssignedFaceCube";
	rename -uid "415B68C0-0000-16E9-57B7-9C7200000281";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".mgi" -type "string" "ID_f8c2d19a-0a4f-48e1-b29c-dc491b67c152";
	setAttr ".allowPerFaceDisplayColors" yes;
createNode mesh -n "polySurfaceShape2" -p "OneAssignedFaceCube";
	rename -uid "415B68C0-0000-16E9-57B7-9CCB00000286";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr -s 6 ".iog[0].og";
	setAttr ".iog[0].og[0].gcl" -type "componentList" 1 "f[2]";
	setAttr ".iog[0].og[1].gcl" -type "componentList" 1 "f[3]";
	setAttr ".iog[0].og[2].gcl" -type "componentList" 1 "f[4]";
	setAttr ".iog[0].og[3].gcl" -type "componentList" 1 "f[1]";
	setAttr ".iog[0].og[4].gcl" -type "componentList" 1 "f[5]";
	setAttr ".iog[0].og[5].gcl" -type "componentList" 1 "f[0]";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -5 -5 10 5 -5 10 -5 5 10 5 5 10 -5 5 0 5 5 0
		 -5 -5 0 5 -5 0;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".mgi" -type "string" "ID_41632452-941c-445a-8955-d85daaf8813f";
	setAttr ".allowPerFaceDisplayColors" yes;
createNode transform -n "CompressibleUVSetsCube" -p "CubeMeshes";
	rename -uid "6BB5D8C0-0000-562B-57BB-88050000027F";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr ".t" -type "double3" -20 0 0 ;
	setAttr ".rp" -type "double3" 0 0 5 ;
	setAttr ".sp" -type "double3" 0 0 5 ;
	setAttr ".allowPerFaceDisplayColors" yes;
createNode mesh -n "CompressibleUVSetsCubeShape" -p "CompressibleUVSetsCube";
	rename -uid "6BB5D8C0-0000-562B-57BB-880500000280";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr -k off ".v";
	setAttr -s 6 ".iog[0].og";
	setAttr ".iog[0].og[0].gcl" -type "componentList" 1 "f[2]";
	setAttr ".iog[0].og[1].gcl" -type "componentList" 1 "f[3]";
	setAttr ".iog[0].og[2].gcl" -type "componentList" 1 "f[4]";
	setAttr ".iog[0].og[3].gcl" -type "componentList" 1 "f[1]";
	setAttr ".iog[0].og[4].gcl" -type "componentList" 1 "f[5]";
	setAttr ".iog[0].og[5].gcl" -type "componentList" 1 "f[0]";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr -s 4 ".uvst";
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".uvst[1].uvsn" -type "string" "ConstantInterpSet";
	setAttr -s 22 ".uvst[1].uvsp[0:21]" -type "float2" 0.25 0.25 0.25 0.25
		 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25
		 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25
		 0.25 0.25 0.25 0.25 0.25 0.25;
	setAttr ".uvst[2].uvsn" -type "string" "UniformInterpSet";
	setAttr -s 24 ".uvst[2].uvsp[0:23]" -type "float2" 0 0 0.1 0.1 0.2 0.2
		 0.30000001 0.30000001 0.40000001 0.40000001 0.5 0.5 0 0 0 0 0 0 0.1 0.1 0.1 0.1 0.1
		 0.1 0.2 0.2 0.2 0.2 0.2 0.2 0.30000001 0.30000001 0.30000001 0.30000001 0.30000001
		 0.30000001 0.40000001 0.40000001 0.40000001 0.40000001 0.40000001 0.40000001 0.5
		 0.5 0.5 0.5 0.5 0.5;
	setAttr ".uvst[3].uvsn" -type "string" "VertexInterpSet";
	setAttr -s 8 ".uvst[3].uvsp[0:7]" -type "float2" 0 0 0.1 0.1 0.2 0.2
		 0.30000001 0.30000001 0.40000001 0.40000001 0.5 0.5 0.60000002 0.60000002 0.69999999
		 0.69999999;
	setAttr ".cuvs" -type "string" "VertexInterpSet";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -5 -5 10 5 -5 10 -5 5 10 5 5 10 -5 5 0 5 5 0
		 -5 -5 0 5 -5 0;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 1 4 0 1 2 3
		mu 2 4 0 6 7 8
		mu 3 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 1 4 4 5 6 7
		mu 2 4 1 9 10 11
		mu 3 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 1 4 8 9 10 11
		mu 2 4 2 12 13 14
		mu 3 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 1 4 12 13 14 0
		mu 2 4 3 15 16 17
		mu 3 4 6 7 1 0
		f 4 -12 -10 -8 -6
		mu 1 4 15 16 17 18
		mu 2 4 4 18 19 20
		mu 3 4 1 7 5 3
		f 4 10 4 6 8
		mu 1 4 19 0 20 21
		mu 2 4 5 21 22 23
		mu 3 4 6 0 2 4;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr -s 4 ".pd";
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".pd[1]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".pd[2]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".pd[3]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".mgi" -type "string" "ID_a46bf4f0-e7ae-46d4-8a18-42305fb119fe";
	setAttr ".allowPerFaceDisplayColors" yes;
createNode transform -n "SharedFacesCube" -p "CubeMeshes";
	rename -uid "6BB5D8C0-0000-562B-57BB-96820000028B";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr ".t" -type "double3" -20 20 0 ;
	setAttr ".rp" -type "double3" 0 0 5 ;
	setAttr ".sp" -type "double3" 0 0 5 ;
	setAttr ".allowPerFaceDisplayColors" yes;
createNode mesh -n "SharedFacesCubeShape" -p "SharedFacesCube";
	rename -uid "6BB5D8C0-0000-562B-57BB-96820000028C";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -sn "allowPerFaceDisplayColors" -ln "allowPerFaceDisplayColors" 
		-min 0 -max 1 -at "bool";
	setAttr -k off ".v";
	setAttr -s 6 ".iog[0].og";
	setAttr ".iog[0].og[0].gcl" -type "componentList" 1 "f[2]";
	setAttr ".iog[0].og[1].gcl" -type "componentList" 1 "f[3]";
	setAttr ".iog[0].og[2].gcl" -type "componentList" 1 "f[4]";
	setAttr ".iog[0].og[3].gcl" -type "componentList" 1 "f[1]";
	setAttr ".iog[0].og[4].gcl" -type "componentList" 1 "f[5]";
	setAttr ".iog[0].og[5].gcl" -type "componentList" 1 "f[0]";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr -s 3 ".uvst";
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".uvst[1].uvsn" -type "string" "PairedFacesSet";
	setAttr -s 23 ".uvst[1].uvsp[0:22]" -type "float2" 0 0 0.5 0 0.5 0.5
		 0 0.5 0.5 0.5 1 0.5 1 1 0.5 1 0 0 0.5 0 0.5 0.5 0 0.5 0.5 0.5 1 0.5 1 1 0.5 1 0 0
		 0.5 0 0.5 0.5 0 0.5 0.5 0.5 1 0.5 1 1;
	setAttr ".uvst[2].uvsn" -type "string" "AllFacesSharedSet";
	setAttr -s 24 ".uvst[2].uvsp[0:23]" -type "float2" 0 0 1 0 1 1 0 1 0
		 0 1 0 1 1 0 1 0 0 1 0 1 1 0 1 0 0 1 0 1 1 0 1 0 0 1 0 1 1 0 1 0 0 1 0 1 1 0 1;
	setAttr ".cuvs" -type "string" "PairedFacesSet";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -5 -5 10 5 -5 10 -5 5 10 5 5 10 -5 5 0 5 5 0
		 -5 -5 0 5 -5 0;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 1 4 0 1 2 3
		mu 2 4 0 1 2 3
		f 4 1 7 -3 -7
		mu 1 4 4 5 6 7
		mu 2 4 4 5 6 7
		f 4 2 9 -4 -9
		mu 1 4 8 9 10 11
		mu 2 4 8 9 10 11
		f 4 3 11 -1 -11
		mu 1 4 12 13 14 15
		mu 2 4 12 13 14 15
		f 4 -12 -10 -8 -6
		mu 1 4 16 17 18 19
		mu 2 4 16 17 18 19
		f 4 10 4 6 8
		mu 1 4 20 21 22 7
		mu 2 4 20 21 22 23;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr -s 3 ".pd";
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".pd[1]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".pd[2]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".mgi" -type "string" "ID_406e2a39-80b1-474f-8921-d5b2c92513a8";
	setAttr ".allowPerFaceDisplayColors" yes;
createNode transform -n "BrokenUVs" -p "Geom";
	rename -uid "0D9708C0-0000-66E1-59DF-C01C00000288";
	addAttr -ci true -sn "USD_hidden" -ln "USD_hidden" -min 0 -max 1 -at "bool";
	setAttr ".r" -type "double3" 90.000000000000028 0 0 ;
	setAttr ".s" -type "double3" 5.4228220060229191 5.4228220060229191 5.4228220060229191 ;
	setAttr ".USD_hidden" yes;
createNode transform -n "box" -p "BrokenUVs";
	rename -uid "0D9708C0-0000-66E1-59DF-C01C00000289";
	setAttr ".rp" -type "double3" 0 -9.7389859848053266 1.2689892891351897 ;
	setAttr ".sp" -type "double3" 0 -9.7389859848053266 1.2689892891351897 ;
createNode mesh -n "boxShape" -p "box";
	rename -uid "0D9708C0-0000-66E1-59DF-C01C0000028A";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -sn "RMNCS__gprimHandleid" -ln "RMNCS__gprimHandleid" -dt "string";
	addAttr -ci true -sn "RMNCS__handleid" -ln "RMNCS__handleid" -dt "string";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.51456177320641516 0.49255575996386747 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 300 ".uvst[0].uvsp";
	setAttr ".uvst[0].uvsp[0:249]" -type "float2" 1 0 0 1 1 1 1 1 1 1 1 1 1 1
		 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 0 1 0 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 0
		 0 1 0 1 0 1 0 1 1 1 1 1 1 1 1 1 0 1 1 1 1 0 1 0 1 0 1 0 1 0 0 1 0 1 0 1 0 1 1 1 1
		 1 1 1 1 1 0 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 1 1 1 1 1 1 1 1 1 1
		 1 1 1 1 1 1 1 1 1 0 1 0 1 0 1 1 1 1 1 0 1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 1 0 1 0
		 1 0 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 0 1 0 1 1 1 1 1 1 0 1 1 1 1 1 0 1 0 1 1 1 1
		 1 0 1 1 1 1 1 1 1 1 1 0 0 1 0 1 0 1 0 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 0
		 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 0 1 0 1 1
		 1 1 1 0 1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 1 0 1 0 1 0 1 1 1 1 1 1 0 1 1 1 1 1 1 1
		 1 1 1 0 1 0 1 1 1 1 1 1 0 1 1 1 1 1 0 1 0 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 0 0 1 0 1
		 0 1 0 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 0 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1
		 1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 0 1 0 1 1 1 1 1 0 1 1 1 1 1 1
		 1 1 1 0 0 1 0 1 0 1 0 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 0 1 0 1 1;
	setAttr ".uvst[0].uvsp[250:299]" 1 1 0 1 0 1 1 1 1 1 0 1 0 1 1 1 1 1 1 1 1
		 1 1 1 1 1 0 0 1 0 1 0 1 0 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 0 1 0 1 1 1 1 1 1 1 1
		 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".lev" 4;
	setAttr -s 202 ".pt";
	setAttr ".pt[0:165]" -type "float3"  -8.3578283e-17 -8.8720875 0.76898932 1.708209 -8.8720875 
		0.76898926 0 -7.6014099 0.76898926 1.7778075 -8.8720875 0.76898926 1.7778075 -7.6014099 0.76898926 -4.1789141e-17 -7.4904943 0.76898926 
		1.7778075 -7.4904943 0.76898926 1.708209 -7.4904943 0.76898926 -6.4973158e-17 -7.4707847 0.76898932 1.7778075 -7.4707847 0.76898926 
		1.708209 -7.4707847 0.76898926 1.7868593 -8.8720875 0.76898932 1.7868593 -7.4707847 0.76898932 1.7868593 -7.4904943 0.76898926 
		1.7868593 -7.6014099 0.76898926 -4.1789141e-17 -7.5834012 0.76898926 1.7868593 -7.5834007 0.76898926 1.7778075 -7.5834007 0.76898926 
		1.7183772 -8.8720875 0.76898926 1.7183772 -7.4707847 0.76898926 1.7183772 -7.4904943 0.76898926 1.7336208 -7.5679517 0.76898926 
		-6.4335527e-17 -7.5834012 -2.4807422 1.7868593 -7.5834012 -2.4807422 1.7868593 -7.6014099 -2.4807422 -4.4888849e-17 -7.6014099 -2.4807422 
		-2.3184026e-17 -7.4707847 -2.4807422 1.7868593 -7.4707847 -2.4807422 1.7868593 -7.4904943 -2.4807422 -1.8716717e-17 -7.4904943 -2.4807422 
		1.7183772 -7.4707847 -2.4807422 1.7183772 -7.4904943 -2.4807422 1.7778075 -7.4904943 -2.4807422 1.7778075 -7.4707847 -2.4807422 
		-1.6352648e-17 -8.8720875 -2.4807422 1.7868593 -8.8720875 -2.4807422 1.7183772 -8.8720875 -2.4807422 1.7778075 -8.8720875 -2.4807422 
		1.7778075 -7.5834012 -2.4807422 1.7778075 -7.6014099 -2.4807422 1.7336208 -7.5679517 -2.4807422 1.708209 -7.4707847 -2.4807422 
		1.708209 -8.8720875 -2.4807422 1.708209 -7.4904943 -2.4807422 1.7591546 -8.8720875 0.76898926 1.7591546 -7.4707847 0.76898926 
		1.7591546 -7.4904943 0.76898926 1.7591546 -7.5829258 0.76898926 1.7591546 -7.5829258 -2.4807422 1.7591546 -7.4904943 -2.4807422 
		1.7591546 -7.4707847 -2.4807422 1.7591546 -8.8720875 -2.4807422 -6.0698745e-17 -7.5245609 0.76898926 1.7868593 -7.5245609 0.76898926 
		1.7778075 -7.5245609 0.76898926 1.7591546 -7.5245609 0.76898926 1.7187064 -7.5245609 0.76898926 1.7187064 -7.5245609 -2.4807422 
		1.7591546 -7.5245609 -2.4807422 1.7778075 -7.5245609 -2.4807422 1.7868593 -7.5245609 -2.4807422 -3.7819207e-17 -7.5245609 -2.4807422 
		-1.7868605 -7.6014099 -2.4807422 -1.7868605 -7.5834012 -2.4807422 -1.7868605 -7.4904943 -2.4807422 -1.7868605 -7.4707847 -2.4807422 
		-1.7591559 -7.4904943 -2.4807422 -1.7591559 -7.4707847 -2.4807422 -1.7778084 -7.4707847 -2.4807422 -1.7778084 -7.4904943 -2.4807422 
		-1.7868605 -8.8720875 -2.4807422 -1.7591559 -8.8720875 -2.4807422 -1.7778084 -8.8720875 -2.4807422 -1.7778084 -7.6014099 -2.4807422 
		-1.7778084 -7.5834012 -2.4807422 -1.7868605 -7.5245609 -2.4807422 -1.7778084 -7.5245609 -2.4807422 -1.7591559 -7.5829258 -2.4807422 
		-1.7591559 -7.5245609 -2.4807422 -1.7082099 -7.4707847 -2.4807422 -1.7082099 -8.8720875 -2.4807422 -1.7183784 -8.8720875 -2.4807422 
		-1.7183784 -7.4707847 -2.4807422 -1.7082099 -7.4904943 -2.4807422 -1.7183784 -7.4904943 -2.4807422 -1.7868606 -7.5834012 0.76898926 
		-1.7868606 -7.6014099 0.76898926 -1.7868606 -7.4707847 0.76898932 -1.7868606 -7.4904943 0.76898926 -1.7591559 -7.4707847 0.76898926 
		-1.7591559 -7.4904943 0.76898926 -1.7778084 -7.4904943 0.76898926 -1.7778084 -7.4707847 0.76898926 -1.7868605 -8.8720875 0.76898932 
		-1.7591559 -8.8720875 0.76898926 -1.7778084 -8.8720875 0.76898926 -1.7778084 -7.5834012 0.76898926 -1.7778084 -7.6014099 0.76898926 
		-1.7868606 -7.5245609 0.76898926 -1.7778084 -7.5245609 0.76898926 -1.7591559 -7.5245609 0.76898926 -1.7591559 -7.5829258 0.76898926 
		-1.7082099 -7.4707847 0.76898926 -1.7183784 -7.4707847 0.76898926 -1.7183784 -8.8720875 0.76898926 -1.7082099 -8.8720875 0.76898926 
		-1.7082099 -7.4904943 0.76898926 -1.7183784 -7.4904943 0.76898926 -1.7187077 -7.5245609 0.76898926 -1.7187077 -7.5245609 -2.4807422 
		-1.7336217 -7.5679517 -2.4807422 -1.7336217 -7.5679517 0.76898926 -1.0612468e-16 -10.204079 -2.4807422 -9.6114238e-17 -10.186069 -2.4807422 
		1.7868593 -10.186069 -2.4807422 1.7868593 -10.204079 -2.4807422 2.7762904e-17 -10.316693 -2.4807422 4.3557143e-18 -10.296985 -2.4807422 
		1.7868593 -10.296985 -2.4807422 1.7868593 -10.316693 -2.4807422 1.7591546 -10.296985 -2.4807422 1.7591546 -10.316693 -2.4807422 
		1.7778075 -10.316693 -2.4807422 1.7778075 -10.296985 -2.4807422 1.7778075 -10.186069 -2.4807422 1.7778075 -10.204079 -2.4807422 
		1.7868593 -10.262918 -2.4807422 -7.9608352e-17 -10.262918 -2.4807422 1.7778075 -10.262918 -2.4807422 1.7591546 -10.204553 -2.4807422 
		1.7591546 -10.262918 -2.4807422 1.708209 -10.316693 -2.4807422 1.7183772 -10.316693 -2.4807422 1.708209 -10.296985 -2.4807422 
		1.7183772 -10.296985 -2.4807422 -4.1789141e-17 -10.204079 0.76898926 1.7868593 -10.204079 0.76898926 1.7868593 -10.186069 0.76898926 
		-5.3029963e-17 -10.186069 0.76898926 -6.4973158e-17 -10.316693 0.76898932 1.7868593 -10.316693 0.76898932 1.7868593 -10.296985 0.76898926 
		-4.1789141e-17 -10.296985 0.76898926 1.7591546 -10.316693 0.76898926 1.7591546 -10.296985 0.76898926 1.7778075 -10.296985 0.76898926 
		1.7778075 -10.316693 0.76898926 1.7778075 -10.204079 0.76898926 1.7778075 -10.186069 0.76898926 -6.0698745e-17 -10.262918 0.76898926 
		1.7868593 -10.262918 0.76898926 1.7778075 -10.262918 0.76898926 1.7591546 -10.262918 0.76898926 1.7591546 -10.204553 0.76898926 
		1.708209 -10.316693 0.76898926 1.7183772 -10.316693 0.76898926 1.708209 -10.296985 0.76898926 1.7183772 -10.296985 0.76898926 
		1.7187064 -10.262918 0.76898926 1.7187064 -10.262918 -2.4807422 1.7336208 -10.219527 -2.4807422 1.7336208 -10.219527 0.76898926 
		-1.7868605 -10.204079 -2.4807422 -1.7868605 -10.186069 -2.4807422 -1.7868605 -10.316693 -2.4807422 -1.7868605 -10.296985 -2.4807422;
	setAttr ".pt[166:201]" -1.7591559 -10.296985 -2.4807422 -1.7778084 -10.296985 -2.4807422 -1.7778084 -10.316693 
		-2.4807422 -1.7591559 -10.316693 -2.4807422 -1.7778084 -10.204079 -2.4807422 -1.7778084 -10.186069 -2.4807422 -1.7868605 -10.262918 
		-2.4807422 -1.7778084 -10.262918 -2.4807422 -1.7591559 -10.204553 -2.4807422 -1.7591559 -10.262918 -2.4807422 -1.7082099 -10.316693 
		-2.4807422 -1.7183784 -10.316693 -2.4807422 -1.7082099 -10.296985 -2.4807422 -1.7183784 -10.296985 -2.4807422 -1.7868605 -10.186069 
		0.76898926 -1.7868605 -10.204079 0.76898926 -1.7868605 -10.296985 0.76898926 -1.7868605 -10.316693 0.76898932 -1.7591559 -10.316693 
		0.76898926 -1.7778084 -10.316693 0.76898926 -1.7778084 -10.296985 0.76898926 -1.7591559 -10.296985 0.76898926 -1.7778084 -10.186069 
		0.76898926 -1.7778084 -10.204079 0.76898926 -1.7868605 -10.262918 0.76898926 -1.7778084 -10.262918 0.76898926 -1.7591559 -10.204553 
		0.76898926 -1.7591559 -10.262918 0.76898926 -1.7082099 -10.316693 0.76898926 -1.7183784 -10.316693 0.76898926 -1.7082099 -10.296985 
		0.76898926 -1.7183784 -10.296985 0.76898926 -1.7187077 -10.262918 0.76898926 -1.7336217 -10.219527 0.76898926 -1.7336217 -10.219527 
		-2.4807422 -1.7187077 -10.262918 -2.4807422;
	setAttr -s 202 ".vt";
	setAttr ".vt[0:165]"  -2.744402e-17 -0.028458595 0.50000006 0.61315286 -0.028458595 0.50000006
		 0 0.50000143 0.50000006 0.54355145 -0.028458595 0.50000006 0.54355145 0.50000143 0.50000006
		 -1.372201e-17 0.38908339 0.50000006 0.54355145 0.38908339 0.50000006 0.61315286 0.38908339 0.50000006
		 -2.1334785e-17 0.3693738 0.50000006 0.54355145 0.3693738 0.50000006 0.61315286 0.3693738 0.50000006
		 0.53449911 -0.028458595 0.50000006 0.53449911 0.3693738 0.50000006 0.53449911 0.38908339 0.50000006
		 0.53449911 0.50000143 0.50000006 -1.372201e-17 0.48199248 0.50000006 0.53449911 0.48199248 0.50000006
		 0.54355145 0.48199248 0.50000006 0.60298401 -0.028458595 0.50000006 0.60298401 0.3693738 0.50000006
		 0.60298401 0.38908339 0.50000006 0.58774 0.46654248 0.50000006 -2.112541e-17 0.48199248 0.45681721
		 0.53449911 0.48199248 0.45681709 0.53449911 0.50000143 0.45681721 -1.4739839e-17 0.50000143 0.45681721
		 -7.6127773e-18 0.3693738 0.45681721 0.53449911 0.3693738 0.45681709 0.53449911 0.38908339 0.45681709
		 -6.1458779e-18 0.38908339 0.45681721 0.60298401 0.3693738 0.45681709 0.60298401 0.38908339 0.45681721
		 0.54355145 0.38908339 0.45681709 0.54355145 0.3693738 0.45681709 -5.3696053e-18 -0.028458595 0.45681721
		 0.53449911 -0.028458595 0.45681721 0.60298401 -0.028458595 0.45681721 0.54355145 -0.028458595 0.45681721
		 0.54355145 0.48199248 0.45681721 0.54355145 0.50000143 0.45681721 0.58774 0.46654248 0.45681721
		 0.61315286 0.3693738 0.45681721 0.61315286 -0.028458595 0.45681721 0.61315286 0.38908339 0.45681721
		 0.56220496 -0.028458595 0.50000006 0.56220496 0.3693738 0.50000006 0.56220496 0.38908339 0.50000006
		 0.56220496 0.48151731 0.50000006 0.56220496 0.48151731 0.45681721 0.56220496 0.38908339 0.45681721
		 0.56220496 0.3693738 0.45681709 0.56220496 -0.028458595 0.45681721 -1.9931225e-17 0.42315102 0.50000006
		 0.53449911 0.42315102 0.50000006 0.54355145 0.42315102 0.50000006 0.56220496 0.42315102 0.50000006
		 0.6026547 0.42315102 0.50000006 0.6026547 0.42315102 0.45681721 0.56220496 0.42315102 0.45681721
		 0.54355145 0.42315102 0.45681721 0.53449911 0.42315102 0.45681709 -1.241843e-17 0.42315102 0.45681721
		 -0.53449911 0.50000143 0.45681721 -0.53449911 0.48199248 0.45681709 -0.53449911 0.38908339 0.45681709
		 -0.53449911 0.3693738 0.45681709 -0.56220496 0.38908339 0.45681721 -0.56220496 0.3693738 0.45681709
		 -0.54355145 0.3693738 0.45681709 -0.54355145 0.38908339 0.45681709 -0.53449911 -0.028458595 0.45681721
		 -0.56220496 -0.028458595 0.45681721 -0.54355145 -0.028458595 0.45681721 -0.54355145 0.50000143 0.45681721
		 -0.54355145 0.48199248 0.45681721 -0.53449911 0.42315102 0.45681709 -0.54355145 0.42315102 0.45681721
		 -0.56220496 0.48151731 0.45681721 -0.56220496 0.42315102 0.45681721 -0.61315286 0.3693738 0.45681721
		 -0.61315286 -0.028458595 0.45681721 -0.60298401 -0.028458595 0.45681721 -0.60298401 0.3693738 0.45681709
		 -0.61315286 0.38908339 0.45681721 -0.60298401 0.38908339 0.45681721 -0.53449911 0.48199248 0.50000006
		 -0.53449911 0.50000143 0.50000006 -0.53449911 0.3693738 0.50000006 -0.53449911 0.38908339 0.50000006
		 -0.56220496 0.3693738 0.50000006 -0.56220496 0.38908339 0.50000006 -0.54355145 0.38908339 0.50000006
		 -0.54355145 0.3693738 0.50000006 -0.53449911 -0.028458595 0.50000006 -0.56220496 -0.028458595 0.50000006
		 -0.54355145 -0.028458595 0.50000006 -0.54355145 0.48199248 0.50000006 -0.54355145 0.50000143 0.50000006
		 -0.53449911 0.42315102 0.50000006 -0.54355145 0.42315102 0.50000006 -0.56220496 0.42315102 0.50000006
		 -0.56220496 0.48151731 0.50000006 -0.61315286 0.3693738 0.50000006 -0.60298401 0.3693738 0.50000006
		 -0.60298401 -0.028458595 0.50000006 -0.61315286 -0.028458595 0.50000006 -0.61315286 0.38908339 0.50000006
		 -0.60298401 0.38908339 0.50000006 -0.6026547 0.42315102 0.50000006 -0.6026547 0.42315102 0.45681721
		 -0.58774 0.46654248 0.45681721 -0.58774 0.46654248 0.50000006 -3.4847422e-17 -0.53890944 0.45681721
		 -3.1560365e-17 -0.55691814 0.45681721 0.53449911 -0.55691814 0.45681721 0.53449911 -0.53890944 0.45681709
		 9.116312e-18 -0.42629099 0.45681721 1.4302556e-18 -0.4460001 0.45681721 0.53449911 -0.4460001 0.45681709
		 0.53449911 -0.42629099 0.45681709 0.56220496 -0.4460001 0.45681721 0.56220496 -0.42629099 0.45681709
		 0.54355145 -0.42629099 0.45681709 0.54355145 -0.4460001 0.45681709 0.54355145 -0.55691814 0.45681721
		 0.54355145 -0.53890944 0.45681721 0.53449911 -0.48006773 0.45681709 -2.6140441e-17 -0.48006773 0.45681721
		 0.54355145 -0.48006773 0.45681721 0.56220496 -0.53843451 0.45681721 0.56220496 -0.48006773 0.45681721
		 0.61315286 -0.42629099 0.45681721 0.60298401 -0.42629099 0.45681709 0.61315286 -0.4460001 0.45681721
		 0.60298401 -0.4460001 0.45681721 -1.372201e-17 -0.53890944 0.50000006 0.53449911 -0.53890944 0.50000006
		 0.53449911 -0.55691814 0.50000006 -1.741308e-17 -0.55691814 0.50000006 -2.1334785e-17 -0.42629099 0.50000006
		 0.53449911 -0.42629099 0.50000006 0.53449911 -0.4460001 0.50000006 -1.372201e-17 -0.4460001 0.50000006
		 0.56220496 -0.42629099 0.50000006 0.56220496 -0.4460001 0.50000006 0.54355145 -0.4460001 0.50000006
		 0.54355145 -0.42629099 0.50000006 0.54355145 -0.53890944 0.50000006 0.54355145 -0.55691814 0.50000006
		 -1.9931225e-17 -0.48006773 0.50000006 0.53449911 -0.48006773 0.50000006 0.54355145 -0.48006773 0.50000006
		 0.56220496 -0.48006773 0.50000006 0.56220496 -0.53843451 0.50000006 0.61315286 -0.42629099 0.50000006
		 0.60298401 -0.42629099 0.50000006 0.61315286 -0.4460001 0.50000006 0.60298401 -0.4460001 0.50000006
		 0.6026547 -0.48006773 0.50000006 0.6026547 -0.48006773 0.45681721 0.58774 -0.52345943 0.45681721
		 0.58774 -0.52345943 0.50000006 -0.53449911 -0.53890944 0.45681709 -0.53449911 -0.55691814 0.45681721
		 -0.53449911 -0.42629099 0.45681709 -0.53449911 -0.4460001 0.45681709;
	setAttr ".vt[166:201]" -0.56220496 -0.4460001 0.45681721 -0.54355145 -0.4460001 0.45681709
		 -0.54355145 -0.42629099 0.45681709 -0.56220496 -0.42629099 0.45681709 -0.54355145 -0.53890944 0.45681721
		 -0.54355145 -0.55691814 0.45681721 -0.53449911 -0.48006773 0.45681709 -0.54355145 -0.48006773 0.45681721
		 -0.56220496 -0.53843451 0.45681721 -0.56220496 -0.48006773 0.45681721 -0.61315286 -0.42629099 0.45681721
		 -0.60298401 -0.42629099 0.45681709 -0.61315286 -0.4460001 0.45681721 -0.60298401 -0.4460001 0.45681721
		 -0.53449911 -0.55691814 0.50000006 -0.53449911 -0.53890944 0.50000006 -0.53449911 -0.4460001 0.50000006
		 -0.53449911 -0.42629099 0.50000006 -0.56220496 -0.42629099 0.50000006 -0.54355145 -0.42629099 0.50000006
		 -0.54355145 -0.4460001 0.50000006 -0.56220496 -0.4460001 0.50000006 -0.54355145 -0.55691814 0.50000006
		 -0.54355145 -0.53890944 0.50000006 -0.53449911 -0.48006773 0.50000006 -0.54355145 -0.48006773 0.50000006
		 -0.56220496 -0.53843451 0.50000006 -0.56220496 -0.48006773 0.50000006 -0.61315286 -0.42629099 0.50000006
		 -0.60298401 -0.42629099 0.50000006 -0.61315286 -0.4460001 0.50000006 -0.60298401 -0.4460001 0.50000006
		 -0.6026547 -0.48006773 0.50000006 -0.58774 -0.52345943 0.50000006 -0.58774 -0.52345943 0.45681721
		 -0.6026547 -0.48006773 0.45681721;
	setAttr -s 400 ".ed";
	setAttr ".ed[0:165]"  0 11 0 2 14 0 0 8 0 1 10 0 3 44 0 3 9 1 5 52 0 6 54 1
		 5 13 1 6 46 1 8 5 0 9 6 1 10 7 0 8 12 1 9 45 1 11 3 0 12 9 1 13 6 1 14 4 0 11 12 1
		 12 13 1 13 53 1 15 2 0 16 14 1 17 4 0 15 16 1 16 17 1 17 47 0 18 1 0 19 10 1 20 7 0
		 18 19 1 19 20 1 20 56 0 22 23 1 14 24 1 23 24 1 2 25 0 25 24 0 22 25 0 26 27 1 27 28 1
		 29 28 1 26 29 0 20 31 0 30 31 1 32 49 1 33 32 1 33 50 1 34 35 0 35 27 1 34 26 0 36 30 1
		 37 33 1 37 51 0 35 37 0 27 33 1 28 32 1 17 38 0 23 38 1 4 39 0 38 39 0 24 39 0 28 60 1
		 29 61 0 32 59 1 21 40 0 31 57 0 38 48 0 10 41 1 30 41 1 1 42 0 36 42 0 42 41 0 7 43 0
		 31 43 0 41 43 0 44 18 0 45 19 1 46 20 1 47 21 0 48 40 0 49 31 1 50 30 1 51 36 0 44 45 1
		 45 46 1 46 55 1 47 48 1 48 58 1 49 50 1 50 51 1 52 15 0 53 16 1 54 17 1 55 47 1 56 21 0
		 57 40 0 58 49 1 59 38 1 60 23 1 61 22 0 52 53 1 53 54 1 54 55 1 55 56 1 56 57 1 57 58 1
		 58 59 1 59 60 1 60 61 1 25 62 0 63 62 1 22 63 1 29 64 1 65 64 1 26 65 1 66 67 1 68 67 1
		 68 69 1 69 66 1 70 65 1 34 70 0 67 71 1 72 71 0 72 68 1 65 68 1 70 72 0 64 69 1 62 73 0
		 74 73 0 63 74 1 75 61 1 75 63 1 76 74 1 76 75 1 77 78 1 78 76 1 74 77 0 80 79 0 81 80 0
		 81 82 1 82 79 1 79 83 0 82 84 1 84 83 0 15 85 1 85 86 1 2 86 0 8 87 1 87 88 1 5 88 1
		 89 90 1 91 90 1 92 91 1 92 89 1 0 93 0 93 87 1 94 89 1 95 92 1 95 94 0 93 95 0 87 92 1
		 88 91 1 85 96 1 96 97 0;
	setAttr ".ed[166:331]" 86 97 0 52 98 1 98 85 1 98 99 1 99 96 1 99 100 1 100 101 1
		 96 101 0 103 102 1 104 103 1 104 105 0 105 102 0 107 106 0 103 107 1 102 106 0 86 62 1
		 96 74 0 97 73 0 108 109 1 109 110 0 111 110 0 108 111 0 101 77 1 105 80 0 102 79 1
		 106 83 0 107 84 0 89 103 1 94 104 0 90 107 1 100 108 1 101 111 0 77 110 0 109 78 1
		 67 82 1 66 84 1 71 81 0 88 98 1 91 99 1 90 100 1 107 108 0 84 109 0 78 66 1 69 76 1
		 64 75 1 112 113 0 113 114 0 115 114 1 112 115 1 116 117 0 117 118 1 119 118 1 116 119 1
		 120 121 1 122 121 1 122 123 1 123 120 1 34 116 0 35 119 1 121 51 1 37 122 1 119 122 1
		 118 123 1 114 124 0 125 124 0 115 125 1 126 127 1 127 112 0 126 115 1 128 125 1 128 126 1
		 129 130 1 130 128 1 125 129 0 42 131 0 36 132 1 132 131 1 131 133 0 132 134 1 134 133 0
		 135 136 1 136 137 1 138 137 0 135 138 0 139 140 1 140 141 1 142 141 1 139 142 0 143 144 1
		 145 144 1 146 145 1 146 143 1 11 140 1 0 139 0 44 143 1 3 146 1 140 146 1 141 145 1
		 136 147 1 147 148 0 137 148 0 149 150 1 150 136 1 149 135 0 150 151 1 151 147 1 151 152 1
		 152 153 1 147 153 0 155 154 1 18 155 1 1 154 0 157 156 0 155 157 1 154 156 0 137 114 1
		 138 113 0 147 125 0 148 124 0 158 159 1 159 160 0 161 160 0 158 161 0 153 129 1 154 131 1
		 156 133 0 157 134 0 143 155 1 144 157 1 152 158 1 153 161 0 129 160 0 159 130 1 121 132 1
		 120 134 1 141 150 1 142 149 0 145 151 1 144 152 1 157 158 0 134 159 0 130 120 1 123 128 1
		 118 126 1 117 127 0 112 162 1 162 163 1 113 163 0 116 164 1 164 165 1 117 165 1 167 166 1
		 168 167 1 168 169 1 166 169 1 70 164 1 72 168 1 169 71 1 164 168 1 165 167 1 162 170 1
		 170 171 0 163 171 0 172 162 1 172 127 1 173 172 1;
	setAttr ".ed[332:399]" 173 170 1 170 174 0 175 173 1 174 175 1 177 176 1 81 177 1
		 80 176 0 179 178 0 177 179 1 176 178 0 138 180 0 181 180 1 135 181 1 142 182 1 183 182 1
		 139 183 1 185 184 1 185 186 1 186 187 1 184 187 1 93 183 1 95 185 1 94 184 1 183 185 1
		 182 186 1 180 188 0 189 188 0 181 189 1 190 181 1 149 190 1 191 189 1 190 191 1 189 192 0
		 193 192 1 191 193 1 105 194 0 104 195 1 195 194 1 194 196 0 195 197 1 197 196 0 180 163 1
		 188 171 0 189 170 0 198 199 0 199 200 0 201 200 0 198 201 1 192 174 1 194 176 1 197 179 0
		 196 178 0 184 195 1 187 197 1 192 199 0 193 198 1 174 200 0 201 175 1 166 179 1 169 177 1
		 182 190 1 186 191 1 187 193 1 197 198 0 179 201 0 175 166 1 167 173 1 165 172 1;
	setAttr -s 200 -ch 800 ".fc[0:199]" -type "polyFaces" 
		f 4 39 38 -37 -35
		mu 0 4 39 38 37 10
		f 4 43 42 -42 -41
		mu 0 4 41 40 8 7
		f 4 90 -49 47 46
		mu 0 4 59 61 5 3
		f 4 51 40 -51 -50
		mu 0 4 42 41 7 43
		f 4 91 -55 53 48
		mu 0 4 61 62 44 5
		f 4 56 -54 -56 50
		mu 0 4 7 5 44 43
		f 4 57 -48 -57 41
		mu 0 4 8 3 5 7
		f 4 62 -62 -60 36
		mu 0 4 37 47 46 10
		f 4 110 101 34 -101
		mu 0 4 72 73 39 10
		f 4 -100 109 100 59
		mu 0 4 46 71 72 10
		f 4 89 108 99 68
		mu 0 4 58 70 71 46
		f 4 -74 -73 52 70
		mu 0 4 51 50 45 12
		f 4 -77 -71 45 75
		mu 0 4 52 51 12 48
		f 4 25 23 -2 -23
		mu 0 4 15 16 17 18
		f 4 13 20 -9 -11
		mu 0 4 19 20 21 22
		f 4 86 -10 -12 14
		mu 0 4 54 55 25 26
		f 4 0 19 -14 -3
		mu 0 4 27 28 20 19
		f 4 85 -15 -6 4
		mu 0 4 53 54 26 30
		f 4 -20 15 5 -17
		mu 0 4 20 28 30 26
		f 4 -21 16 11 -18
		mu 0 4 21 20 26 25
		f 4 -24 26 24 -19
		mu 0 4 17 16 31 32
		f 4 102 93 -26 -93
		mu 0 4 63 64 16 15
		f 4 -27 -94 103 94
		mu 0 4 31 16 64 65
		f 4 104 95 -28 -95
		mu 0 4 65 66 57 31
		f 4 -30 -32 28 3
		mu 0 4 34 23 29 35
		f 4 -31 -33 29 12
		mu 0 4 36 24 23 34
		f 4 35 -39 -38 1
		mu 0 4 9 37 38 1
		f 4 58 61 -61 -25
		mu 0 4 11 46 47 2
		f 4 60 -63 -36 18
		mu 0 4 2 47 37 9
		f 4 106 97 -67 -97
		mu 0 4 68 69 49 14
		f 4 88 -69 -59 27
		mu 0 4 56 58 46 11
		f 4 71 73 -70 -4
		mu 0 4 0 50 51 6
		f 4 74 -76 -45 30
		mu 0 4 4 52 48 13
		f 4 69 76 -75 -13
		mu 0 4 6 51 52 4
		f 4 31 -79 -86 77
		mu 0 4 29 23 54 53
		f 4 32 -80 -87 78
		mu 0 4 23 24 55 54
		f 4 105 96 -81 -96
		mu 0 4 66 67 33 57
		f 4 66 -82 -89 80
		mu 0 4 14 49 58 56
		f 4 107 -90 81 -98
		mu 0 4 69 70 58 49
		f 4 -84 -91 82 -46
		mu 0 4 12 61 59 296
		f 4 -85 -92 83 -53
		mu 0 4 45 62 61 12
		f 4 8 21 -103 -7
		mu 0 4 22 21 64 63
		f 4 -104 -22 17 7
		mu 0 4 65 64 21 25
		f 4 87 -105 -8 9
		mu 0 4 55 66 65 25
		f 4 33 -106 -88 79
		mu 0 4 24 67 66 55
		f 4 44 67 -107 -34
		mu 0 4 13 48 69 68
		f 4 -83 -99 -108 -68
		mu 0 4 48 60 70 69
		f 4 -109 98 -47 65
		mu 0 4 71 70 60 3
		f 4 -110 -66 -58 63
		mu 0 4 72 71 3 8
		f 4 64 -111 -64 -43
		mu 0 4 40 73 72 8
		f 4 113 112 -112 -40
		mu 0 4 74 77 76 75
		f 4 116 115 -115 -44
		mu 0 4 78 81 80 79
		f 4 -121 -120 118 -118
		mu 0 4 82 85 84 83
		f 4 122 121 -117 -52
		mu 0 4 86 87 81 78
		f 4 -119 -126 124 -124
		mu 0 4 83 84 89 88
		f 4 -122 127 125 -127
		mu 0 4 81 87 89 84
		f 4 -116 126 119 -129
		mu 0 4 80 81 84 85
		f 4 -113 131 130 -130
		mu 0 4 76 77 91 90
		f 4 133 -114 -102 -133
		mu 0 4 92 77 74 93
		f 4 -132 -134 -136 134
		mu 0 4 91 77 92 94
		f 4 -139 -135 -138 -137
		mu 0 4 95 91 94 96
		f 4 -143 -142 140 139
		mu 0 4 97 100 99 98
		f 4 -146 -145 142 143
		mu 0 4 101 102 100 97
		f 4 22 148 -148 -147
		mu 0 4 103 106 105 104
		f 4 10 151 -151 -150
		mu 0 4 107 110 109 108
		f 4 -156 154 153 -153
		mu 0 4 111 114 113 112
		f 4 2 149 -158 -157
		mu 0 4 115 107 108 116
		f 4 -161 159 155 -159
		mu 0 4 117 118 114 111
		f 4 162 -160 -162 157
		mu 0 4 108 114 118 116
		f 4 163 -155 -163 150
		mu 0 4 109 113 114 108
		f 4 166 -166 -165 147
		mu 0 4 105 120 119 104
		f 4 92 146 -169 -168
		mu 0 4 121 103 104 122
		f 4 -171 -170 168 164
		mu 0 4 119 123 122 104
		f 4 170 173 -173 -172
		mu 0 4 123 119 125 124
		f 4 -178 -177 175 174
		mu 0 4 126 129 128 127
		f 4 -181 -175 179 178
		mu 0 4 130 126 127 131
		f 4 -149 37 111 -182
		mu 0 4 132 133 75 76
		f 4 165 183 -131 -183
		mu 0 4 134 135 90 91
		f 4 -167 181 129 -184
		mu 0 4 135 132 76 90
		f 4 187 186 -186 -185
		mu 0 4 136 139 138 137
		f 4 -174 182 138 -189
		mu 0 4 140 134 91 95
		f 4 177 190 -140 -190
		mu 0 4 141 142 97 98
		f 4 -179 192 145 -192
		mu 0 4 143 144 102 101
		f 4 180 191 -144 -191
		mu 0 4 142 143 101 97
		f 4 -195 158 193 -176
		mu 0 4 128 117 111 127
		f 4 -194 152 195 -180
		mu 0 4 127 111 112 131
		f 4 172 197 -188 -197
		mu 0 4 124 125 146 145
		f 4 -198 188 198 -187
		mu 0 4 139 140 95 138
		f 4 185 -199 136 -200
		mu 0 4 137 138 95 96
		f 4 144 -202 117 200
		mu 0 4 100 297 82 83
		f 4 141 -201 123 202
		mu 0 4 99 100 83 88
		f 4 6 167 -204 -152
		mu 0 4 110 121 122 109
		f 4 -205 -164 203 169
		mu 0 4 123 113 109 122
		f 4 -154 204 171 -206
		mu 0 4 112 113 123 124
		f 4 -196 205 196 -207
		mu 0 4 131 112 124 145
		f 4 206 184 -208 -193
		mu 0 4 144 136 137 102
		f 4 207 199 208 201
		mu 0 4 102 137 96 147
		f 4 -210 120 -209 137
		mu 0 4 94 85 147 96
		f 4 -211 128 209 135
		mu 0 4 92 80 85 94
		f 4 114 210 132 -65
		mu 0 4 79 80 92 93
		f 4 214 213 -213 -212
		mu 0 4 148 151 150 149
		f 4 218 217 -217 -216
		mu 0 4 152 155 154 153
		f 4 -223 -222 220 -220
		mu 0 4 156 159 158 157
		f 4 49 224 -219 -224
		mu 0 4 160 161 155 152
		f 4 -221 -227 54 -226
		mu 0 4 157 158 163 162
		f 4 -225 55 226 -228
		mu 0 4 155 161 163 158
		f 4 -218 227 221 -229
		mu 0 4 154 155 158 159
		f 4 -214 231 230 -230
		mu 0 4 150 151 165 164
		f 4 234 -215 -234 -233
		mu 0 4 166 151 148 167
		f 4 -232 -235 -237 235
		mu 0 4 165 151 166 168
		f 4 -240 -236 -239 -238
		mu 0 4 169 165 168 170
		f 4 -243 -242 72 240
		mu 0 4 171 174 173 172
		f 4 -246 -245 242 243
		mu 0 4 175 176 174 171
		f 4 249 248 -248 -247
		mu 0 4 177 180 179 178
		f 4 253 252 -252 -251
		mu 0 4 181 184 183 182
		f 4 -258 256 255 -255
		mu 0 4 185 188 187 186
		f 4 259 250 -259 -1
		mu 0 4 189 181 182 190
		f 4 -5 261 257 -261
		mu 0 4 191 192 188 185
		f 4 262 -262 -16 258
		mu 0 4 182 188 192 190
		f 4 263 -257 -263 251
		mu 0 4 183 187 188 182
		f 4 266 -266 -265 247
		mu 0 4 179 194 193 178
		f 4 269 246 -269 -268
		mu 0 4 195 177 178 196
		f 4 -272 -271 268 264
		mu 0 4 193 197 196 178
		f 4 271 274 -274 -273
		mu 0 4 197 193 199 198
		f 4 -278 -29 276 275
		mu 0 4 200 203 202 201
		f 4 -281 -276 279 278
		mu 0 4 204 200 201 205
		f 4 -249 282 212 -282
		mu 0 4 206 207 149 150
		f 4 265 284 -231 -284
		mu 0 4 208 209 164 165
		f 4 -267 281 229 -285
		mu 0 4 209 206 150 164
		f 4 288 287 -287 -286
		mu 0 4 210 213 212 211
		f 4 -275 283 239 -290
		mu 0 4 214 208 165 169
		f 4 277 290 -241 -72
		mu 0 4 215 216 171 172
		f 4 -279 292 245 -292
		mu 0 4 217 218 176 175
		f 4 280 291 -244 -291
		mu 0 4 216 217 175 171
		f 4 -78 260 293 -277
		mu 0 4 202 191 185 201
		f 4 -294 254 294 -280
		mu 0 4 201 185 186 205
		f 4 273 296 -289 -296
		mu 0 4 198 199 220 219
		f 4 -297 289 297 -288
		mu 0 4 213 214 169 212
		f 4 286 -298 237 -299
		mu 0 4 211 212 169 170
		f 4 244 -301 219 299
		mu 0 4 174 298 156 157
		f 4 241 -300 225 84
		mu 0 4 173 174 157 162
		f 4 302 267 -302 -253
		mu 0 4 184 195 196 183
		f 4 -304 -264 301 270
		mu 0 4 197 187 183 196
		f 4 -256 303 272 -305
		mu 0 4 186 187 197 198
		f 4 -295 304 295 -306
		mu 0 4 205 186 198 219
		f 4 305 285 -307 -293
		mu 0 4 218 210 211 176
		f 4 306 298 307 300
		mu 0 4 176 211 170 221
		f 4 -309 222 -308 238
		mu 0 4 168 159 221 170
		f 4 -310 228 308 236
		mu 0 4 166 154 159 168
		f 4 216 309 232 -311
		mu 0 4 153 154 166 167
		f 4 211 313 -313 -312
		mu 0 4 222 225 224 223
		f 4 215 316 -316 -315
		mu 0 4 226 229 228 227
		f 4 320 -320 318 317
		mu 0 4 230 233 232 231
		f 4 223 314 -322 -123
		mu 0 4 234 226 227 235
		f 4 323 -125 322 319
		mu 0 4 233 237 236 232
		f 4 324 -323 -128 321
		mu 0 4 227 232 236 235
		f 4 325 -319 -325 315
		mu 0 4 228 231 232 227
		f 4 328 -328 -327 312
		mu 0 4 224 239 238 223
		f 4 330 233 311 -330
		mu 0 4 240 241 222 223
		f 4 -333 331 329 326
		mu 0 4 238 242 240 223
		f 4 335 334 332 333
		mu 0 4 243 244 242 238
		f 4 -339 -141 337 336
		mu 0 4 245 248 247 246
		f 4 -342 -337 340 339
		mu 0 4 249 245 246 250
		f 4 344 343 -343 -250
		mu 0 4 251 254 253 252
		f 4 347 346 -346 -254
		mu 0 4 255 258 257 256
		f 4 351 -351 -350 348
		mu 0 4 259 262 261 260
		f 4 156 352 -348 -260
		mu 0 4 263 264 258 255
		f 4 354 -349 -354 160
		mu 0 4 265 259 260 266
		f 4 -353 161 353 -356
		mu 0 4 258 264 266 260
		f 4 -347 355 349 -357
		mu 0 4 257 258 260 261
		f 4 -344 359 358 -358
		mu 0 4 253 254 268 267
		f 4 361 360 -345 -270
		mu 0 4 269 270 254 251
		f 4 -360 -361 363 362
		mu 0 4 268 254 270 271
		f 4 366 365 -365 -363
		mu 0 4 271 273 272 268
		f 4 -370 -369 176 367
		mu 0 4 274 277 276 275
		f 4 -373 -372 369 370
		mu 0 4 278 279 277 274
		f 4 373 -314 -283 342
		mu 0 4 280 224 225 281
		f 4 375 327 -375 -359
		mu 0 4 282 238 239 283
		f 4 374 -329 -374 357
		mu 0 4 283 239 224 280
		f 4 379 378 -378 -377
		mu 0 4 284 287 286 285
		f 4 380 -334 -376 364
		mu 0 4 288 243 238 282
		f 4 189 338 -382 -368
		mu 0 4 289 248 245 290
		f 4 383 -340 -383 372
		mu 0 4 291 249 250 292
		f 4 381 341 -384 -371
		mu 0 4 290 245 249 291
		f 4 368 -385 -355 194
		mu 0 4 276 277 259 265
		f 4 371 -386 -352 384
		mu 0 4 277 279 262 259
		f 4 387 376 -387 -366
		mu 0 4 273 294 293 272
		f 4 377 -389 -381 386
		mu 0 4 285 286 243 288
		f 4 389 -336 388 -379
		mu 0 4 287 244 243 286
		f 4 -392 -321 390 -341
		mu 0 4 246 233 230 299
		f 4 -203 -324 391 -338
		mu 0 4 247 237 233 246
		f 4 345 392 -362 -303
		mu 0 4 256 257 270 269
		f 4 -364 -393 356 393
		mu 0 4 271 270 257 261
		f 4 394 -367 -394 350
		mu 0 4 262 273 271 261
		f 4 395 -388 -395 385
		mu 0 4 279 294 273 262
		f 4 382 396 -380 -396
		mu 0 4 292 250 287 284
		f 4 -391 -398 -390 -397
		mu 0 4 250 295 244 287
		f 4 -335 397 -318 398
		mu 0 4 242 244 295 231
		f 4 -332 -399 -326 399
		mu 0 4 240 242 231 228
		f 4 310 -331 -400 -317
		mu 0 4 229 241 240 228;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 176 
		1 0 
		3 0 
		5 0 
		7 0 
		8 0 
		9 0 
		11 0 
		12 0 
		17 0 
		18 0 
		24 0 
		27 0 
		30 0 
		33 0 
		34 0 
		38 0 
		42 0 
		44 0 
		45 0 
		46 0 
		47 0 
		52 0 
		53 0 
		57 0 
		58 0 
		59 0 
		60 0 
		61 0 
		62 0 
		65 0 
		67 0 
		68 0 
		73 0 
		74 0 
		75 0 
		76 0 
		79 0 
		80 0 
		81 0 
		82 0 
		94 0 
		96 0 
		97 0 
		99 0 
		111 0 
		113 0 
		114 0 
		119 0 
		120 0 
		125 0 
		128 0 
		129 0 
		130 0 
		131 0 
		134 0 
		138 0 
		139 0 
		141 0 
		143 0 
		144 0 
		145 0 
		148 0 
		151 0 
		153 0 
		154 0 
		159 0 
		163 0 
		165 0 
		166 0 
		170 0 
		173 0 
		177 0 
		178 0 
		180 0 
		182 0 
		183 0 
		185 0 
		187 0 
		191 0 
		192 0 
		195 0 
		197 0 
		198 0 
		201 0 
		204 0 
		206 0 
		207 0 
		209 0 
		212 0 
		214 0 
		216 0 
		221 0 
		222 0 
		226 0 
		228 0 
		229 0 
		230 0 
		231 0 
		235 0 
		239 0 
		240 0 
		241 0 
		243 0 
		244 0 
		245 0 
		248 0 
		252 0 
		255 0 
		256 0 
		261 0 
		263 0 
		265 0 
		266 0 
		271 0 
		274 0 
		277 0 
		278 0 
		280 0 
		283 0 
		284 0 
		286 0 
		288 0 
		291 0 
		292 0 
		294 0 
		296 0 
		297 0 
		300 0 
		303 0 
		305 0 
		306 0 
		308 0 
		311 0 
		313 0 
		316 0 
		317 0 
		318 0 
		322 0 
		325 0 
		326 0 
		327 0 
		328 0 
		332 0 
		333 0 
		337 0 
		338 0 
		339 0 
		340 0 
		341 0 
		342 0 
		345 0 
		349 0 
		350 0 
		353 0 
		356 0 
		357 0 
		358 0 
		362 0 
		364 0 
		367 0 
		370 0 
		372 0 
		374 0 
		375 0 
		376 0 
		378 0 
		382 0 
		383 0 
		385 0 
		386 0 
		388 0 
		390 0 
		393 0 
		395 0 
		396 0 
		398 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".db" yes;
	setAttr ".bw" 3;
	setAttr ".ndt" 0;
	setAttr ".dr" 3;
	setAttr ".mgi" -type "string" "ID_36e3ad18-fe77-11e4-ba31-1cc1de328f39";
	setAttr ".RMNCS__gprimHandleid" -type "string" "plate";
	setAttr ".RMNCS__handleid" -type "string" "plate";
createNode mentalrayItemsList -s -n "mentalrayItemsList";
	rename -uid "0D9708C0-0000-66E1-59DF-C01C00000282";
createNode mentalrayGlobals -s -n "mentalrayGlobals";
	rename -uid "0D9708C0-0000-66E1-59DF-C01C00000283";
createNode mentalrayOptions -s -n "miDefaultOptions";
	rename -uid "0D9708C0-0000-66E1-59DF-C01C00000284";
	addAttr -ci true -m -sn "stringOptions" -ln "stringOptions" -at "compound" -nc 
		3;
	addAttr -ci true -sn "name" -ln "name" -dt "string" -p "stringOptions";
	addAttr -ci true -sn "value" -ln "value" -dt "string" -p "stringOptions";
	addAttr -ci true -sn "type" -ln "type" -dt "string" -p "stringOptions";
	setAttr -s 81 ".stringOptions";
	setAttr ".stringOptions[0].name" -type "string" "rast motion factor";
	setAttr ".stringOptions[0].value" -type "string" "1.0";
	setAttr ".stringOptions[0].type" -type "string" "scalar";
	setAttr ".stringOptions[1].name" -type "string" "rast transparency depth";
	setAttr ".stringOptions[1].value" -type "string" "8";
	setAttr ".stringOptions[1].type" -type "string" "integer";
	setAttr ".stringOptions[2].name" -type "string" "rast useopacity";
	setAttr ".stringOptions[2].value" -type "string" "true";
	setAttr ".stringOptions[2].type" -type "string" "boolean";
	setAttr ".stringOptions[3].name" -type "string" "importon";
	setAttr ".stringOptions[3].value" -type "string" "false";
	setAttr ".stringOptions[3].type" -type "string" "boolean";
	setAttr ".stringOptions[4].name" -type "string" "importon density";
	setAttr ".stringOptions[4].value" -type "string" "1.0";
	setAttr ".stringOptions[4].type" -type "string" "scalar";
	setAttr ".stringOptions[5].name" -type "string" "importon merge";
	setAttr ".stringOptions[5].value" -type "string" "0.0";
	setAttr ".stringOptions[5].type" -type "string" "scalar";
	setAttr ".stringOptions[6].name" -type "string" "importon trace depth";
	setAttr ".stringOptions[6].value" -type "string" "0";
	setAttr ".stringOptions[6].type" -type "string" "integer";
	setAttr ".stringOptions[7].name" -type "string" "importon traverse";
	setAttr ".stringOptions[7].value" -type "string" "true";
	setAttr ".stringOptions[7].type" -type "string" "boolean";
	setAttr ".stringOptions[8].name" -type "string" "shadowmap pixel samples";
	setAttr ".stringOptions[8].value" -type "string" "3";
	setAttr ".stringOptions[8].type" -type "string" "integer";
	setAttr ".stringOptions[9].name" -type "string" "ambient occlusion";
	setAttr ".stringOptions[9].value" -type "string" "false";
	setAttr ".stringOptions[9].type" -type "string" "boolean";
	setAttr ".stringOptions[10].name" -type "string" "ambient occlusion rays";
	setAttr ".stringOptions[10].value" -type "string" "64";
	setAttr ".stringOptions[10].type" -type "string" "integer";
	setAttr ".stringOptions[11].name" -type "string" "ambient occlusion cache";
	setAttr ".stringOptions[11].value" -type "string" "false";
	setAttr ".stringOptions[11].type" -type "string" "boolean";
	setAttr ".stringOptions[12].name" -type "string" "ambient occlusion cache density";
	setAttr ".stringOptions[12].value" -type "string" "1.0";
	setAttr ".stringOptions[12].type" -type "string" "scalar";
	setAttr ".stringOptions[13].name" -type "string" "ambient occlusion cache points";
	setAttr ".stringOptions[13].value" -type "string" "64";
	setAttr ".stringOptions[13].type" -type "string" "integer";
	setAttr ".stringOptions[14].name" -type "string" "irradiance particles";
	setAttr ".stringOptions[14].value" -type "string" "false";
	setAttr ".stringOptions[14].type" -type "string" "boolean";
	setAttr ".stringOptions[15].name" -type "string" "irradiance particles rays";
	setAttr ".stringOptions[15].value" -type "string" "256";
	setAttr ".stringOptions[15].type" -type "string" "integer";
	setAttr ".stringOptions[16].name" -type "string" "irradiance particles interpolate";
	setAttr ".stringOptions[16].value" -type "string" "1";
	setAttr ".stringOptions[16].type" -type "string" "integer";
	setAttr ".stringOptions[17].name" -type "string" "irradiance particles interppoints";
	setAttr ".stringOptions[17].value" -type "string" "64";
	setAttr ".stringOptions[17].type" -type "string" "integer";
	setAttr ".stringOptions[18].name" -type "string" "irradiance particles indirect passes";
	setAttr ".stringOptions[18].value" -type "string" "0";
	setAttr ".stringOptions[18].type" -type "string" "integer";
	setAttr ".stringOptions[19].name" -type "string" "irradiance particles scale";
	setAttr ".stringOptions[19].value" -type "string" "1.0";
	setAttr ".stringOptions[19].type" -type "string" "scalar";
	setAttr ".stringOptions[20].name" -type "string" "irradiance particles env";
	setAttr ".stringOptions[20].value" -type "string" "true";
	setAttr ".stringOptions[20].type" -type "string" "boolean";
	setAttr ".stringOptions[21].name" -type "string" "irradiance particles env rays";
	setAttr ".stringOptions[21].value" -type "string" "256";
	setAttr ".stringOptions[21].type" -type "string" "integer";
	setAttr ".stringOptions[22].name" -type "string" "irradiance particles env scale";
	setAttr ".stringOptions[22].value" -type "string" "1";
	setAttr ".stringOptions[22].type" -type "string" "integer";
	setAttr ".stringOptions[23].name" -type "string" "irradiance particles rebuild";
	setAttr ".stringOptions[23].value" -type "string" "true";
	setAttr ".stringOptions[23].type" -type "string" "boolean";
	setAttr ".stringOptions[24].name" -type "string" "irradiance particles file";
	setAttr ".stringOptions[24].value" -type "string" "";
	setAttr ".stringOptions[24].type" -type "string" "string";
	setAttr ".stringOptions[25].name" -type "string" "geom displace motion factor";
	setAttr ".stringOptions[25].value" -type "string" "1.0";
	setAttr ".stringOptions[25].type" -type "string" "scalar";
	setAttr ".stringOptions[26].name" -type "string" "contrast all buffers";
	setAttr ".stringOptions[26].value" -type "string" "false";
	setAttr ".stringOptions[26].type" -type "string" "boolean";
	setAttr ".stringOptions[27].name" -type "string" "finalgather normal tolerance";
	setAttr ".stringOptions[27].value" -type "string" "25.842";
	setAttr ".stringOptions[27].type" -type "string" "scalar";
	setAttr ".stringOptions[28].name" -type "string" "trace camera clip";
	setAttr ".stringOptions[28].value" -type "string" "false";
	setAttr ".stringOptions[28].type" -type "string" "boolean";
	setAttr ".stringOptions[29].name" -type "string" "unified sampling";
	setAttr ".stringOptions[29].value" -type "string" "true";
	setAttr ".stringOptions[29].type" -type "string" "boolean";
	setAttr ".stringOptions[30].name" -type "string" "samples quality";
	setAttr ".stringOptions[30].value" -type "string" "0.25 0.25 0.25 0.25";
	setAttr ".stringOptions[30].type" -type "string" "color";
	setAttr ".stringOptions[31].name" -type "string" "samples min";
	setAttr ".stringOptions[31].value" -type "string" "1.0";
	setAttr ".stringOptions[31].type" -type "string" "scalar";
	setAttr ".stringOptions[32].name" -type "string" "samples max";
	setAttr ".stringOptions[32].value" -type "string" "100.0";
	setAttr ".stringOptions[32].type" -type "string" "scalar";
	setAttr ".stringOptions[33].name" -type "string" "samples error cutoff";
	setAttr ".stringOptions[33].value" -type "string" "0.0 0.0 0.0 0.0";
	setAttr ".stringOptions[33].type" -type "string" "color";
	setAttr ".stringOptions[34].name" -type "string" "samples per object";
	setAttr ".stringOptions[34].value" -type "string" "false";
	setAttr ".stringOptions[34].type" -type "string" "boolean";
	setAttr ".stringOptions[35].name" -type "string" "progressive";
	setAttr ".stringOptions[35].value" -type "string" "false";
	setAttr ".stringOptions[35].type" -type "string" "boolean";
	setAttr ".stringOptions[36].name" -type "string" "progressive max time";
	setAttr ".stringOptions[36].value" -type "string" "0";
	setAttr ".stringOptions[36].type" -type "string" "integer";
	setAttr ".stringOptions[37].name" -type "string" "progressive subsampling size";
	setAttr ".stringOptions[37].value" -type "string" "4";
	setAttr ".stringOptions[37].type" -type "string" "integer";
	setAttr ".stringOptions[38].name" -type "string" "iray";
	setAttr ".stringOptions[38].value" -type "string" "false";
	setAttr ".stringOptions[38].type" -type "string" "boolean";
	setAttr ".stringOptions[39].name" -type "string" "light relative scale";
	setAttr ".stringOptions[39].value" -type "string" "0.31831";
	setAttr ".stringOptions[39].type" -type "string" "scalar";
	setAttr ".stringOptions[40].name" -type "string" "trace camera motion vectors";
	setAttr ".stringOptions[40].value" -type "string" "false";
	setAttr ".stringOptions[40].type" -type "string" "boolean";
	setAttr ".stringOptions[41].name" -type "string" "ray differentials";
	setAttr ".stringOptions[41].value" -type "string" "true";
	setAttr ".stringOptions[41].type" -type "string" "boolean";
	setAttr ".stringOptions[42].name" -type "string" "environment lighting mode";
	setAttr ".stringOptions[42].value" -type "string" "off";
	setAttr ".stringOptions[42].type" -type "string" "string";
	setAttr ".stringOptions[43].name" -type "string" "environment lighting quality";
	setAttr ".stringOptions[43].value" -type "string" "0.2";
	setAttr ".stringOptions[43].type" -type "string" "scalar";
	setAttr ".stringOptions[44].name" -type "string" "environment lighting shadow";
	setAttr ".stringOptions[44].value" -type "string" "transparent";
	setAttr ".stringOptions[44].type" -type "string" "string";
	setAttr ".stringOptions[45].name" -type "string" "environment lighting resolution";
	setAttr ".stringOptions[45].value" -type "string" "512";
	setAttr ".stringOptions[45].type" -type "string" "integer";
	setAttr ".stringOptions[46].name" -type "string" "environment lighting shader samples";
	setAttr ".stringOptions[46].value" -type "string" "2";
	setAttr ".stringOptions[46].type" -type "string" "integer";
	setAttr ".stringOptions[47].name" -type "string" "environment lighting scale";
	setAttr ".stringOptions[47].value" -type "string" "1 1 1";
	setAttr ".stringOptions[47].type" -type "string" "color";
	setAttr ".stringOptions[48].name" -type "string" "environment lighting caustic photons";
	setAttr ".stringOptions[48].value" -type "string" "0";
	setAttr ".stringOptions[48].type" -type "string" "integer";
	setAttr ".stringOptions[49].name" -type "string" "environment lighting globillum photons";
	setAttr ".stringOptions[49].value" -type "string" "0";
	setAttr ".stringOptions[49].type" -type "string" "integer";
	setAttr ".stringOptions[50].name" -type "string" "light importance sampling";
	setAttr ".stringOptions[50].value" -type "string" "all";
	setAttr ".stringOptions[50].type" -type "string" "string";
	setAttr ".stringOptions[51].name" -type "string" "light importance sampling quality";
	setAttr ".stringOptions[51].value" -type "string" "1.0";
	setAttr ".stringOptions[51].type" -type "string" "scalar";
	setAttr ".stringOptions[52].name" -type "string" "light importance sampling samples";
	setAttr ".stringOptions[52].value" -type "string" "4";
	setAttr ".stringOptions[52].type" -type "string" "integer";
	setAttr ".stringOptions[53].name" -type "string" "light importance sampling resolution";
	setAttr ".stringOptions[53].value" -type "string" "1.0";
	setAttr ".stringOptions[53].type" -type "string" "scalar";
	setAttr ".stringOptions[54].name" -type "string" "light importance sampling precomputed";
	setAttr ".stringOptions[54].value" -type "string" "false";
	setAttr ".stringOptions[54].type" -type "string" "boolean";
	setAttr ".stringOptions[55].name" -type "string" "mila quality";
	setAttr ".stringOptions[55].value" -type "string" "1.0";
	setAttr ".stringOptions[55].type" -type "string" "scalar";
	setAttr ".stringOptions[56].name" -type "string" "mila glossy quality";
	setAttr ".stringOptions[56].value" -type "string" "1.0";
	setAttr ".stringOptions[56].type" -type "string" "scalar";
	setAttr ".stringOptions[57].name" -type "string" "mila scatter quality";
	setAttr ".stringOptions[57].value" -type "string" "1.0";
	setAttr ".stringOptions[57].type" -type "string" "scalar";
	setAttr ".stringOptions[58].name" -type "string" "mila scatter scale";
	setAttr ".stringOptions[58].value" -type "string" "1.0";
	setAttr ".stringOptions[58].type" -type "string" "scalar";
	setAttr ".stringOptions[59].name" -type "string" "mila diffuse quality";
	setAttr ".stringOptions[59].value" -type "string" "1.0";
	setAttr ".stringOptions[59].type" -type "string" "scalar";
	setAttr ".stringOptions[60].name" -type "string" "mila diffuse detail";
	setAttr ".stringOptions[60].value" -type "string" "false";
	setAttr ".stringOptions[60].type" -type "string" "boolean";
	setAttr ".stringOptions[61].name" -type "string" "mila diffuse detail distance";
	setAttr ".stringOptions[61].value" -type "string" "10.0";
	setAttr ".stringOptions[61].type" -type "string" "scalar";
	setAttr ".stringOptions[62].name" -type "string" "mila use max distance inside";
	setAttr ".stringOptions[62].value" -type "string" "true";
	setAttr ".stringOptions[62].type" -type "string" "boolean";
	setAttr ".stringOptions[63].name" -type "string" "mila clamp output";
	setAttr ".stringOptions[63].value" -type "string" "false";
	setAttr ".stringOptions[63].type" -type "string" "boolean";
	setAttr ".stringOptions[64].name" -type "string" "mila clamp level";
	setAttr ".stringOptions[64].value" -type "string" "1.0";
	setAttr ".stringOptions[64].type" -type "string" "scalar";
	setAttr ".stringOptions[65].name" -type "string" "gi gpu";
	setAttr ".stringOptions[65].value" -type "string" "off";
	setAttr ".stringOptions[65].type" -type "string" "string";
	setAttr ".stringOptions[66].name" -type "string" "gi gpu rays";
	setAttr ".stringOptions[66].value" -type "string" "34";
	setAttr ".stringOptions[66].type" -type "string" "integer";
	setAttr ".stringOptions[67].name" -type "string" "gi gpu passes";
	setAttr ".stringOptions[67].value" -type "string" "4";
	setAttr ".stringOptions[67].type" -type "string" "integer";
	setAttr ".stringOptions[68].name" -type "string" "gi gpu presample density";
	setAttr ".stringOptions[68].value" -type "string" "1.0";
	setAttr ".stringOptions[68].type" -type "string" "scalar";
	setAttr ".stringOptions[69].name" -type "string" "gi gpu presample depth";
	setAttr ".stringOptions[69].value" -type "string" "2";
	setAttr ".stringOptions[69].type" -type "string" "integer";
	setAttr ".stringOptions[70].name" -type "string" "gi gpu filter";
	setAttr ".stringOptions[70].value" -type "string" "1.0";
	setAttr ".stringOptions[70].type" -type "string" "integer";
	setAttr ".stringOptions[71].name" -type "string" "gi gpu depth";
	setAttr ".stringOptions[71].value" -type "string" "3";
	setAttr ".stringOptions[71].type" -type "string" "integer";
	setAttr ".stringOptions[72].name" -type "string" "gi gpu devices";
	setAttr ".stringOptions[72].value" -type "string" "0";
	setAttr ".stringOptions[72].type" -type "string" "integer";
	setAttr ".stringOptions[73].name" -type "string" "shutter shape function";
	setAttr ".stringOptions[73].value" -type "string" "none";
	setAttr ".stringOptions[73].type" -type "string" "string";
	setAttr ".stringOptions[74].name" -type "string" "shutter full open";
	setAttr ".stringOptions[74].value" -type "string" "0.2";
	setAttr ".stringOptions[74].type" -type "string" "scalar";
	setAttr ".stringOptions[75].name" -type "string" "shutter full close";
	setAttr ".stringOptions[75].value" -type "string" "0.8";
	setAttr ".stringOptions[75].type" -type "string" "scalar";
	setAttr ".stringOptions[76].name" -type "string" "gi";
	setAttr ".stringOptions[76].value" -type "string" "off";
	setAttr ".stringOptions[76].type" -type "string" "boolean";
	setAttr ".stringOptions[77].name" -type "string" "gi rays";
	setAttr ".stringOptions[77].value" -type "string" "100";
	setAttr ".stringOptions[77].type" -type "string" "integer";
	setAttr ".stringOptions[78].name" -type "string" "gi depth";
	setAttr ".stringOptions[78].value" -type "string" "0";
	setAttr ".stringOptions[78].type" -type "string" "integer";
	setAttr ".stringOptions[79].name" -type "string" "gi freeze";
	setAttr ".stringOptions[79].value" -type "string" "off";
	setAttr ".stringOptions[79].type" -type "string" "boolean";
	setAttr ".stringOptions[80].name" -type "string" "gi filter";
	setAttr ".stringOptions[80].value" -type "string" "1.0";
	setAttr ".stringOptions[80].type" -type "string" "scalar";
createNode mentalrayFramebuffer -s -n "miDefaultFramebuffer";
	rename -uid "0D9708C0-0000-66E1-59DF-C01C00000285";
createNode lightLinker -s -n "lightLinker1";
	rename -uid "555D58C0-0000-4214-59DF-C29900000333";
	setAttr -s 8 ".lnk";
	setAttr -s 8 ".slnk";
createNode displayLayerManager -n "layerManager";
	rename -uid "555D58C0-0000-4214-59DF-C29900000334";
	setAttr -s 7 ".dli[1:6]"  1 2 3 4 5 6;
	setAttr -s 7 ".dli";
createNode displayLayer -n "defaultLayer";
	rename -uid "109AE860-0000-148A-5708-3ED200000251";
createNode renderLayerManager -n "renderLayerManager";
	rename -uid "555D58C0-0000-4214-59DF-C29900000336";
createNode renderLayer -n "defaultRenderLayer";
	rename -uid "109AE860-0000-148A-5708-3ED200000253";
	setAttr ".g" yes;
createNode expression -n "C3d__HiddenInfoNode";
	rename -uid "109AE860-0000-148A-5708-3ED200000255";
	addAttr -ci true -h true -sn "__pxSimNodeEmulate" -ln "__pxSimNodeEmulate" -min 
		0 -max 1 -at "bool";
	addAttr -s false -ci true -h true -sn "_c3dgui_posingMesh" -ln "_c3dgui_posingMesh" 
		-at "message";
	addAttr -s false -ci true -sn "_c3dgui_inputMesh" -ln "_c3dgui_inputMesh" -at "message";
	addAttr -ci true -sn "_c3dgui_inputMesh__broken" -ln "_c3dgui_inputMesh__broken" 
		-dt "string";
	setAttr -k on ".nds";
	setAttr ".__pxSimNodeEmulate" yes;
	setAttr -l on "._c3dgui_inputMesh__broken" -type "string" "qGarmentMeshShape";
createNode displayLayer -n "Decimated";
	rename -uid "109AE860-0000-148A-5708-3ED200000256";
	setAttr ".do" 1;
createNode displayLayer -n "Simulation";
	rename -uid "109AE860-0000-148A-5708-3ED200000257";
	setAttr ".do" 2;
createNode displayLayer -n "Render";
	rename -uid "109AE860-0000-148A-5708-3ED200000258";
	setAttr ".do" 3;
createNode script -n "uiConfigurationScriptNode";
	rename -uid "109AE860-0000-148A-5708-3ED200000259";
	setAttr ".b" -type "string" (
		"// Maya Mel UI Configuration File.\n//\n//  This script is machine generated.  Edit at your own risk.\n//\n//\n\nglobal string $gMainPane;\nif (`paneLayout -exists $gMainPane`) {\n\n\tglobal int $gUseScenePanelConfig;\n\tint    $useSceneConfig = $gUseScenePanelConfig;\n\tint    $menusOkayInPanels = `optionVar -q allowMenusInPanels`;\tint    $nVisPanes = `paneLayout -q -nvp $gMainPane`;\n\tint    $nPanes = 0;\n\tstring $editorName;\n\tstring $panelName;\n\tstring $itemFilterName;\n\tstring $panelConfig;\n\n\t//\n\t//  get current state of the UI\n\t//\n\tsceneUIReplacement -update $gMainPane;\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Top View\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `modelPanel -unParent -l (localizedPanelLabel(\"Top View\")) -mbv $menusOkayInPanels `;\n\t\t\t$editorName = $panelName;\n            modelEditor -e \n                -camera \"top\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"wireframe\" \n"
		+ "                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 1\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n"
		+ "                -rendererName \"base_OpenGL_Renderer\" \n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 256 256 \n                -bumpResolution 512 512 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 1\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n"
		+ "                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 1\n                -height 1\n                -sceneRenderFilter 0\n                $editorName;\n            modelEditor -e -viewSelected 0 $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n"
		+ "\t\tmodelPanel -edit -l (localizedPanelLabel(\"Top View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"top\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"wireframe\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 1\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n"
		+ "            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n"
		+ "            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n"
		+ "\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Side View\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `modelPanel -unParent -l (localizedPanelLabel(\"Side View\")) -mbv $menusOkayInPanels `;\n\t\t\t$editorName = $panelName;\n            modelEditor -e \n                -camera \"side\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"wireframe\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 1\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n"
		+ "                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n                -rendererName \"base_OpenGL_Renderer\" \n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 256 256 \n                -bumpResolution 512 512 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 1\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n"
		+ "                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n"
		+ "                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 1\n                -height 1\n                -sceneRenderFilter 0\n                $editorName;\n            modelEditor -e -viewSelected 0 $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Side View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"side\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"wireframe\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n"
		+ "            -twoSidedLighting 1\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n"
		+ "            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n"
		+ "            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Front View\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `modelPanel -unParent -l (localizedPanelLabel(\"Front View\")) -mbv $menusOkayInPanels `;\n\t\t\t$editorName = $panelName;\n            modelEditor -e \n                -camera \"front\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"wireframe\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n"
		+ "                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 1\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n                -rendererName \"base_OpenGL_Renderer\" \n                -objectFilterShowInHUD 1\n"
		+ "                -isFiltered 0\n                -colorResolution 256 256 \n                -bumpResolution 512 512 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 1\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n"
		+ "                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 1\n                -height 1\n                -sceneRenderFilter 0\n                $editorName;\n            modelEditor -e -viewSelected 0 $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Front View\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"front\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"wireframe\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 1\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n"
		+ "            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n"
		+ "            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Persp View\")) `;\n\tif (\"\" == $panelName) {\n"
		+ "\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `modelPanel -unParent -l (localizedPanelLabel(\"Persp View\")) -mbv $menusOkayInPanels `;\n\t\t\t$editorName = $panelName;\n            modelEditor -e \n                -camera \"persp\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"smoothShaded\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 1\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n"
		+ "                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n                -rendererName \"base_OpenGL_Renderer\" \n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 256 256 \n                -bumpResolution 512 512 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 1\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n"
		+ "                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n"
		+ "                -motionTrails 1\n                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 1008\n                -height 750\n                -sceneRenderFilter 0\n                $editorName;\n            modelEditor -e -viewSelected 0 $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Persp View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"persp\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 1\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n"
		+ "            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n"
		+ "            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n"
		+ "            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1008\n            -height 750\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"Outliner\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `outlinerPanel -unParent -l (localizedPanelLabel(\"Outliner\")) -mbv $menusOkayInPanels `;\n\t\t\t$editorName = $panelName;\n            outlinerEditor -e \n                -docTag \"isolOutln_fromSeln\" \n                -showShapes 1\n                -showReferenceNodes 1\n                -showReferenceMembers 1\n                -showAttributes 0\n                -showConnected 0\n                -showAnimCurvesOnly 0\n                -showMuteInfo 0\n                -organizeByLayer 1\n                -showAnimLayerWeight 1\n"
		+ "                -autoExpandLayers 1\n                -autoExpand 0\n                -showDagOnly 1\n                -showAssets 1\n                -showContainedOnly 1\n                -showPublishedAsConnected 0\n                -showContainerContents 1\n                -ignoreDagHierarchy 0\n                -expandConnections 0\n                -showUpstreamCurves 1\n                -showUnitlessCurves 1\n                -showCompounds 1\n                -showLeafs 1\n                -showNumericAttrsOnly 0\n                -highlightActive 1\n                -autoSelectNewObjects 0\n                -doNotSelectNewObjects 0\n                -dropIsParent 1\n                -transmitFilters 0\n                -setFilter \"defaultSetFilter\" \n                -showSetMembers 1\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n"
		+ "                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 0\n                -mapMotionTrails 0\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\toutlinerPanel -edit -l (localizedPanelLabel(\"Outliner\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        outlinerEditor -e \n            -docTag \"isolOutln_fromSeln\" \n            -showShapes 1\n            -showReferenceNodes 1\n            -showReferenceMembers 1\n            -showAttributes 0\n            -showConnected 0\n            -showAnimCurvesOnly 0\n            -showMuteInfo 0\n            -organizeByLayer 1\n"
		+ "            -showAnimLayerWeight 1\n            -autoExpandLayers 1\n            -autoExpand 0\n            -showDagOnly 1\n            -showAssets 1\n            -showContainedOnly 1\n            -showPublishedAsConnected 0\n            -showContainerContents 1\n            -ignoreDagHierarchy 0\n            -expandConnections 0\n            -showUpstreamCurves 1\n            -showUnitlessCurves 1\n            -showCompounds 1\n            -showLeafs 1\n            -showNumericAttrsOnly 0\n            -highlightActive 1\n            -autoSelectNewObjects 0\n            -doNotSelectNewObjects 0\n            -dropIsParent 1\n            -transmitFilters 0\n            -setFilter \"defaultSetFilter\" \n            -showSetMembers 1\n            -allowMultiSelection 1\n            -alwaysToggleSelect 0\n            -directSelect 0\n            -displayMode \"DAG\" \n            -expandObjects 0\n            -setsIgnoreFilters 1\n            -containersIgnoreFilters 0\n            -editAttrName 0\n            -showAttrValues 0\n            -highlightSecondary 0\n"
		+ "            -showUVAttrsOnly 0\n            -showTextureNodesOnly 0\n            -attrAlphaOrder \"default\" \n            -animLayerFilterOptions \"allAffecting\" \n            -sortOrder \"none\" \n            -longNames 0\n            -niceNames 1\n            -showNamespace 1\n            -showPinIcons 0\n            -mapMotionTrails 0\n            -ignoreHiddenAttribute 0\n            -ignoreOutlinerColor 0\n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"graphEditor\" (localizedPanelLabel(\"Graph Editor\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"graphEditor\" -l (localizedPanelLabel(\"Graph Editor\")) -mbv $menusOkayInPanels `;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n            outlinerEditor -e \n                -showShapes 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n                -showAttributes 1\n                -showConnected 1\n"
		+ "                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n                -organizeByLayer 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 1\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n                -showUpstreamCurves 1\n                -showUnitlessCurves 1\n                -showCompounds 0\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n                -autoSelectNewObjects 1\n                -doNotSelectNewObjects 0\n                -dropIsParent 1\n                -transmitFilters 1\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -displayMode \"DAG\" \n"
		+ "                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 1\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                $editorName;\n\n\t\t\t$editorName = ($panelName+\"GraphEd\");\n            animCurveEditor -e \n                -displayKeys 1\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 1\n                -displayInfinities 0\n                -autoFit 0\n                -snapTime \"integer\" \n                -snapValue \"none\" \n"
		+ "                -showResults \"off\" \n                -showBufferCurves \"off\" \n                -smoothness \"fine\" \n                -resultSamples 1\n                -resultScreenSamples 0\n                -resultUpdate \"delayed\" \n                -showUpstreamCurves 1\n                -stackedCurves 0\n                -stackedCurvesMin -1\n                -stackedCurvesMax 1\n                -stackedCurvesSpace 0.2\n                -displayNormalized 0\n                -preSelectionHighlight 0\n                -constrainDrag 0\n                -classicMode 1\n                $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Graph Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n            outlinerEditor -e \n                -showShapes 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n"
		+ "                -organizeByLayer 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 1\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n                -showUpstreamCurves 1\n                -showUnitlessCurves 1\n                -showCompounds 0\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n                -autoSelectNewObjects 1\n                -doNotSelectNewObjects 0\n                -dropIsParent 1\n                -transmitFilters 1\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n"
		+ "                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 1\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                $editorName;\n\n\t\t\t$editorName = ($panelName+\"GraphEd\");\n            animCurveEditor -e \n                -displayKeys 1\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 1\n                -displayInfinities 0\n                -autoFit 0\n                -snapTime \"integer\" \n                -snapValue \"none\" \n                -showResults \"off\" \n                -showBufferCurves \"off\" \n"
		+ "                -smoothness \"fine\" \n                -resultSamples 1\n                -resultScreenSamples 0\n                -resultUpdate \"delayed\" \n                -showUpstreamCurves 1\n                -stackedCurves 0\n                -stackedCurvesMin -1\n                -stackedCurvesMax 1\n                -stackedCurvesSpace 0.2\n                -displayNormalized 0\n                -preSelectionHighlight 0\n                -constrainDrag 0\n                -classicMode 1\n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dopeSheetPanel\" (localizedPanelLabel(\"Dope Sheet\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"dopeSheetPanel\" -l (localizedPanelLabel(\"Dope Sheet\")) -mbv $menusOkayInPanels `;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n            outlinerEditor -e \n                -showShapes 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n"
		+ "                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n                -organizeByLayer 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 0\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n                -showUpstreamCurves 1\n                -showUnitlessCurves 0\n                -showCompounds 1\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n                -autoSelectNewObjects 0\n                -doNotSelectNewObjects 1\n                -dropIsParent 1\n                -transmitFilters 0\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n"
		+ "                -directSelect 0\n                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 0\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                $editorName;\n\n\t\t\t$editorName = ($panelName+\"DopeSheetEd\");\n            dopeSheetEditor -e \n                -displayKeys 1\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 0\n                -displayInfinities 0\n                -autoFit 0\n"
		+ "                -snapTime \"integer\" \n                -snapValue \"none\" \n                -outliner \"dopeSheetPanel1OutlineEd\" \n                -showSummary 1\n                -showScene 0\n                -hierarchyBelow 0\n                -showTicks 1\n                -selectionWindow 0 0 0 0 \n                $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dope Sheet\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n            outlinerEditor -e \n                -showShapes 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n                -organizeByLayer 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 0\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n"
		+ "                -showPublishedAsConnected 0\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n                -showUpstreamCurves 1\n                -showUnitlessCurves 0\n                -showCompounds 1\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n                -autoSelectNewObjects 0\n                -doNotSelectNewObjects 1\n                -dropIsParent 1\n                -transmitFilters 0\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n"
		+ "                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 0\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                $editorName;\n\n\t\t\t$editorName = ($panelName+\"DopeSheetEd\");\n            dopeSheetEditor -e \n                -displayKeys 1\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 0\n                -displayInfinities 0\n                -autoFit 0\n                -snapTime \"integer\" \n                -snapValue \"none\" \n                -outliner \"dopeSheetPanel1OutlineEd\" \n                -showSummary 1\n                -showScene 0\n                -hierarchyBelow 0\n                -showTicks 1\n                -selectionWindow 0 0 0 0 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n"
		+ "\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"clipEditorPanel\" (localizedPanelLabel(\"Trax Editor\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"clipEditorPanel\" -l (localizedPanelLabel(\"Trax Editor\")) -mbv $menusOkayInPanels `;\n\n\t\t\t$editorName = clipEditorNameFromPanel($panelName);\n            clipEditor -e \n                -displayKeys 0\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 0\n                -displayInfinities 0\n                -autoFit 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -manageSequencer 0 \n                $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Trax Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = clipEditorNameFromPanel($panelName);\n            clipEditor -e \n                -displayKeys 0\n                -displayTangents 0\n"
		+ "                -displayActiveKeys 0\n                -displayActiveKeyTangents 0\n                -displayInfinities 0\n                -autoFit 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -manageSequencer 0 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"sequenceEditorPanel\" (localizedPanelLabel(\"Camera Sequencer\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"sequenceEditorPanel\" -l (localizedPanelLabel(\"Camera Sequencer\")) -mbv $menusOkayInPanels `;\n\n\t\t\t$editorName = sequenceEditorNameFromPanel($panelName);\n            clipEditor -e \n                -displayKeys 0\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 0\n                -displayInfinities 0\n                -autoFit 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n"
		+ "                -manageSequencer 1 \n                $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Camera Sequencer\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = sequenceEditorNameFromPanel($panelName);\n            clipEditor -e \n                -displayKeys 0\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 0\n                -displayInfinities 0\n                -autoFit 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -manageSequencer 1 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperGraphPanel\" (localizedPanelLabel(\"Hypergraph Hierarchy\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"hyperGraphPanel\" -l (localizedPanelLabel(\"Hypergraph Hierarchy\")) -mbv $menusOkayInPanels `;\n"
		+ "\t\t\t$editorName = ($panelName+\"HyperGraphEd\");\n            hyperGraph -e \n                -graphLayoutStyle \"hierarchicalLayout\" \n                -orientation \"horiz\" \n                -mergeConnections 0\n                -zoom 1\n                -animateTransition 0\n                -showRelationships 1\n                -showShapes 0\n                -showDeformers 0\n                -showExpressions 0\n                -showConstraints 0\n                -showConnectionFromSelected 0\n                -showConnectionToSelected 0\n                -showConstraintLabels 0\n                -showUnderworld 0\n                -showInvisible 0\n                -transitionFrames 1\n                -opaqueContainers 0\n                -freeform 0\n                -imagePosition 0 0 \n                -imageScale 1\n                -imageEnabled 0\n                -graphType \"DAG\" \n                -heatMapDisplay 0\n                -updateSelection 1\n                -updateNodeAdded 1\n                -useDrawOverrideColor 0\n                -limitGraphTraversal -1\n"
		+ "                -range 0 0 \n                -iconSize \"smallIcons\" \n                -showCachedConnections 0\n                $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypergraph Hierarchy\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"HyperGraphEd\");\n            hyperGraph -e \n                -graphLayoutStyle \"hierarchicalLayout\" \n                -orientation \"horiz\" \n                -mergeConnections 0\n                -zoom 1\n                -animateTransition 0\n                -showRelationships 1\n                -showShapes 0\n                -showDeformers 0\n                -showExpressions 0\n                -showConstraints 0\n                -showConnectionFromSelected 0\n                -showConnectionToSelected 0\n                -showConstraintLabels 0\n                -showUnderworld 0\n                -showInvisible 0\n                -transitionFrames 1\n                -opaqueContainers 0\n                -freeform 0\n"
		+ "                -imagePosition 0 0 \n                -imageScale 1\n                -imageEnabled 0\n                -graphType \"DAG\" \n                -heatMapDisplay 0\n                -updateSelection 1\n                -updateNodeAdded 1\n                -useDrawOverrideColor 0\n                -limitGraphTraversal -1\n                -range 0 0 \n                -iconSize \"smallIcons\" \n                -showCachedConnections 0\n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"visorPanel\" (localizedPanelLabel(\"Visor\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"visorPanel\" -l (localizedPanelLabel(\"Visor\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Visor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"createNodePanel\" (localizedPanelLabel(\"Create Node\")) `;\n"
		+ "\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"createNodePanel\" -l (localizedPanelLabel(\"Create Node\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Create Node\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"polyTexturePlacementPanel\" (localizedPanelLabel(\"UV Editor\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"polyTexturePlacementPanel\" -l (localizedPanelLabel(\"UV Editor\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"UV Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\tif ($useSceneConfig) {\n\t\tscriptedPanel -e -to $panelName;\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"renderWindowPanel\" (localizedPanelLabel(\"Render View\")) `;\n"
		+ "\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"renderWindowPanel\" -l (localizedPanelLabel(\"Render View\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Render View\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"blendShapePanel\" (localizedPanelLabel(\"Blend Shape\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\tblendShapePanel -unParent -l (localizedPanelLabel(\"Blend Shape\")) -mbv $menusOkayInPanels ;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tblendShapePanel -edit -l (localizedPanelLabel(\"Blend Shape\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynRelEdPanel\" (localizedPanelLabel(\"Dynamic Relationships\")) `;\n\tif (\"\" == $panelName) {\n"
		+ "\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"dynRelEdPanel\" -l (localizedPanelLabel(\"Dynamic Relationships\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dynamic Relationships\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"relationshipPanel\" (localizedPanelLabel(\"Relationship Editor\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"relationshipPanel\" -l (localizedPanelLabel(\"Relationship Editor\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Relationship Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"referenceEditorPanel\" (localizedPanelLabel(\"Reference Editor\")) `;\n"
		+ "\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"referenceEditorPanel\" -l (localizedPanelLabel(\"Reference Editor\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Reference Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"componentEditorPanel\" (localizedPanelLabel(\"Component Editor\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"componentEditorPanel\" -l (localizedPanelLabel(\"Component Editor\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Component Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynPaintScriptedPanelType\" (localizedPanelLabel(\"Paint Effects\")) `;\n"
		+ "\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"dynPaintScriptedPanelType\" -l (localizedPanelLabel(\"Paint Effects\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Paint Effects\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"scriptEditorPanel\" (localizedPanelLabel(\"Script Editor\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"scriptEditorPanel\" -l (localizedPanelLabel(\"Script Editor\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Script Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\tif ($useSceneConfig) {\n\t\tscriptedPanel -e -to $panelName;\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"profilerPanel\" (localizedPanelLabel(\"Profiler Tool\")) `;\n"
		+ "\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"profilerPanel\" -l (localizedPanelLabel(\"Profiler Tool\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Profiler Tool\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"Stereo\" (localizedPanelLabel(\"Stereo\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"Stereo\" -l (localizedPanelLabel(\"Stereo\")) -mbv $menusOkayInPanels `;\nstring $editorName = ($panelName+\"Editor\");\n            stereoCameraView -e \n                -camera \"persp\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"wireframe\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n                -headsUpDisplay 1\n"
		+ "                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 1\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 4 4 \n                -bumpResolution 4 4 \n"
		+ "                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 0\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n"
		+ "                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 0\n                -height 0\n                -sceneRenderFilter 0\n                -displayMode \"centerEye\" \n                -viewColor 0 0 0 1 \n                -useCustomBackground 1\n                $editorName;\n            stereoCameraView -e -viewSelected 0 $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Stereo\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "string $editorName = ($panelName+\"Editor\");\n            stereoCameraView -e \n                -camera \"persp\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"wireframe\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 1\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n"
		+ "                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 4 4 \n                -bumpResolution 4 4 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 0\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n"
		+ "                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 0\n"
		+ "                -height 0\n                -sceneRenderFilter 0\n                -displayMode \"centerEye\" \n                -viewColor 0 0 0 1 \n                -useCustomBackground 1\n                $editorName;\n            stereoCameraView -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"multiListerPanel\" (localizedPanelLabel(\"Multilister\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"multiListerPanel\" -l (localizedPanelLabel(\"Multilister\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Multilister\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"devicePanel\" (localizedPanelLabel(\"Devices\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\tdevicePanel -unParent -l (localizedPanelLabel(\"Devices\")) -mbv $menusOkayInPanels ;\n"
		+ "\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tdevicePanel -edit -l (localizedPanelLabel(\"Devices\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperShadePanel\" (localizedPanelLabel(\"Hypershade\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"hyperShadePanel\" -l (localizedPanelLabel(\"Hypershade\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypershade\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"nodeEditorPanel\" (localizedPanelLabel(\"Node Editor\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"nodeEditorPanel\" -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels `;\n"
		+ "\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -keyReleaseCommand \"nodeEdKeyReleaseCommand\" \n                -nodeTitleMode \"name\" \n                -gridSnap 0\n                -gridVisibility 1\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n                -useAssets 1\n                -syncedSelection 1\n                -extendToShapes 1\n                -activeTab -1\n                -editorMode \"default\" \n                $editorName;\n"
		+ "\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -keyReleaseCommand \"nodeEdKeyReleaseCommand\" \n                -nodeTitleMode \"name\" \n                -gridSnap 0\n                -gridVisibility 1\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n                -useAssets 1\n"
		+ "                -syncedSelection 1\n                -extendToShapes 1\n                -activeTab -1\n                -editorMode \"default\" \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\tif ($useSceneConfig) {\n        string $configName = `getPanel -cwl (localizedPanelLabel(\"Current Layout\"))`;\n        if (\"\" != $configName) {\n\t\t\tpanelConfiguration -edit -label (localizedPanelLabel(\"Current Layout\")) \n\t\t\t\t-defaultImage \"vacantCell.xP:/\"\n\t\t\t\t-image \"\"\n\t\t\t\t-sc false\n\t\t\t\t-configString \"global string $gMainPane; paneLayout -e -cn \\\"vertical2\\\" -ps 1 19 100 -ps 2 81 100 $gMainPane;\"\n\t\t\t\t-removeAllPanels\n\t\t\t\t-ap false\n\t\t\t\t\t(localizedPanelLabel(\"Outliner\")) \n\t\t\t\t\t\"outlinerPanel\"\n"
		+ "\t\t\t\t\t\"$panelName = `outlinerPanel -unParent -l (localizedPanelLabel(\\\"Outliner\\\")) -mbv $menusOkayInPanels `;\\n$editorName = $panelName;\\noutlinerEditor -e \\n    -docTag \\\"isolOutln_fromSeln\\\" \\n    -showShapes 1\\n    -showReferenceNodes 1\\n    -showReferenceMembers 1\\n    -showAttributes 0\\n    -showConnected 0\\n    -showAnimCurvesOnly 0\\n    -showMuteInfo 0\\n    -organizeByLayer 1\\n    -showAnimLayerWeight 1\\n    -autoExpandLayers 1\\n    -autoExpand 0\\n    -showDagOnly 1\\n    -showAssets 1\\n    -showContainedOnly 1\\n    -showPublishedAsConnected 0\\n    -showContainerContents 1\\n    -ignoreDagHierarchy 0\\n    -expandConnections 0\\n    -showUpstreamCurves 1\\n    -showUnitlessCurves 1\\n    -showCompounds 1\\n    -showLeafs 1\\n    -showNumericAttrsOnly 0\\n    -highlightActive 1\\n    -autoSelectNewObjects 0\\n    -doNotSelectNewObjects 0\\n    -dropIsParent 1\\n    -transmitFilters 0\\n    -setFilter \\\"defaultSetFilter\\\" \\n    -showSetMembers 1\\n    -allowMultiSelection 1\\n    -alwaysToggleSelect 0\\n    -directSelect 0\\n    -displayMode \\\"DAG\\\" \\n    -expandObjects 0\\n    -setsIgnoreFilters 1\\n    -containersIgnoreFilters 0\\n    -editAttrName 0\\n    -showAttrValues 0\\n    -highlightSecondary 0\\n    -showUVAttrsOnly 0\\n    -showTextureNodesOnly 0\\n    -attrAlphaOrder \\\"default\\\" \\n    -animLayerFilterOptions \\\"allAffecting\\\" \\n    -sortOrder \\\"none\\\" \\n    -longNames 0\\n    -niceNames 1\\n    -showNamespace 1\\n    -showPinIcons 0\\n    -mapMotionTrails 0\\n    -ignoreHiddenAttribute 0\\n    -ignoreOutlinerColor 0\\n    $editorName\"\n"
		+ "\t\t\t\t\t\"outlinerPanel -edit -l (localizedPanelLabel(\\\"Outliner\\\")) -mbv $menusOkayInPanels  $panelName;\\n$editorName = $panelName;\\noutlinerEditor -e \\n    -docTag \\\"isolOutln_fromSeln\\\" \\n    -showShapes 1\\n    -showReferenceNodes 1\\n    -showReferenceMembers 1\\n    -showAttributes 0\\n    -showConnected 0\\n    -showAnimCurvesOnly 0\\n    -showMuteInfo 0\\n    -organizeByLayer 1\\n    -showAnimLayerWeight 1\\n    -autoExpandLayers 1\\n    -autoExpand 0\\n    -showDagOnly 1\\n    -showAssets 1\\n    -showContainedOnly 1\\n    -showPublishedAsConnected 0\\n    -showContainerContents 1\\n    -ignoreDagHierarchy 0\\n    -expandConnections 0\\n    -showUpstreamCurves 1\\n    -showUnitlessCurves 1\\n    -showCompounds 1\\n    -showLeafs 1\\n    -showNumericAttrsOnly 0\\n    -highlightActive 1\\n    -autoSelectNewObjects 0\\n    -doNotSelectNewObjects 0\\n    -dropIsParent 1\\n    -transmitFilters 0\\n    -setFilter \\\"defaultSetFilter\\\" \\n    -showSetMembers 1\\n    -allowMultiSelection 1\\n    -alwaysToggleSelect 0\\n    -directSelect 0\\n    -displayMode \\\"DAG\\\" \\n    -expandObjects 0\\n    -setsIgnoreFilters 1\\n    -containersIgnoreFilters 0\\n    -editAttrName 0\\n    -showAttrValues 0\\n    -highlightSecondary 0\\n    -showUVAttrsOnly 0\\n    -showTextureNodesOnly 0\\n    -attrAlphaOrder \\\"default\\\" \\n    -animLayerFilterOptions \\\"allAffecting\\\" \\n    -sortOrder \\\"none\\\" \\n    -longNames 0\\n    -niceNames 1\\n    -showNamespace 1\\n    -showPinIcons 0\\n    -mapMotionTrails 0\\n    -ignoreHiddenAttribute 0\\n    -ignoreOutlinerColor 0\\n    $editorName\"\n"
		+ "\t\t\t\t-ap false\n\t\t\t\t\t(localizedPanelLabel(\"Persp View\")) \n\t\t\t\t\t\"modelPanel\"\n"
		+ "\t\t\t\t\t\"$panelName = `modelPanel -unParent -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels `;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"default\\\" \\n    -displayAppearance \\\"smoothShaded\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 1\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 0\\n    -activeComponentsXray 0\\n    -displayTextures 0\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 16384\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"base_OpenGL_Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 1\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -greasePencils 1\\n    -shadows 0\\n    -captureSequenceNumber -1\\n    -width 1008\\n    -height 750\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName\"\n"
		+ "\t\t\t\t\t\"modelPanel -edit -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels  $panelName;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"default\\\" \\n    -displayAppearance \\\"smoothShaded\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 1\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 0\\n    -activeComponentsXray 0\\n    -displayTextures 0\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 16384\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"base_OpenGL_Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 1\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -greasePencils 1\\n    -shadows 0\\n    -captureSequenceNumber -1\\n    -width 1008\\n    -height 750\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName\"\n"
		+ "\t\t\t\t$configName;\n\n            setNamedPanelLayout (localizedPanelLabel(\"Current Layout\"));\n        }\n\n        panelHistory -e -clear mainPanelHistory;\n        setFocus `paneLayout -q -p1 $gMainPane`;\n        sceneUIReplacement -deleteRemaining;\n        sceneUIReplacement -clear;\n\t}\n\n\ngrid -spacing 5 -size 12 -divisions 5 -displayAxes yes -displayGridLines yes -displayDivisionLines yes -displayPerspectiveLabels no -displayOrthographicLabels no -displayAxesBold yes -perspectiveLabelPosition axis -orthographicLabelPosition edge;\nviewManip -drawCompass 0 -compassAngle 0 -frontParameters \"\" -homeParameters \"\" -selectionLockParameters \"\";\n}\n");
	setAttr ".st" 3;
createNode script -n "sceneConfigurationScriptNode";
	rename -uid "109AE860-0000-148A-5708-3ED20000025A";
	setAttr ".b" -type "string" "playbackOptions -min 1 -max 24 -ast 1 -aet 48 ";
	setAttr ".st" 6;
createNode lambert -n "Red";
	rename -uid "109AE860-0000-148A-5708-3FF300000263";
	setAttr ".c" -type "float3" 1 0 0 ;
createNode shadingEngine -n "lambert2SG";
	rename -uid "109AE860-0000-148A-5708-3FF300000264";
	setAttr ".ihi" 0;
	setAttr -s 6 ".dsm";
	setAttr ".ro" yes;
createNode materialInfo -n "materialInfo1";
	rename -uid "109AE860-0000-148A-5708-3FF300000265";
createNode lambert -n "Green";
	rename -uid "109AE860-0000-148A-5708-40580000026B";
	setAttr ".c" -type "float3" 0 1 0 ;
	setAttr ".it" -type "float3" 0.30000001 0.30000001 0.30000001 ;
createNode shadingEngine -n "lambert3SG";
	rename -uid "109AE860-0000-148A-5708-40580000026C";
	setAttr ".ihi" 0;
	setAttr ".ro" yes;
createNode materialInfo -n "materialInfo2";
	rename -uid "109AE860-0000-148A-5708-40580000026D";
createNode lambert -n "Blue";
	rename -uid "109AE860-0000-148A-5708-40700000026F";
	setAttr ".c" -type "float3" 0 0 1 ;
	setAttr ".it" -type "float3" 0.44999999 0.44999999 0.44999999 ;
createNode shadingEngine -n "lambert4SG";
	rename -uid "109AE860-0000-148A-5708-407000000270";
	setAttr ".ihi" 0;
	setAttr ".ro" yes;
createNode materialInfo -n "materialInfo3";
	rename -uid "109AE860-0000-148A-5708-407000000271";
createNode lambert -n "Yellow";
	rename -uid "109AE860-0000-148A-5708-408900000273";
	setAttr ".c" -type "float3" 1 1 0 ;
	setAttr ".it" -type "float3" 0.60000002 0.60000002 0.60000002 ;
createNode shadingEngine -n "lambert5SG";
	rename -uid "109AE860-0000-148A-5708-408900000274";
	setAttr ".ihi" 0;
	setAttr ".ro" yes;
createNode materialInfo -n "materialInfo4";
	rename -uid "109AE860-0000-148A-5708-408900000275";
createNode lambert -n "Magenta";
	rename -uid "109AE860-0000-148A-5708-40A300000277";
	setAttr ".c" -type "float3" 1 0 1 ;
	setAttr ".it" -type "float3" 0.75 0.75 0.75 ;
createNode shadingEngine -n "lambert6SG";
	rename -uid "109AE860-0000-148A-5708-40A300000278";
	setAttr ".ihi" 0;
	setAttr ".ro" yes;
createNode materialInfo -n "materialInfo5";
	rename -uid "109AE860-0000-148A-5708-40A300000279";
createNode lambert -n "Cyan";
	rename -uid "109AE860-0000-148A-5708-40B50000027B";
	setAttr ".c" -type "float3" 0 1 1 ;
	setAttr ".it" -type "float3" 0.89999998 0.89999998 0.89999998 ;
createNode shadingEngine -n "lambert7SG";
	rename -uid "109AE860-0000-148A-5708-40B50000027C";
	setAttr ".ihi" 0;
	setAttr ".ro" yes;
createNode materialInfo -n "materialInfo6";
	rename -uid "109AE860-0000-148A-5708-40B50000027D";
createNode groupId -n "groupId21";
	rename -uid "C81B5860-0000-05FF-57A1-03BE00000289";
	setAttr ".ihi" 0;
createNode groupId -n "groupId28";
	rename -uid "C81B5860-0000-05FF-57A1-03CB00000292";
	setAttr ".ihi" 0;
createNode groupId -n "groupId35";
	rename -uid "C81B5860-0000-05FF-57A1-03D10000029B";
	setAttr ".ihi" 0;
createNode nodeGraphEditorInfo -n "hyperShadePrimaryNodeEditorSavedTabsInfo";
	rename -uid "C81B5860-0000-05FF-57A1-04E9000002FB";
	setAttr ".tgi[0].tn" -type "string" "Untitled_1";
	setAttr ".tgi[0].vl" -type "double2" -113.09523360123723 -455.97199742362869 ;
	setAttr ".tgi[0].vh" -type "double2" 571.428548722041 205.97200735773578 ;
createNode polyMapDel -n "polyMapDel1";
	rename -uid "415B68C0-0000-16E9-57B7-9C390000027E";
	setAttr ".uopa" yes;
	setAttr ".ics" -type "componentList" 1 "f[2]";
createNode polyMapDel -n "polyMapDel2";
	rename -uid "415B68C0-0000-16E9-57B7-9CCB00000285";
	setAttr ".uopa" yes;
	setAttr ".ics" -type "componentList" 2 "f[0:1]" "f[3:5]";
createNode slimPartition -n "slimPartition";
	rename -uid "0D9708C0-0000-66E1-59DF-BFE300000280";
	addAttr -ci true -sn "sid" -ln "slimIDs" -dt "stringArray";
	addAttr -ci true -sn "snm" -ln "slimNames" -dt "stringArray";
	addAttr -ci true -sn "sd" -ln "slimData" -dt "string";
	setAttr ".sid" -type "stringArray" 0  ;
	setAttr ".snm" -type "stringArray" 0  ;
	setAttr ".sd" -type "string" "#\n# generated by slim for sdao at Thu Oct 12 12:29:35 PM PDT 2017\n#\nslim 2 TOR slim {\n}\n";
createNode px_renderGlobals -s -n "px_renderGlobals1";
	rename -uid "0D9708C0-0000-66E1-59DF-BFE300000281";
	setAttr ".fgh" -type "string" "studio_shading";
createNode expression -n "BrokenUVs_C3d__HiddenInfoNode";
	rename -uid "0D9708C0-0000-66E1-59DF-C01C0000028F";
	addAttr -ci true -h true -sn "__pxSimNodeEmulate" -ln "__pxSimNodeEmulate" -min 
		0 -max 1 -at "bool";
	addAttr -s false -ci true -h true -sn "_c3dgui_posingMesh" -ln "_c3dgui_posingMesh" 
		-at "message";
	addAttr -s false -ci true -sn "_c3dgui_inputMesh" -ln "_c3dgui_inputMesh" -at "message";
	addAttr -ci true -sn "_c3dgui_inputMesh__broken" -ln "_c3dgui_inputMesh__broken" 
		-dt "string";
	setAttr -k on ".nds";
	setAttr ".__pxSimNodeEmulate" yes;
	setAttr -l on "._c3dgui_inputMesh__broken" -type "string" "qGarmentMeshShape";
createNode displayLayer -n "BrokenUVs_Decimated";
	rename -uid "0D9708C0-0000-66E1-59DF-C01C00000290";
	setAttr ".do" 1;
createNode displayLayer -n "BrokenUVs_Simulation";
	rename -uid "0D9708C0-0000-66E1-59DF-C01C00000291";
	setAttr ".do" 2;
createNode displayLayer -n "BrokenUVs_Render";
	rename -uid "0D9708C0-0000-66E1-59DF-C01C00000292";
	setAttr ".do" 3;
createNode nodeGraphEditorInfo -n "BrokenUVs_hyperShadePrimaryNodeEditorSavedTabsInfo";
	rename -uid "0D9708C0-0000-66E1-59DF-C01C00000296";
	setAttr ".tgi[0].tn" -type "string" "Untitled_1";
	setAttr ".tgi[0].vl" -type "double2" 0 -368.57142857142861 ;
	setAttr ".tgi[0].vh" -type "double2" 124.76190476190476 0 ;
	setAttr -s 32 ".tgi[0].ni";
	setAttr ".tgi[0].ni[0].x" 278.57144165039062;
	setAttr ".tgi[0].ni[0].y" -910;
	setAttr ".tgi[0].ni[0].nvs" 2098;
	setAttr ".tgi[0].ni[1].x" 1.4285714626312256;
	setAttr ".tgi[0].ni[1].y" -905.71429443359375;
	setAttr ".tgi[0].ni[1].nvs" 2098;
	setAttr ".tgi[0].ni[2].x" 590;
	setAttr ".tgi[0].ni[2].y" -1370;
	setAttr ".tgi[0].ni[2].nvs" 2098;
	setAttr ".tgi[0].ni[3].x" 278.57144165039062;
	setAttr ".tgi[0].ni[3].y" -1498.5714111328125;
	setAttr ".tgi[0].ni[3].nvs" 2098;
	setAttr ".tgi[0].ni[4].x" 884.28570556640625;
	setAttr ".tgi[0].ni[4].y" -1241.4285888671875;
	setAttr ".tgi[0].ni[4].nvs" 2098;
	setAttr ".tgi[0].ni[5].x" 590;
	setAttr ".tgi[0].ni[5].y" -1111.4285888671875;
	setAttr ".tgi[0].ni[5].nvs" 2098;
	setAttr ".tgi[0].ni[6].x" 1178.5714111328125;
	setAttr ".tgi[0].ni[6].y" -1051.4285888671875;
	setAttr ".tgi[0].ni[6].nvs" 2098;
	setAttr ".tgi[0].ni[7].x" 884.28570556640625;
	setAttr ".tgi[0].ni[7].y" -921.4285888671875;
	setAttr ".tgi[0].ni[7].nvs" 2098;
	setAttr ".tgi[0].ni[8].x" 1472.857177734375;
	setAttr ".tgi[0].ni[8].y" -861.4285888671875;
	setAttr ".tgi[0].ni[8].nvs" 2098;
	setAttr ".tgi[0].ni[9].x" 1178.5714111328125;
	setAttr ".tgi[0].ni[9].y" -731.4285888671875;
	setAttr ".tgi[0].ni[9].nvs" 2098;
	setAttr ".tgi[0].ni[10].x" 1767.142822265625;
	setAttr ".tgi[0].ni[10].y" -671.4285888671875;
	setAttr ".tgi[0].ni[10].nvs" 2098;
	setAttr ".tgi[0].ni[11].x" 1472.857177734375;
	setAttr ".tgi[0].ni[11].y" -541.4285888671875;
	setAttr ".tgi[0].ni[11].nvs" 2098;
	setAttr ".tgi[0].ni[12].x" 2078.571533203125;
	setAttr ".tgi[0].ni[12].y" -541.4285888671875;
	setAttr ".tgi[0].ni[12].nvs" 2098;
	setAttr ".tgi[0].ni[13].x" 1767.142822265625;
	setAttr ".tgi[0].ni[13].y" -351.42855834960938;
	setAttr ".tgi[0].ni[13].nvs" 2098;
	setAttr ".tgi[0].ni[14].x" 1472.857177734375;
	setAttr ".tgi[0].ni[14].y" -360;
	setAttr ".tgi[0].ni[14].nvs" 2098;
	setAttr ".tgi[0].ni[15].x" 2390;
	setAttr ".tgi[0].ni[15].y" -340;
	setAttr ".tgi[0].ni[15].nvs" 2098;
	setAttr ".tgi[0].ni[16].x" 2078.571533203125;
	setAttr ".tgi[0].ni[16].y" -340;
	setAttr ".tgi[0].ni[16].nvs" 2098;
	setAttr ".tgi[0].ni[17].x" 1767.142822265625;
	setAttr ".tgi[0].ni[17].y" -170;
	setAttr ".tgi[0].ni[17].nvs" 2098;
	setAttr ".tgi[0].ni[18].x" 1767.142822265625;
	setAttr ".tgi[0].ni[18].y" -872.85711669921875;
	setAttr ".tgi[0].ni[18].nvs" 2098;
	setAttr ".tgi[0].ni[19].x" 2078.571533203125;
	setAttr ".tgi[0].ni[19].y" -1000;
	setAttr ".tgi[0].ni[19].nvs" 2098;
	setAttr ".tgi[0].ni[20].x" 1178.5714111328125;
	setAttr ".tgi[0].ni[20].y" -550;
	setAttr ".tgi[0].ni[20].nvs" 2098;
	setAttr ".tgi[0].ni[21].x" 884.28570556640625;
	setAttr ".tgi[0].ni[21].y" -740;
	setAttr ".tgi[0].ni[21].nvs" 2098;
	setAttr ".tgi[0].ni[22].x" 590;
	setAttr ".tgi[0].ni[22].y" -930;
	setAttr ".tgi[0].ni[22].nvs" 2098;
	setAttr ".tgi[0].ni[23].x" 278.57144165039062;
	setAttr ".tgi[0].ni[23].y" -1111.4285888671875;
	setAttr ".tgi[0].ni[23].nvs" 2098;
	setAttr ".tgi[0].ni[24].x" 590;
	setAttr ".tgi[0].ni[24].y" -750;
	setAttr ".tgi[0].ni[24].nvs" 2098;
	setAttr ".tgi[0].ni[25].x" 1.4285714626312256;
	setAttr ".tgi[0].ni[25].y" -1434.2857666015625;
	setAttr ".tgi[0].ni[25].nvs" 2098;
	setAttr ".tgi[0].ni[26].x" 590;
	setAttr ".tgi[0].ni[26].y" -550;
	setAttr ".tgi[0].ni[26].nvs" 2098;
	setAttr ".tgi[0].ni[27].x" 278.57144165039062;
	setAttr ".tgi[0].ni[27].y" -240;
	setAttr ".tgi[0].ni[27].nvs" 2098;
	setAttr ".tgi[0].ni[28].x" 1.4285714626312256;
	setAttr ".tgi[0].ni[28].y" -111.42857360839844;
	setAttr ".tgi[0].ni[28].nvs" 2098;
	setAttr ".tgi[0].ni[29].x" 278.57144165039062;
	setAttr ".tgi[0].ni[29].y" -1.4285714626312256;
	setAttr ".tgi[0].ni[29].nvs" 2098;
	setAttr ".tgi[0].ni[30].x" 1.4285714626312256;
	setAttr ".tgi[0].ni[30].y" -427.14285278320312;
	setAttr ".tgi[0].ni[30].nvs" 2098;
	setAttr ".tgi[0].ni[31].x" 278.57144165039062;
	setAttr ".tgi[0].ni[31].y" -440;
	setAttr ".tgi[0].ni[31].nvs" 2098;
select -ne :time1;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr ".o" 1;
	setAttr ".unw" 1;
select -ne :hardwareRenderingGlobals;
	setAttr ".vac" 2;
select -ne :renderPartition;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 8 ".st";
	setAttr -cb on ".an";
	setAttr -cb on ".pt";
select -ne :renderGlobalsList1;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
select -ne :defaultShaderList1;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 10 ".s";
select -ne :postProcessList1;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -s 2 ".p";
select -ne :defaultRenderingList1;
select -ne :initialShadingGroup;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -av -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -k on ".mwc";
	setAttr -cb on ".an";
	setAttr -cb on ".il";
	setAttr -cb on ".vo";
	setAttr -cb on ".eo";
	setAttr -cb on ".fo";
	setAttr -cb on ".epo";
	setAttr ".ro" yes;
select -ne :initialParticleSE;
	setAttr -k on ".cch";
	setAttr -cb on ".ihi";
	setAttr -k on ".nds";
	setAttr -cb on ".bnm";
	setAttr -k on ".mwc";
	setAttr -cb on ".an";
	setAttr -cb on ".il";
	setAttr -cb on ".vo";
	setAttr -cb on ".eo";
	setAttr -cb on ".fo";
	setAttr -cb on ".epo";
	setAttr ".ro" yes;
select -ne :defaultRenderGlobals;
	setAttr -k on ".cch";
	setAttr -k on ".nds";
	setAttr -k on ".clip";
	setAttr -k on ".edm";
	setAttr -k on ".edl";
	setAttr ".ren" -type "string" "px_render";
	setAttr -av -k on ".esr";
	setAttr -k on ".ors";
	setAttr ".outf" 3;
	setAttr -k on ".gama";
	setAttr -k on ".be";
	setAttr ".ep" 1;
	setAttr -k on ".fec";
	setAttr -k on ".ofc";
	setAttr -k on ".comp";
	setAttr -k on ".cth";
	setAttr -k on ".soll";
	setAttr -k on ".rd";
	setAttr -k on ".lp";
	setAttr -k on ".sp";
	setAttr -k on ".shs";
	setAttr -k on ".lpr";
	setAttr -k on ".mm";
	setAttr -k on ".npu";
	setAttr -k on ".itf";
	setAttr -k on ".shp";
	setAttr -k on ".uf";
	setAttr -k on ".oi";
	setAttr -k on ".rut";
	setAttr -av -k on ".mbf";
	setAttr -k on ".afp";
	setAttr -k on ".pfb";
	setAttr -av -k on ".bll";
	setAttr -k on ".bls";
	setAttr -k on ".smv";
	setAttr -k on ".ubc";
	setAttr -k on ".mbc";
	setAttr -k on ".udbx";
	setAttr -k on ".smc";
	setAttr -k on ".kmv";
	setAttr -k on ".rlen";
	setAttr -av -k on ".frts";
	setAttr -k on ".tlwd";
	setAttr -k on ".tlht";
	setAttr -k on ".jfc";
select -ne :defaultResolution;
	setAttr ".w" 1024;
	setAttr ".h" 1024;
	setAttr ".pa" 1;
	setAttr ".dar" 1;
select -ne :defaultLightSet;
	setAttr -k on ".cch";
	setAttr -k on ".nds";
	setAttr -k on ".mwc";
	setAttr ".ro" yes;
select -ne :defaultColorMgtGlobals;
	setAttr ".cme" no;
select -ne :hardwareRenderGlobals;
	setAttr ".ctrs" 256;
	setAttr ".btrs" 512;
connectAttr "polyMapDel1.out" "OneMissingFaceCubeShape.i";
connectAttr "polyMapDel2.out" "OneAssignedFaceCubeShape.i";
connectAttr ":mentalrayGlobals.msg" ":mentalrayItemsList.glb";
connectAttr ":miDefaultOptions.msg" ":mentalrayItemsList.opt" -na;
connectAttr ":miDefaultFramebuffer.msg" ":mentalrayItemsList.fb" -na;
connectAttr ":miDefaultOptions.msg" ":mentalrayGlobals.opt";
connectAttr ":miDefaultFramebuffer.msg" ":mentalrayGlobals.fb";
relationship "link" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "lambert2SG.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "lambert3SG.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "lambert4SG.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "lambert5SG.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "lambert6SG.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "lambert7SG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "lambert2SG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "lambert3SG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "lambert4SG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "lambert5SG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "lambert6SG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "lambert7SG.message" ":defaultLightSet.message";
connectAttr "layerManager.dli[0]" "defaultLayer.id";
connectAttr "renderLayerManager.rlmi[0]" "defaultRenderLayer.rlid";
connectAttr ":time1.o" "C3d__HiddenInfoNode.tim";
connectAttr "layerManager.dli[1]" "Decimated.id";
connectAttr "layerManager.dli[2]" "Simulation.id";
connectAttr "layerManager.dli[3]" "Render.id";
connectAttr "Red.oc" "lambert2SG.ss";
connectAttr "DefaultUVSetCubeShape.iog" "lambert2SG.dsm" -na;
connectAttr "EmptyDefaultUVSetCubeShape.iog" "lambert2SG.dsm" -na;
connectAttr "OneMissingFaceCubeShape.iog" "lambert2SG.dsm" -na;
connectAttr "OneAssignedFaceCubeShape.iog" "lambert2SG.dsm" -na;
connectAttr "CompressibleUVSetsCubeShape.iog" "lambert2SG.dsm" -na;
connectAttr "SharedFacesCubeShape.iog" "lambert2SG.dsm" -na;
connectAttr "lambert2SG.msg" "materialInfo1.sg";
connectAttr "Red.msg" "materialInfo1.m";
connectAttr "Green.oc" "lambert3SG.ss";
connectAttr "lambert3SG.msg" "materialInfo2.sg";
connectAttr "Green.msg" "materialInfo2.m";
connectAttr "Blue.oc" "lambert4SG.ss";
connectAttr "lambert4SG.msg" "materialInfo3.sg";
connectAttr "Blue.msg" "materialInfo3.m";
connectAttr "Yellow.oc" "lambert5SG.ss";
connectAttr "lambert5SG.msg" "materialInfo4.sg";
connectAttr "Yellow.msg" "materialInfo4.m";
connectAttr "Magenta.oc" "lambert6SG.ss";
connectAttr "lambert6SG.msg" "materialInfo5.sg";
connectAttr "Magenta.msg" "materialInfo5.m";
connectAttr "Cyan.oc" "lambert7SG.ss";
connectAttr "lambert7SG.msg" "materialInfo6.sg";
connectAttr "Cyan.msg" "materialInfo6.m";
connectAttr "polySurfaceShape1.o" "polyMapDel1.ip";
connectAttr "polySurfaceShape2.o" "polyMapDel2.ip";
connectAttr ":time1.o" "BrokenUVs_C3d__HiddenInfoNode.tim";
connectAttr "layerManager.dli[4]" "BrokenUVs_Decimated.id";
connectAttr "layerManager.dli[5]" "BrokenUVs_Simulation.id";
connectAttr "layerManager.dli[6]" "BrokenUVs_Render.id";
connectAttr "lambert2SG.pa" ":renderPartition.st" -na;
connectAttr "lambert3SG.pa" ":renderPartition.st" -na;
connectAttr "lambert4SG.pa" ":renderPartition.st" -na;
connectAttr "lambert5SG.pa" ":renderPartition.st" -na;
connectAttr "lambert6SG.pa" ":renderPartition.st" -na;
connectAttr "lambert7SG.pa" ":renderPartition.st" -na;
connectAttr "Red.msg" ":defaultShaderList1.s" -na;
connectAttr "Green.msg" ":defaultShaderList1.s" -na;
connectAttr "Blue.msg" ":defaultShaderList1.s" -na;
connectAttr "Yellow.msg" ":defaultShaderList1.s" -na;
connectAttr "Magenta.msg" ":defaultShaderList1.s" -na;
connectAttr "Cyan.msg" ":defaultShaderList1.s" -na;
connectAttr "defaultRenderLayer.msg" ":defaultRenderingList1.r" -na;
connectAttr "boxShape.iog" ":initialShadingGroup.dsm" -na;
connectAttr ":perspShape.msg" ":defaultRenderGlobals.sc";
// End of UsdExportUVSetsTest.ma
