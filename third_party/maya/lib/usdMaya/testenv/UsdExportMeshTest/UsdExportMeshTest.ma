//Maya ASCII 2016 scene
//Name: UsdExportMeshTest.ma
//Last modified: Mon, Feb 26, 2018 09:30:37 AM
//Codeset: UTF-8
requires maya "2016";
requires "stereoCamera" "10.0";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2016";
fileInfo "version" "2016";
fileInfo "cutIdentifier" "201610262200-1005964";
fileInfo "osv" "Linux 3.10.0-693.2.2.el7.x86_64 #1 SMP Sat Sep 9 03:55:24 EDT 2017 x86_64";
createNode transform -s -n "persp";
	rename -uid "3440D8C0-0000-61FE-5853-236400000224";
	setAttr ".v" no;
	setAttr ".t" -type "double3" -16.776594230603806 -7.9752912994138558 23.277625201414214 ;
	setAttr ".r" -type "double3" 20.482924915367438 -3.1805546814635168e-15 -104.80508600522955 ;
createNode camera -s -n "perspShape" -p "persp";
	rename -uid "3440D8C0-0000-61FE-5853-236400000225";
	setAttr -k off ".v" no;
	setAttr ".fl" 34.999999999999993;
	setAttr ".coi" 27.322031022595525;
	setAttr ".imn" -type "string" "persp";
	setAttr ".den" -type "string" "persp_depth";
	setAttr ".man" -type "string" "persp_mask";
	setAttr ".tp" -type "double3" -4.2917227768002624 -12.19250397872004 1.37971653312152 ;
	setAttr ".hc" -type "string" "viewSet -p %camera";
createNode transform -s -n "top";
	rename -uid "3440D8C0-0000-61FE-5853-236400000226";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 0 100.1 ;
createNode camera -s -n "topShape" -p "top";
	rename -uid "3440D8C0-0000-61FE-5853-236400000227";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 100.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "top";
	setAttr ".den" -type "string" "top_depth";
	setAttr ".man" -type "string" "top_mask";
	setAttr ".hc" -type "string" "viewSet -t %camera";
	setAttr ".o" yes;
createNode transform -s -n "front";
	rename -uid "3440D8C0-0000-61FE-5853-236400000228";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 -100.1 0 ;
	setAttr ".r" -type "double3" 89.999999999999986 0 0 ;
createNode camera -s -n "frontShape" -p "front";
	rename -uid "3440D8C0-0000-61FE-5853-236400000229";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 100.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "front";
	setAttr ".den" -type "string" "front_depth";
	setAttr ".man" -type "string" "front_mask";
	setAttr ".hc" -type "string" "viewSet -f %camera";
	setAttr ".o" yes;
createNode transform -s -n "side";
	rename -uid "3440D8C0-0000-61FE-5853-23640000022A";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 100.1 0 0 ;
	setAttr ".r" -type "double3" 90 4.7708320221952805e-14 89.999999999999986 ;
createNode camera -s -n "sideShape" -p "side";
	rename -uid "3440D8C0-0000-61FE-5853-23640000022B";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 100.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "side";
	setAttr ".den" -type "string" "side_depth";
	setAttr ".man" -type "string" "side_mask";
	setAttr ".hc" -type "string" "viewSet -s %camera";
	setAttr ".o" yes;
createNode transform -n "UsdExportMeshTest";
	rename -uid "3440D8C0-0000-61FE-5853-25D500000250";
createNode transform -n "poly" -p "UsdExportMeshTest";
	rename -uid "F18048C0-0000-66FA-5854-06E20000025E";
createNode mesh -n "polyShape" -p "poly";
	rename -uid "F18048C0-0000-66FA-5854-06E20000025F";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -sn "USD_subdivisionScheme" -ln "USD_subdivisionScheme" -dt "string";
	setAttr -k off ".v";
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
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 -0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5
		 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 -0.5;
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
	setAttr ".mgi" -type "string" "ID_d97c1ddc-ac7f-457f-b695-b70e17ea65e7";
	setAttr -k on ".USD_subdivisionScheme" -type "string" "none";
createNode transform -n "polyNoNormals" -p "UsdExportMeshTest";
	rename -uid "3440D8C0-0000-61FE-5853-262100000251";
createNode mesh -n "polyNoNormalsShape" -p "polyNoNormals";
	rename -uid "3440D8C0-0000-61FE-5853-262100000252";
	addAttr -ci true -sn "USD_EmitNormals" -ln "USD_EmitNormals" -min 0 -max 1 -at "bool";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -sn "USD_subdivisionScheme" -ln "USD_subdivisionScheme" -dt "string";
	setAttr -k off ".v";
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
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 -0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5
		 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 -0.5;
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
	setAttr -k on ".USD_EmitNormals";
	setAttr ".mgi" -type "string" "ID_015d3451-d9dd-4b20-a91f-d49b8213a692";
	setAttr -k on ".USD_subdivisionScheme" -type "string" "none";
createNode transform -n "subdiv" -p "UsdExportMeshTest";
	rename -uid "F18048C0-0000-66FA-5854-06E700000260";
createNode mesh -n "subdivShape" -p "subdiv";
	rename -uid "F18048C0-0000-66FA-5854-06E700000261";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -sn "USD_subdivisionScheme" -ln "USD_subdivisionScheme" -dt "string";
	setAttr -k off ".v";
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
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 -0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5
		 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 -0.5;
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
	setAttr ".mgi" -type "string" "ID_4f1d0827-5b38-4f6c-be48-d39e42c0ea89";
	setAttr -k on ".USD_subdivisionScheme" -type "string" "catmullClark";
createNode transform -n "unspecified" -p "UsdExportMeshTest";
	rename -uid "3440D8C0-0000-61FE-5853-236C00000248";
createNode mesh -n "unspecifiedShape" -p "unspecified";
	rename -uid "3440D8C0-0000-61FE-5853-236C00000247";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".mgi" -type "string" "ID_ac3a300d-bd34-46a3-9429-3dc78911b8b4";
createNode transform -n "TestNormalsMesh" -p "UsdExportMeshTest";
	rename -uid "723618C0-0000-704C-5872-BC9000000268";
	setAttr ".r" -type "double3" 0 -3.1805546814635168e-15 0 ;
	setAttr ".rp" -type "double3" 5991.8219102288003 609.09342212239301 -1410.4901112184125 ;
	setAttr ".rpt" -type "double3" -7402.3120214472137 5382.7284881064061 2019.5835333408056 ;
	setAttr ".sp" -type "double3" 5991.8219102288003 609.09342212239301 -1410.4901112184125 ;
createNode mesh -n "TestNormalsMeshShape" -p "TestNormalsMesh";
	rename -uid "723618C0-0000-704C-5872-BC9000000269";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	addAttr -ci true -k true -sn "RMNCFaxis" -ln "RMNCFaxis" -dv 1 -at "float";
	addAttr -ci true -sn "USD_subdivisionScheme" -ln "USD_subdivisionScheme" -dt "string";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".pv" -type "double2" 0.5 0.5 ;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".pt[0:7]" -type "float3"  5765.2817 641.37012 -1464.6793 5764.1357 
		641.37549 -1356.3859 6221.2598 640.01709 -1464.6818 6220.1133 640.02197 -1356.3878 6221.2598 576.48242 -1464.1395 6220.1147 
		576.48779 -1355.8461 5765.2817 577.83594 -1464.1381 5764.1367 577.84082 -1355.8441;
	setAttr -s 8 ".vt[0:7]"  -0.60149187 -0.51270145 0.045243949 0.54110885 -0.51270145 0.045243949
		 -0.60149187 0.84138989 0.045243949 0.54110885 0.84138989 0.045243949 -0.60149187 0.84138989 -0.5
		 0.54110885 0.84138989 -0.5 -0.60149187 -0.51270145 -0.5 0.54110885 -0.51270145 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 4 -ch 16 ".fc[0:3]" -type "polyFaces" 
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
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
	setAttr ".db" yes;
	setAttr ".dc" yes;
	setAttr ".vs" 5;
	setAttr ".bw" 3;
	setAttr ".ns" 1;
	setAttr ".ndt" 2;
	setAttr ".dn" yes;
	setAttr ".dr" 1;
	setAttr ".mgi" -type "string" "ID_feecf336-849b-4b53-af4d-ae97ce301741";
	setAttr -k on ".USD_subdivisionScheme" -type "string" "none";
createNode joint -n "joint1" -p "UsdExportMeshTest";
	rename -uid "40621900-0000-7889-5A79-03280000027E";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".uoc" 1;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".jo" -type "double3" 89.999999999999986 0 89.999999999999986 ;
	setAttr ".bps" -type "matrix" 0 1 0 0 0 0 1 0
		 1 0 0 0 0 -1 0 1;
	setAttr ".radi" 0.5;
createNode joint -n "joint2" -p "joint1";
	rename -uid "40621900-0000-7889-5A79-03280000027F";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".uoc" 1;
	setAttr ".oc" 1;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".bps" -type "matrix" 0 1 0 0 0 0 1 0
		 1 0 0 0 0 0 0 1;
	setAttr ".radi" 0.50112332777342816;
createNode joint -n "joint3" -p "joint2";
	rename -uid "40621900-0000-7889-5A79-032800000280";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".uoc" 1;
	setAttr ".oc" 2;
	setAttr ".t" -type "double3" 1 0 0 ;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".jo" -type "double3" -90 -90 0 ;
	setAttr ".bps" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 1 0 1;
	setAttr ".radi" 0.50112332777342816;
createNode transform -n "ExplicitSkelRoot2";
	rename -uid "DA02D900-0000-7B70-5A7B-525A0000029D";
	addAttr -ci true -sn "USD_typeName" -ln "USD_typeName" -dt "string";
	setAttr -k on ".USD_typeName" -type "string" "SkelRoot";
createNode transform -n "ExplicitSkelRoot";
	rename -uid "DA02D900-0000-7B70-5A7B-525A0000029C";
	addAttr -ci true -sn "USD_typeName" -ln "USD_typeName" -dt "string";
	setAttr -k on ".USD_typeName" -type "string" "SkelRoot";
createNode transform -n "mesh" -p "ExplicitSkelRoot";
	rename -uid "DA02D900-0000-7B70-5A7B-52440000029B";
	setAttr -l on ".tx";
	setAttr -l on ".ty";
	setAttr -l on ".tz";
	setAttr -l on ".rx";
	setAttr -l on ".ry";
	setAttr -l on ".rz";
	setAttr -l on ".sx";
	setAttr -l on ".sy";
	setAttr -l on ".sz";
createNode mesh -n "meshShape" -p "mesh";
	rename -uid "DA02D900-0000-7B70-5A7B-52440000029A";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	setAttr -k off ".v";
	setAttr -s 4 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".vcs" 2;
	setAttr ".mgi" -type "string" "ID_4b44e632-16fa-42d6-980a-986dfb4d3e4f";
createNode mesh -n "meshShapeOrig" -p "mesh";
	rename -uid "DA02D900-0000-7B70-5A7B-527A000002A2";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".ds" no;
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".mgi" -type "string" "ID_c1849860-e27d-4a42-bf98-c474d9d3efaa";
createNode joint -n "joint4" -p "ExplicitSkelRoot";
	rename -uid "DA02D900-0000-7B70-5A7B-52660000029D";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".uoc" 1;
	setAttr ".t" -type "double3" 0 5 0 ;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".jo" -type "double3" 0 180 80 ;
	setAttr ".bps" -type "matrix" -0.020929209753067113 -0.99978096010031714 -1.2246467991473532e-16 0 -0.99978096010031714 0.020929209753067224 -1.2325951644078309e-32 0
		 2.5630889732777334e-18 1.2243785526353212e-16 -1 0 -0.0076450012970963321 4.8985203263032 0 1;
	setAttr ".radi" 0.59650560396304841;
createNode joint -n "joint5" -p "joint4";
	rename -uid "DA02D900-0000-7B70-5A7B-52670000029E";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".uoc" 1;
	setAttr ".oc" 1;
	setAttr ".t" -type "double3" 3 0 2 ;
	setAttr ".r" -type "double3" -29.999999999999996 0 -20 ;
	setAttr ".s" -type "double3" 4 4 4 ;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".jo" -type "double3" 180 7.0141242810281726e-15 0 ;
	setAttr ".bps" -type "matrix" 0.0062144400276028487 -0.999980690181137 -1.2241956284801516e-16 0 0.999980690181137 0.0062144400276027377 -1.1914075772559786e-16 0
		 1.1989922617067134e-16 -1.2167680585470244e-16 1 0 -0.067623407585485129 2.033373035421624 9.8607613152626476e-32 1;
	setAttr ".radi" 0.75911548522251659;
createNode joint -n "joint6" -p "joint5";
	rename -uid "DA02D900-0000-7B70-5A7B-526A0000029F";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".uoc" 1;
	setAttr ".oc" 2;
	setAttr ".t" -type "double3" 6 0 7.3568844845778947e-16 ;
	setAttr ".r" -type "double3" 0 0 25 ;
	setAttr ".s" -type "double3" 2 1 1 ;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".jo" -type "double3" 180 0 0 ;
	setAttr ".bps" -type "matrix" -0.0035334455848443211 -0.99999375736166407 -1.2125238887907057e-16 0 -0.99999375736166407 0.0035334455848444321 2.4279309597032621e-16 0
		 -2.4236314158270109e-16 1.2210952813724654e-16 -1 0 -0.030277319790545938 -3.9760769685818715 3.9443045261050599e-31 1;
	setAttr ".radi" 0.50115671627230696;
createNode joint -n "joint7" -p "joint6";
	rename -uid "DA02D900-0000-7B70-5A7B-526B000002A0";
	addAttr -ci true -sn "liw" -ln "lockInfluenceWeights" -min 0 -max 1 -at "bool";
	setAttr ".uoc" 1;
	setAttr ".oc" 3;
	setAttr ".t" -type "double3" 1 0 0 ;
	setAttr ".mnrl" -type "double3" -360 -360 -360 ;
	setAttr ".mxrl" -type "double3" 360 360 360 ;
	setAttr ".jo" -type "double3" 0 180 0 ;
	setAttr ".bps" -type "matrix" 0.99999999999999989 -6.8955258170078094e-17 -1.1989846166796572e-16 0 6.8955258170078082e-17 0.99999999999999989 1.2210952813724649e-16 0
		 1.1989846166796579e-16 -1.2210952813724654e-16 1 0 -0.033889784459489695 -4.9984337676028847 2.9582283945787943e-31 1;
	setAttr ".radi" 0.50115671627230696;
createNode transform -n "ImplicitSkelRoot";
	rename -uid "02D4F900-0000-367F-5A8C-B607000002A9";
createNode transform -n "animatedCube" -p "ImplicitSkelRoot";
	rename -uid "40621900-0000-7889-5A79-03280000027B";
	setAttr -l on ".tx";
	setAttr -l on ".ty";
	setAttr -l on ".tz";
	setAttr -l on ".rx";
	setAttr -l on ".ry";
	setAttr -l on ".rz";
	setAttr -l on ".sx";
	setAttr -l on ".sy";
	setAttr -l on ".sz";
createNode mesh -n "animatedCubeShape" -p "animatedCube";
	rename -uid "40621900-0000-7889-5A79-03280000027C";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	setAttr -k off ".v";
	setAttr -s 4 ".iog[0].og";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".vcs" 2;
	setAttr ".mgi" -type "string" "ID_d023b07f-246a-4448-af7e-1f5d207cdbbe";
createNode mesh -n "animatedCubeShape1Orig" -p "animatedCube";
	rename -uid "40621900-0000-7889-5A79-03280000027D";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".mgi" -type "string" "ID_80e16c30-84b0-4198-9c46-c086d6f8a24c";
createNode lightLinker -s -n "lightLinker1";
	rename -uid "02D4F900-0000-367F-5A8C-B5EA0000026E";
	setAttr -s 4 ".lnk";
	setAttr -s 4 ".slnk";
createNode displayLayerManager -n "layerManager";
	rename -uid "02D4F900-0000-367F-5A8C-B5EA0000026F";
createNode displayLayer -n "defaultLayer";
	rename -uid "3440D8C0-0000-61FE-5853-236400000242";
createNode renderLayerManager -n "renderLayerManager";
	rename -uid "02D4F900-0000-367F-5A8C-B5EA00000271";
createNode renderLayer -n "defaultRenderLayer";
	rename -uid "3440D8C0-0000-61FE-5853-236400000244";
	setAttr ".g" yes;
createNode polyCube -n "polyCube1";
	rename -uid "3440D8C0-0000-61FE-5853-236C00000246";
	setAttr ".ax" -type "double3" 0 0 1 ;
	setAttr ".cuv" 4;
createNode script -n "sceneConfigurationScriptNode";
	rename -uid "3440D8C0-0000-61FE-5853-23710000024B";
	setAttr ".b" -type "string" "playbackOptions -min 1 -max 120 -ast 1 -aet 200 ";
	setAttr ".st" 6;
createNode materialInfo -n "pasted__materialInfo10";
	rename -uid "723618C0-0000-704C-5872-BC900000026A";
createNode shadingEngine -n "pasted__phong4SG";
	rename -uid "723618C0-0000-704C-5872-BC900000026B";
	setAttr ".ihi" 0;
	setAttr ".ro" yes;
createNode nodeGraphEditorInfo -n "hyperShadePrimaryNodeEditorSavedTabsInfo";
	rename -uid "723618C0-0000-704C-5872-C8BD000002D0";
	setAttr ".tgi[0].tn" -type "string" "Untitled_1";
	setAttr ".tgi[0].vl" -type "double2" -349.99998609225014 -324.99998708566085 ;
	setAttr ".tgi[0].vh" -type "double2" 335.71427237419914 340.47617694688279 ;
createNode displayLayer -n "defaultLayer1";
	rename -uid "40621900-0000-7889-5A79-02AA0000026C";
createNode renderLayer -n "defaultRenderLayer1";
	rename -uid "40621900-0000-7889-5A79-02AA0000026D";
	setAttr ".g" yes;
createNode lambert -n "lambert2";
	rename -uid "40621900-0000-7889-5A79-02EC00000275";
createNode shadingEngine -n "lambert1SG";
	rename -uid "40621900-0000-7889-5A79-02EC00000276";
	setAttr ".ihi" 0;
	setAttr -s 5 ".dsm";
	setAttr ".ro" yes;
createNode materialInfo -n "materialInfo1";
	rename -uid "40621900-0000-7889-5A79-02EC00000277";
createNode script -n "uiConfigurationScriptNode";
	rename -uid "40621900-0000-7889-5A79-03130000027A";
createNode polyCube -n "skinCubeAnimated_polyCube1";
	rename -uid "40621900-0000-7889-5A79-032800000287";
	setAttr ".h" 2;
	setAttr ".sh" 2;
	setAttr ".cuv" 4;
createNode animCurveTU -n "skinCubeAnimated_joint1_visibility";
	rename -uid "40621900-0000-7889-5A79-032800000293";
	setAttr ".tan" 9;
	setAttr ".ktv[0]"  1 1;
	setAttr ".kot[0]"  5;
createNode animCurveTL -n "skinCubeAnimated_joint1_translateX";
	rename -uid "40621900-0000-7889-5A79-032800000294";
	setAttr ".tan" 18;
	setAttr -s 3 ".ktv[0:2]"  1 0 10 0 20 0;
createNode animCurveTL -n "skinCubeAnimated_joint1_translateY";
	rename -uid "40621900-0000-7889-5A79-032800000295";
	setAttr ".tan" 18;
	setAttr ".ktv[0]"  1 -1;
createNode animCurveTL -n "skinCubeAnimated_joint1_translateZ";
	rename -uid "40621900-0000-7889-5A79-032800000296";
	setAttr ".tan" 18;
	setAttr -s 2 ".ktv[0:1]"  1 0 10 0;
createNode animCurveTA -n "skinCubeAnimated_joint1_rotateX";
	rename -uid "40621900-0000-7889-5A79-032800000297";
	setAttr ".tan" 18;
	setAttr ".ktv[0]"  1 0;
createNode animCurveTA -n "skinCubeAnimated_joint1_rotateY";
	rename -uid "40621900-0000-7889-5A79-032800000298";
	setAttr ".tan" 18;
	setAttr ".ktv[0]"  1 0;
createNode animCurveTA -n "skinCubeAnimated_joint1_rotateZ";
	rename -uid "40621900-0000-7889-5A79-032800000299";
	setAttr ".tan" 18;
	setAttr ".ktv[0]"  1 0;
createNode animCurveTU -n "skinCubeAnimated_joint1_scaleX";
	rename -uid "40621900-0000-7889-5A79-03280000029A";
	setAttr ".tan" 18;
	setAttr ".ktv[0]"  1 1;
createNode animCurveTU -n "skinCubeAnimated_joint1_scaleY";
	rename -uid "40621900-0000-7889-5A79-03280000029B";
	setAttr ".tan" 18;
	setAttr ".ktv[0]"  1 1;
createNode animCurveTU -n "skinCubeAnimated_joint1_scaleZ";
	rename -uid "40621900-0000-7889-5A79-03280000029C";
	setAttr ".tan" 18;
	setAttr ".ktv[0]"  1 1;
createNode animCurveTA -n "skinCubeAnimated_joint2_rotateX";
	rename -uid "40621900-0000-7889-5A79-03280000029D";
	setAttr ".tan" 18;
	setAttr -s 4 ".ktv[0:3]"  1 0 10 0 20 -28.355336530865241 30 -38.852751322467974;
createNode animCurveTA -n "skinCubeAnimated_joint2_rotateY";
	rename -uid "40621900-0000-7889-5A79-03280000029E";
	setAttr ".tan" 18;
	setAttr -s 4 ".ktv[0:3]"  1 0 10 0 20 17.749 30 17.749;
createNode animCurveTA -n "skinCubeAnimated_joint2_rotateZ";
	rename -uid "40621900-0000-7889-5A79-03280000029F";
	setAttr ".tan" 18;
	setAttr -s 4 ".ktv[0:3]"  1 0 10 20.007123849431142 20 20.007 30 20.007;
createNode animCurveTU -n "skinCubeAnimated_joint2_visibility";
	rename -uid "40621900-0000-7889-5A79-0328000002A0";
	setAttr ".tan" 9;
	setAttr -s 3 ".ktv[0:2]"  1 1 10 1 30 1;
	setAttr -s 3 ".kot[0:2]"  5 5 5;
createNode animCurveTL -n "skinCubeAnimated_joint2_translateX";
	rename -uid "40621900-0000-7889-5A79-0328000002A1";
	setAttr ".tan" 18;
	setAttr -s 3 ".ktv[0:2]"  1 1 10 1 30 1;
createNode animCurveTL -n "skinCubeAnimated_joint2_translateY";
	rename -uid "40621900-0000-7889-5A79-0328000002A2";
	setAttr ".tan" 18;
	setAttr -s 3 ".ktv[0:2]"  1 0 10 0 30 0;
createNode animCurveTL -n "skinCubeAnimated_joint2_translateZ";
	rename -uid "40621900-0000-7889-5A79-0328000002A3";
	setAttr ".tan" 18;
	setAttr -s 3 ".ktv[0:2]"  1 0 10 0 30 0;
createNode animCurveTU -n "skinCubeAnimated_joint2_scaleX";
	rename -uid "40621900-0000-7889-5A79-0328000002A4";
	setAttr ".tan" 18;
	setAttr -s 3 ".ktv[0:2]"  1 1 10 1 30 1;
createNode animCurveTU -n "skinCubeAnimated_joint2_scaleY";
	rename -uid "40621900-0000-7889-5A79-0328000002A5";
	setAttr ".tan" 18;
	setAttr -s 3 ".ktv[0:2]"  1 1 10 1 30 1;
createNode animCurveTU -n "skinCubeAnimated_joint2_scaleZ";
	rename -uid "40621900-0000-7889-5A79-0328000002A6";
	setAttr ".tan" 18;
	setAttr -s 3 ".ktv[0:2]"  1 1 10 1 30 1;
createNode polyCylinder -n "polyCylinder1";
	rename -uid "DA02D900-0000-7B70-5A7B-524400000299";
	setAttr ".r" 0.2;
	setAttr ".h" 10;
	setAttr ".sh" 10;
	setAttr ".sc" 1;
	setAttr ".cuv" 3;
createNode skinCluster -n "skinCluster1";
	rename -uid "151FC8C0-0000-1D0E-5A94-42DD00000303";
	setAttr -s 222 ".wl";
	setAttr -s 4 ".wl[0].w[0:3]"  5.9435394315908558e-07 0.0012035781519909054 0.49939791374703296 0.49939791374703296;
	setAttr -s 4 ".wl[1].w[0:3]"  5.5288720865994032e-07 0.0011229403030823314 0.49943825340485454 0.49943825340485454;
	setAttr -s 4 ".wl[2].w[0:3]"  4.9126671659710998e-07 0.0010024262992175007 0.49949854121703297 0.49949854121703297;
	setAttr -s 4 ".wl[3].w[0:3]"  4.187707145613609e-07 0.00085952361464009777 0.49957002880732265 0.49957002880732265;
	setAttr -s 4 ".wl[4].w[0:3]"  3.4513113676544971e-07 0.00071301393022601273 0.49964332046931864 0.49964332046931864;
	setAttr -s 4 ".wl[5].w[0:3]"  2.7857295240537668e-07 0.00057928916307214279 0.49971021613198757 0.49971021613198779;
	setAttr -s 4 ".wl[6].w[0:3]"  2.2461569276068432e-07 0.00046986135298725902 0.49976495701565998 0.49976495701565998;
	setAttr -s 4 ".wl[7].w[0:3]"  1.8590954576286452e-07 0.00039073238014551123 0.49980454085515441 0.49980454085515441;
	setAttr -s 4 ".wl[8].w[0:3]"  1.6298022469924866e-07 0.0003435819869678071 0.49982812751640382 0.49982812751640382;
	setAttr -s 4 ".wl[9].w[0:3]"  1.5542765871855566e-07 0.00032800341163323211 0.49983592058035403 0.49983592058035403;
	setAttr -s 4 ".wl[10].w[0:3]"  1.6298022469924866e-07 0.0003435819869678071 0.49982812751640382 0.49982812751640382;
	setAttr -s 4 ".wl[11].w[0:3]"  1.8590945572801051e-07 0.00039073219619018457 0.49980454094717691 0.49980454094717713;
	setAttr -s 4 ".wl[12].w[0:3]"  2.2461542968037479e-07 0.00046986081941158633 0.49976495728257936 0.49976495728257936;
	setAttr -s 4 ".wl[13].w[0:3]"  2.7857259539342391e-07 0.00057928844559719994 0.49971021649090369 0.49971021649090369;
	setAttr -s 4 ".wl[14].w[0:3]"  3.4513073069105222e-07 0.00071301312228157069 0.49964332087349395 0.49964332087349383;
	setAttr -s 4 ".wl[15].w[0:3]"  4.1877007360676431e-07 0.00085952235208618588 0.49957002943892009 0.49957002943892009;
	setAttr -s 4 ".wl[16].w[0:3]"  4.9126568854424462e-07 0.0010024242923774758 0.49949854222096696 0.49949854222096696;
	setAttr -s 4 ".wl[17].w[0:3]"  5.5288604778922817e-07 0.0011229380530059824 0.49943825453047319 0.49943825453047308;
	setAttr -s 4 ".wl[18].w[0:3]"  5.9435266971396462e-07 0.001203575695045775 0.49939791497614228 0.49939791497614228;
	setAttr -s 4 ".wl[19].w[0:3]"  6.0898569323695471e-07 0.0012319467380823895 0.49938372213811222 0.49938372213811222;
	setAttr -s 4 ".wl[20].w[0:3]"  1.0441048254751401e-06 0.49429909194808802 0.50444230391095501 0.0012575600361314011;
	setAttr -s 4 ".wl[21].w[0:3]"  9.7728835235946494e-07 0.49412011580825166 0.50469774691386071 0.0011811599895352267;
	setAttr -s 4 ".wl[22].w[0:3]"  8.7757837816257844e-07 0.49380799717591495 0.50512473492742005 0.0010663903182868094;
	setAttr -s 4 ".wl[23].w[0:3]"  7.5950918255550713e-07 0.4933474036122778 0.50572259792624374 0.00092923895229596082;
	setAttr -s 4 ".wl[24].w[0:3]"  6.3851638048365827e-07 0.49272930029950318 0.50648289677500369 0.0007871644091125981;
	setAttr -s 4 ".wl[25].w[0:3]"  5.2794248714525908e-07 0.4919667000495716 0.50737693919237914 0.00065583281556217982;
	setAttr -s 4 ".wl[26].w[0:3]"  4.371495062425369e-07 0.49111657226460925 0.50833617922008323 0.00054681136580127297;
	setAttr -s 4 ".wl[27].w[0:3]"  3.7115381385270743e-07 0.49029914660015184 0.50923366017973359 0.00046682206630081405;
	setAttr -s 4 ".wl[28].w[0:3]"  3.3161193329455387e-07 0.48969063800991292 0.50989046249367487 0.00041856788447893389;
	setAttr -s 4 ".wl[29].w[0:3]"  3.1849983370273382e-07 0.48946323903034372 0.51013393334217993 0.00040250912764256796;
	setAttr -s 4 ".wl[30].w[0:3]"  3.3161193329455387e-07 0.48969063800991292 0.50989046249367487 0.00041856788447893389;
	setAttr -s 4 ".wl[31].w[0:3]"  3.7115363852121347e-07 0.49029914450829254 0.50923366248580426 0.00046682185226455664;
	setAttr -s 4 ".wl[32].w[0:3]"  4.3714900225138111e-07 0.49111656767150103 0.50833618442398587 0.00054681075551086763;
	setAttr -s 4 ".wl[33].w[0:3]"  5.279418158641694e-07 0.4919666955832469 0.50737694446398962 0.00065583201094758699;
	setAttr -s 4 ".wl[34].w[0:3]"  6.3851563170357411e-07 0.49272929668637905 0.50648290127626627 0.00078716352172302352;
	setAttr -s 4 ".wl[35].w[0:3]"  7.5950801849624131e-07 0.49334739947303075 0.50572260343075215 0.00092923758819864471;
	setAttr -s 4 ".wl[36].w[0:3]"  8.7757653178026279e-07 0.49380799214200161 0.5051247421051106 0.0010663881763560068;
	setAttr -s 4 ".wl[37].w[0:3]"  9.7728628537264818e-07 0.49412011121084437 0.50469775389202631 0.0011811576108438475;
	setAttr -s 4 ".wl[38].w[0:3]"  1.0441025676296233e-06 0.49429908754538876 0.50444231090101244 0.0012575574510311548;
	setAttr -s 4 ".wl[39].w[0:3]"  1.0676323621375294e-06 0.49435722232009022 0.50435734128091281 0.0012843687666348588;
	setAttr -s 4 ".wl[40].w[0:3]"  4.6965236612002386e-06 0.99681011007040532 0.0030001871985557548 0.00018500620737767871;
	setAttr -s 4 ".wl[41].w[0:3]"  4.3537610972405416e-06 0.99703410383429869 0.002789926993115545 0.00017161541148859942;
	setAttr -s 4 ".wl[42].w[0:3]"  3.8453504029129566e-06 0.99736824813574798 0.0024761780556585649 0.00015172845819059685;
	setAttr -s 4 ".wl[43].w[0:3]"  3.2490116270279259e-06 0.99776328042683371 0.0021051084893242838 0.0001283620722147948;
	setAttr -s 4 ".wl[44].w[0:3]"  2.6459245974211148e-06 0.99816651477985718 0.0017261558050954231 0.00010468349045006351;
	setAttr -s 4 ".wl[45].w[0:3]"  2.1040439966867896e-06 0.99853240332047044 0.0013821299559763753 8.3362679556470007e-05;
	setAttr -s 4 ".wl[46].w[0:3]"  1.6679351533025672e-06 0.9988296534625426 0.0011025099111158958 6.6168691188169126e-05;
	setAttr -s 4 ".wl[47].w[0:3]"  1.3575611613298846e-06 0.99904291140659507 0.00090182033547998571 5.3910696763579048e-05;
	setAttr -s 4 ".wl[48].w[0:3]"  1.1749984851933527e-06 0.99916908926012216 0.00078304433471046149 4.6691406682188858e-05;
	setAttr -s 4 ".wl[49].w[0:3]"  1.1151237669431186e-06 0.99921060006451046 0.00074396267762868898 4.432213409398477e-05;
	setAttr -s 4 ".wl[50].w[0:3]"  1.1749984851933527e-06 0.99916908926012216 0.00078304433471046149 4.6691406682188858e-05;
	setAttr -s 4 ".wl[51].w[0:3]"  1.3575604973911184e-06 0.99904291186134686 0.00090181990758904623 5.391067056681106e-05;
	setAttr -s 4 ".wl[52].w[0:3]"  1.6679331927971184e-06 0.99882965479518415 0.0011025086576618758 6.6168613961203676e-05;
	setAttr -s 4 ".wl[53].w[0:3]"  2.1040413049876536e-06 0.99853240513297903 0.0013821282519719644 8.3362573744119847e-05;
	setAttr -s 4 ".wl[54].w[0:3]"  2.6459214991875129e-06 0.99816651684497637 0.0017261538645982409 0.0001046833689261613;
	setAttr -s 4 ".wl[55].w[0:3]"  3.2490066934323254e-06 0.99776328368186773 0.0021051054323057258 0.00012836187913311579;
	setAttr -s 4 ".wl[56].w[0:3]"  3.8453424409414475e-06 0.9973682533405539 0.0024761731697850609 0.00015172814722011301;
	setAttr -s 4 ".wl[57].w[0:3]"  4.3537520643770918e-06 0.99703410969674666 0.0027899214919392783 0.00017161505924971444;
	setAttr -s 4 ".wl[58].w[0:3]"  4.6965137312792733e-06 0.996810116484796 0.0030001811809134819 0.00018500582055927533;
	setAttr -s 4 ".wl[59].w[0:3]"  4.8175754370692119e-06 0.9967312395763106 0.0030742105695208423 0.00018973227873148409;
	setAttr -s 4 ".wl[60].w[0:3]"  1.2598145781464488e-05 0.99973154676759568 0.00021474557304641717 4.1109513576361637e-05;
	setAttr -s 4 ".wl[61].w[0:3]"  1.1575514527474944e-05 0.99975326086042782 0.00019739278321961323 3.777084182519986e-05;
	setAttr -s 4 ".wl[62].w[0:3]"  1.0067946272239257e-05 0.99978529099939262 0.0001717916804495771 3.284937388563187e-05;
	setAttr -s 4 ".wl[63].w[0:3]"  8.3164627414506785e-06 0.99982253405483756 0.00014201716880841983 2.7132313612557094e-05;
	setAttr -s 4 ".wl[64].w[0:3]"  6.5685989962430284e-06 0.99985973620171309 0.00011226734219237058 2.1427857098214576e-05;
	setAttr -s 4 ".wl[65].w[0:3]"  5.0250590583245135e-06 0.99989262307094773 8.5960889620905788e-05 1.6390980373090429e-05;
	setAttr -s 4 ".wl[66].w[0:3]"  3.8084959692554183e-06 0.99991856865016593 6.5201208462110409e-05 1.2421645402747237e-05;
	setAttr -s 4 ".wl[67].w[0:3]"  2.9621196959245495e-06 0.99993663433866498 5.0743078564986106e-05 9.6604630740963404e-06;
	setAttr -s 4 ".wl[68].w[0:3]"  2.4743981312193833e-06 0.99994705097256587 4.2405150896325266e-05 8.0694784066337647e-06;
	setAttr -s 4 ".wl[69].w[0:3]"  2.3164435130098533e-06 0.99995042561122183 3.9703703271219108e-05 7.5542419939125109e-06;
	setAttr -s 4 ".wl[70].w[0:3]"  2.4743981312193833e-06 0.99994705097256587 4.2405150896325266e-05 8.0694784066337647e-06;
	setAttr -s 4 ".wl[71].w[0:3]"  2.9621182072233106e-06 0.99993663437035096 5.0743053214635835e-05 9.660458227128402e-06;
	setAttr -s 4 ".wl[72].w[0:3]"  3.8084914426748067e-06 0.99991856874641238 6.5201131478959096e-05 1.2421630665990936e-05;
	setAttr -s 4 ".wl[73].w[0:3]"  5.0250526409426059e-06 0.99989262320723193 8.5960780645637038e-05 1.6390959481525099e-05;
	setAttr -s 4 ".wl[74].w[0:3]"  6.5685913665659589e-06 0.99985973636354664 0.00011226721282757891 2.1427832259238038e-05;
	setAttr -s 4 ".wl[75].w[0:3]"  8.3164502932634407e-06 0.99982253431854706 0.00014201695807341568 2.7132273086222418e-05;
	setAttr -s 4 ".wl[76].w[0:3]"  1.006792583168663e-05 0.99978529143192074 0.00017179133490784042 3.2849307339701909e-05;
	setAttr -s 4 ".wl[77].w[0:3]"  1.1575491026840289e-05 0.9997532613572836 0.000197392386374808 3.7770765314714243e-05;
	setAttr -s 4 ".wl[78].w[0:3]"  1.2598119781722583e-05 0.99973154731695979 0.00021474513432788596 4.110942893059993e-05;
	setAttr -s 4 ".wl[79].w[0:3]"  1.2960389406750116e-05 0.99972385745696424 0.00022088993913286321 4.229221449615539e-05;
	setAttr -s 4 ".wl[80].w[0:3]"  4.3183004132706475e-05 0.99989562606251969 4.6765254399498013e-05 1.4425678948005051e-05;
	setAttr -s 4 ".wl[81].w[0:3]"  3.9378178563567322e-05 0.99990484900150267 4.2625922064944362e-05 1.3146897868789908e-05;
	setAttr -s 4 ".wl[82].w[0:3]"  3.3795499290582909e-05 0.99991837423276553 3.6557568539800737e-05 1.1272699403939274e-05;
	setAttr -s 4 ".wl[83].w[0:3]"  2.7359005151360509e-05 0.99993395651263883 2.9569273550881688e-05 9.1152086587670648e-06;
	setAttr -s 4 ".wl[84].w[0:3]"  2.1007395847468283e-05 0.9999493199433479 2.2682605413355906e-05 6.9900553913938719e-06;
	setAttr -s 4 ".wl[85].w[0:3]"  1.5483950917614644e-05 0.99996266796922173 1.6702532040300515e-05 5.1455478203045951e-06;
	setAttr -s 4 ".wl[86].w[0:3]"  1.1215620274740333e-05 0.99997297387142714 1.2087717077999145e-05 3.7227912202057088e-06;
	setAttr -s 4 ".wl[87].w[0:3]"  8.3129595278256938e-06 0.99997997713679154 8.9531349176506689e-06 2.7567687629008636e-06;
	setAttr -s 4 ".wl[88].w[0:3]"  6.6763276976971072e-06 0.99998392370025679 7.1872601561532474e-06 2.2127118893334924e-06;
	setAttr -s 4 ".wl[89].w[0:3]"  6.1536065083455752e-06 0.99998518382495327 6.6235169990075951e-06 2.0390515394275574e-06;
	setAttr -s 4 ".wl[90].w[0:3]"  6.6763276976971072e-06 0.99998392370025679 7.1872601561532474e-06 2.2127118893334924e-06;
	setAttr -s 4 ".wl[91].w[0:3]"  8.3129552433164602e-06 0.99997997714711007 8.9531303058994719e-06 2.7567673408052443e-06;
	setAttr -s 4 ".wl[92].w[0:3]"  1.1215606844839575e-05 0.99997297390378614 1.2087702611620783e-05 3.7227867574007329e-06;
	setAttr -s 4 ".wl[93].w[0:3]"  1.5483931262553042e-05 0.99996266801661171 1.6702510846753429e-05 5.14554127884499e-06;
	setAttr -s 4 ".wl[94].w[0:3]"  2.1007371747161907e-05 0.99994932000150316 2.2682579393659156e-05 6.990047356074086e-06;
	setAttr -s 4 ".wl[95].w[0:3]"  2.7358964952833791e-05 0.99993395660971718 2.9569230097546483e-05 9.1151952324037519e-06;
	setAttr -s 4 ".wl[96].w[0:3]"  3.3795432278316072e-05 0.99991837439471243 3.6557496022880196e-05 1.1272676986383629e-05;
	setAttr -s 4 ".wl[97].w[0:3]"  3.9378100636245669e-05 0.99990484918993872 4.2625837658953878e-05 1.3146871766126366e-05;
	setAttr -s 4 ".wl[98].w[0:3]"  4.3182917470009952e-05 0.99989562627215245 4.676516048131328e-05 1.4425649896295427e-05;
	setAttr -s 4 ".wl[99].w[0:3]"  4.4533814900639157e-05 0.99989235076044103 4.8235477141613683e-05 1.4879947516715893e-05;
	setAttr -s 4 ".wl[100].w[0:3]"  0.00023116215207028535 0.99974607279085104 1.6245524151258788e-05 6.5195329273758469e-06;
	setAttr -s 4 ".wl[101].w[0:3]"  0.00020951574633406645 0.99976988454705529 1.4700559646480571e-05 5.8991469641326882e-06;
	setAttr -s 4 ".wl[102].w[0:3]"  0.00017784656590916684 0.99980471183771258 1.2447189255116688e-05 4.9944071231849781e-06;
	setAttr -s 4 ".wl[103].w[0:3]"  0.00014152056342857733 0.99984464481859325 9.8734207153488438e-06 3.9611972628415271e-06;
	setAttr -s 4 ".wl[104].w[0:3]"  0.00010596894453031448 0.99988370870288568 7.3670946464326632e-06 2.9552579375324636e-06;
	setAttr -s 4 ".wl[105].w[0:3]"  7.5436589134057132e-05 0.99991724138679572 5.2259566832795261e-06 2.0960673869388028e-06;
	setAttr -s 4 ".wl[106].w[0:3]"  5.2251606899050199e-05 0.99994269309639117 3.608250901225417e-06 1.4470458086709293e-06;
	setAttr -s 4 ".wl[107].w[0:3]"  3.6826545266437683e-05 0.99995961965249658 2.5366209714363511e-06 1.0171812655678847e-06;
	setAttr -s 4 ".wl[108].w[0:3]"  2.8322633178584658e-05 0.99996894870777686 1.9476883087363714e-06 7.8097073593519124e-07;
	setAttr -s 4 ".wl[109].w[0:3]"  2.5647327702747886e-05 0.99997188316302543 1.7627209068219249e-06 7.0678836510874886e-07;
	setAttr -s 4 ".wl[110].w[0:3]"  2.8322633178584658e-05 0.99996894870777686 1.9476883087363714e-06 7.8097073593519124e-07;
	setAttr -s 4 ".wl[111].w[0:3]"  3.6826525857898448e-05 0.99995961967378588 2.536619629248602e-06 1.0171807269941706e-06;
	setAttr -s 4 ".wl[112].w[0:3]"  5.2251544157499644e-05 0.99994269316523399 3.6082465473127915e-06 1.4470440612438918e-06;
	setAttr -s 4 ".wl[113].w[0:3]"  7.5436494462846265e-05 0.99991724149071515 5.2259500842735919e-06 2.0960647378220523e-06;
	setAttr -s 4 ".wl[114].w[0:3]"  0.0001059688251179315 0.99988370883402344 7.367086280332681e-06 2.955254578231747e-06;
	setAttr -s 4 ".wl[115].w[0:3]"  0.00014152036044291552 0.99984464504161286 9.8734064221225801e-06 3.9611915221799875e-06;
	setAttr -s 4 ".wl[116].w[0:3]"  0.00017784622339052722 0.99980471221419176 1.2447165027364667e-05 4.9943973903007001e-06;
	setAttr -s 4 ".wl[117].w[0:3]"  0.00020951534443842452 0.99976988498894614 1.470053111491321e-05 5.8991355003870513e-06;
	setAttr -s 4 ".wl[118].w[0:3]"  0.00023116170346870423 0.99974607328419951 1.6245492231150121e-05 6.5195201006581553e-06;
	setAttr -s 4 ".wl[119].w[0:3]"  0.00023885714927829505 0.99973760678213897 1.6795626238845168e-05 6.740442343963798e-06;
	setAttr -s 4 ".wl[120].w[0:3]"  0.0034559597781165276 0.99653332170357489 7.2709016458824966e-06 3.4476166627289344e-06;
	setAttr -s 4 ".wl[121].w[0:3]"  0.0031288416116318266 0.99686151858053462 6.5392257812469103e-06 3.1005820523560479e-06;
	setAttr -s 4 ".wl[122].w[0:3]"  0.0026474529895991849 0.99734447455809627 5.476087270097816e-06 2.596365034424393e-06;
	setAttr -s 4 ".wl[123].w[0:3]"  0.0020914248091641993 0.99790228150861504 4.2695135983814146e-06 2.0241686223843559e-06;
	setAttr -s 4 ".wl[124].w[0:3]"  0.0015439965899094635 0.99845142485779947 3.1060713953186651e-06 1.472480895805253e-06;
	setAttr -s 4 ".wl[125].w[0:3]"  0.0010727312921566812 0.99892413438981897 2.126356029389576e-06 1.0079619949338971e-06;
	setAttr -s 4 ".wl[126].w[0:3]"  0.00071621554279105426 0.9992817197768642 1.4007301254740713e-06 6.6395021929871566e-07;
	setAttr -s 4 ".wl[127].w[0:3]"  0.00048166964668483209 0.99951695665264495 9.3196731083748163e-07 4.4173335930783955e-07;
	setAttr -s 4 ".wl[128].w[0:3]"  0.00035443859624774139 0.99964455754142034 6.8106282238701322e-07 3.2279950963852168e-07;
	setAttr -s 4 ".wl[129].w[0:3]"  0.00031492218190958557 0.99968418800902392 6.0368637487472463e-07 2.8612269167817946e-07;
	setAttr -s 4 ".wl[130].w[0:3]"  0.00035443859624774139 0.99964455754142034 6.8106282238701322e-07 3.2279950963852168e-07;
	setAttr -s 4 ".wl[131].w[0:3]"  0.00048166939076327417 0.99951695690931164 9.3196680543577474e-07 4.4173311967276255e-07;
	setAttr -s 4 ".wl[132].w[0:3]"  0.0007162146964036219 0.99928172062575438 1.4007284278210816e-06 6.6394941427010081e-07;
	setAttr -s 4 ".wl[133].w[0:3]"  0.0010727299912790893 0.99892413569461969 2.1263533683894305e-06 1.007960732921726e-06;
	setAttr -s 4 ".wl[134].w[0:3]"  0.0015439949264155078 0.99845142652642105 3.1060679172740437e-06 1.4724792460931389e-06;
	setAttr -s 4 ".wl[135].w[0:3]"  0.002091421970321796 0.99790228435640038 4.2695075328792983e-06 2.0241657450135464e-06;
	setAttr -s 4 ".wl[136].w[0:3]"  0.0026474482063177638 0.99734447935674153 5.4760768500062118e-06 2.5963600907253123e-06;
	setAttr -s 4 ".wl[137].w[0:3]"  0.003128836009773078 0.99686152420066609 6.539213388547455e-06 3.1005761722624108e-06;
	setAttr -s 4 ".wl[138].w[0:3]"  0.0034559535450965724 0.99653332795712624 7.2708877216611676e-06 3.4476100555496435e-06;
	setAttr -s 4 ".wl[139].w[0:3]"  0.0035718639935467789 0.99641703272207727 7.5318809895556454e-06 3.5714033863539848e-06;
	setAttr -s 4 ".wl[140].w[0:3]"  0.4913606222762732 0.50863637657036298 1.9588737599877461e-06 1.0422796038963692e-06;
	setAttr -s 4 ".wl[141].w[0:3]"  0.49095800483147173 0.50903931022047766 1.7524950890572728e-06 9.3245296151063467e-07;
	setAttr -s 4 ".wl[142].w[0:3]"  0.49022898032570528 0.50976879223878779 1.4538855116721404e-06 7.7354999530985436e-07;
	setAttr -s 4 ".wl[143].w[0:3]"  0.48907625904099061 0.51092202931189734 1.1172349920412138e-06 5.9441211998309661e-07;
	setAttr -s 4 ".wl[144].w[0:3]"  0.48734574138957032 0.51265303955970576 7.9571559554956121e-07 4.2333512848567904e-07;
	setAttr -s 4 ".wl[145].w[0:3]"  0.48482587189165743 0.51517331836914748 5.2855118575692999e-07 2.8118800944707772e-07;
	setAttr -s 4 ".wl[146].w[0:3]"  0.48131500452383358 0.51868448348328311 3.3420353858710409e-07 1.777893446334243e-07;
	setAttr -s 4 ".wl[147].w[0:3]"  0.47690971084838135 0.52308996515660355 2.1148988400152082e-07 1.1250513106592661e-07;
	setAttr -s 4 ".wl[148].w[0:3]"  0.47265349497961734 0.52734627918918797 1.4741370712446513e-07 7.8417487414555844e-08;
	setAttr -s 4 ".wl[149].w[0:3]"  0.47076963014415252 0.52923017376553338 1.2800030275055642e-07 6.8090011385412772e-08;
	setAttr -s 4 ".wl[150].w[0:3]"  0.47265349497961734 0.52734627918918797 1.4741370712446513e-07 7.8417487414555844e-08;
	setAttr -s 4 ".wl[151].w[0:3]"  0.47690970445923686 0.52308997154592407 2.1148976912410905e-07 1.1250506994206947e-07;
	setAttr -s 4 ".wl[152].w[0:3]"  0.48131499317309923 0.51868449483462864 3.3420313967507903e-07 1.7778913236599866e-07;
	setAttr -s 4 ".wl[153].w[0:3]"  0.48482586252197984 0.51517332773981406 5.2855054024892843e-07 2.8118766593524279e-07;
	setAttr -s 4 ".wl[154].w[0:3]"  0.48734573450746799 0.51265304644313914 7.9571472681271934e-07 4.2333466614757654e-07;
	setAttr -s 4 ".wl[155].w[0:3]"  0.48907625161436996 0.5109220367408861 1.1172334465670967e-06 5.9441129742658536e-07;
	setAttr -s 4 ".wl[156].w[0:3]"  0.49022897153907824 0.50976880102953781 1.4538828208568612e-06 7.7354856306314023e-07;
	setAttr -s 4 ".wl[157].w[0:3]"  0.49095799682141195 0.50903931823549131 1.7524918561392726e-06 9.3245124063220666e-07;
	setAttr -s 4 ".wl[158].w[0:3]"  0.49136061458762931 0.50863638426459901 1.9588701105315938e-06 1.0422776612249534e-06;
	setAttr -s 4 ".wl[159].w[0:3]"  0.49148932086670793 0.5085075649605304 2.032638016558684e-06 1.0815347451218172e-06;
	setAttr -s 4 ".wl[160].w[0:3]"  0.99641424240889886 0.0035833514791219646 1.5238862143273506e-06 8.82225764921278e-07;
	setAttr -s 4 ".wl[161].w[0:3]"  0.99670082148442785 0.0032969814456627606 1.3914969197273706e-06 8.0557298971613525e-07;
	setAttr -s 4 ".wl[162].w[0:3]"  0.99712781070774958 0.0028702991669166617 1.1971031532195315e-06 6.9302218043424326e-07;
	setAttr -s 4 ".wl[163].w[0:3]"  0.99763076956514318 0.0023676946252066001 9.72706097975941e-07 5.6310355232295124e-07;
	setAttr -s 4 ".wl[164].w[0:3]"  0.99814005504655023 0.0018587594222197797 7.5086329629395286e-07 4.3466793361121753e-07;
	setAttr -s 4 ".wl[165].w[0:3]"  0.99859551918283984 0.0014036006844082443 5.574420037923187e-07 3.2269074812853031e-07;
	setAttr -s 4 ".wl[166].w[0:3]"  0.99895744669726338 0.0010419099972110335 4.0744788262034825e-07 2.358576429615353e-07;
	setAttr -s 4 ".wl[167].w[0:3]"  0.9992098593122678 0.00078965911952409153 3.050107819543799e-07 1.7655742608205102e-07;
	setAttr -s 4 ".wl[168].w[0:3]"  0.99935499902188696 0.00064461099039861555 2.4700740833048124e-07 1.4298030616691568e-07;
	setAttr -s 4 ".wl[169].w[0:3]"  0.99940186468008407 0.00059777466361325205 2.2843000773218326e-07 1.3222629499538324e-07;
	setAttr -s 4 ".wl[170].w[0:3]"  0.99935499902188696 0.00064461099039861555 2.4700740833048124e-07 1.4298030616691568e-07;
	setAttr -s 4 ".wl[171].w[0:3]"  0.9992098597075153 0.00078965872452375083 3.050106254539025e-07 1.7655733547728846e-07;
	setAttr -s 4 ".wl[172].w[0:3]"  0.99895744790836771 0.0010419087868769866 4.0744739487850883e-07 2.3585736057731537e-07;
	setAttr -s 4 ".wl[173].w[0:3]"  0.9985955209072771 0.0014035989610914946 5.5744129412347234e-07 3.2269033724011631e-07;
	setAttr -s 4 ".wl[174].w[0:3]"  0.99814005709826092 0.0018587573718754273 7.5086243097356534e-07 4.3466743258239097e-07;
	setAttr -s 4 ".wl[175].w[0:3]"  0.99763077289300972 0.0023676912996102883 9.7270466016897861e-07 5.6310271978400636e-07;
	setAttr -s 4 ".wl[176].w[0:3]"  0.99712781612382484 0.0028702937546167457 1.1971007624014754e-06 6.9302079601630138e-07;
	setAttr -s 4 ".wl[177].w[0:3]"  0.99670082766312129 0.0032969752713508192 1.391494144879039e-06 8.0557138287703318e-07;
	setAttr -s 4 ".wl[178].w[0:3]"  0.99641424920120036 0.0035833446916891448 1.5238831309703657e-06 8.8222397939731155e-07;
	setAttr -s 4 ".wl[179].w[0:3]"  0.99631331058415495 0.0036842091143805635 1.5708712811004905e-06 9.0943018339617217e-07;
	setAttr -s 4 ".wl[180].w[0:3]"  0.99983295702475949 0.00016601578653058541 6.3505337370772887e-07 3.9213533617088782e-07;
	setAttr -s 4 ".wl[181].w[0:3]"  0.99984236692062367 0.00015666558382416662 5.9814991652672474e-07 3.6934563572863379e-07;
	setAttr -s 4 ".wl[182].w[0:3]"  0.99985652303915307 0.0001425989410297437 5.428340333412788e-07 3.3518578381518549e-07;
	setAttr -s 4 ".wl[183].w[0:3]"  0.99987348313753677 0.00012574550541251907 4.768923125659007e-07 2.9446473813961293e-07;
	setAttr -s 4 ".wl[184].w[0:3]"  0.99989112294261473 0.00010821598459556282 4.0871116131520879e-07 2.5236162838322941e-07;
	setAttr -s 4 ".wl[185].w[0:3]"  0.99990752251464243 9.1918306421759274e-05 3.4571670714604343e-07 2.1346222868989316e-07;
	setAttr -s 4 ".wl[186].w[0:3]"  0.99992123549576839 7.8290022170232919e-05 2.9335363981732701e-07 1.8112842165612149e-07;
	setAttr -s 4 ".wl[187].w[0:3]"  0.99993137753941375 6.8210303111017459e-05 2.5482178046889298e-07 1.5733569485193587e-07;
	setAttr -s 4 ".wl[188].w[0:3]"  0.99993753964753151 6.2085922594492935e-05 2.314967701968321e-07 1.4293310378186748e-07;
	setAttr -s 4 ".wl[189].w[0:3]"  0.99993959919028264 6.0038964915217731e-05 2.2371605635190834e-07 1.3812874587368207e-07;
	setAttr -s 4 ".wl[190].w[0:3]"  0.99993753964753151 6.2085922594492935e-05 2.314967701968321e-07 1.4293310378186748e-07;
	setAttr -s 4 ".wl[191].w[0:3]"  0.99993137757137707 6.8210271340867268e-05 2.5482166101393474e-07 1.573356210880729e-07;
	setAttr -s 4 ".wl[192].w[0:3]"  0.9999212355859598 7.8289932526048021e-05 2.9335330147427728e-07 1.8112821272447247e-07;
	setAttr -s 4 ".wl[193].w[0:3]"  0.99990752263212468 9.191818965579798e-05 3.4571626425542872e-07 2.1346195519281554e-07;
	setAttr -s 4 ".wl[194].w[0:3]"  0.999891123070484 0.00010821585751057273 4.0871067650018865e-07 2.5236132898989084e-07;
	setAttr -s 4 ".wl[195].w[0:3]"  0.99987348333230386 0.00012574531184657064 4.7689156997363097e-07 2.9446427954757198e-07;
	setAttr -s 4 ".wl[196].w[0:3]"  0.99985652334319675 0.00014259863887059935 5.4283286835234805e-07 3.3518506435457489e-07;
	setAttr -s 4 ".wl[197].w[0:3]"  0.99984236725667575 0.00015666524986330423 5.9814862366321677e-07 3.6934483728367972e-07;
	setAttr -s 4 ".wl[198].w[0:3]"  0.99983295738939637 0.00016601542416853516 6.3505196740752703e-07 3.921344676604172e-07;
	setAttr -s 4 ".wl[199].w[0:3]"  0.99982965723518191 0.00016929460333320339 6.4801911368622053e-07 4.0014237113327582e-07;
	setAttr -s 4 ".wl[200].w[0:3]"  0.99996322997559572 3.604997671368927e-05 4.3647142792629409e-07 2.8357626264570782e-07;
	setAttr -s 4 ".wl[201].w[0:3]"  0.99996379643358879 3.5495184814915113e-05 4.2940049531348177e-07 2.7898110094428821e-07;
	setAttr -s 4 ".wl[202].w[0:3]"  0.99996467090837049 3.4638691772853068e-05 4.1850155462528424e-07 2.7189830198266327e-07;
	setAttr -s 4 ".wl[203].w[0:3]"  0.99996575920678732 3.3572724192800179e-05 4.0496651706993805e-07 2.6310250274519178e-07;
	setAttr -s 4 ".wl[204].w[0:3]"  0.99996694776378126 3.2408494039695091e-05 3.9022159793310275e-07 2.5352058111910305e-07;
	setAttr -s 4 ".wl[205].w[0:3]"  0.99996811746359637 3.1262671854934479e-05 3.7574887539251148e-07 2.4411567337051314e-07;
	setAttr -s 4 ".wl[206].w[0:3]"  0.9999691564151364 3.0244875539306634e-05 3.6292619349938452e-07 2.3578313076431119e-07;
	setAttr -s 4 ".wl[207].w[0:3]"  0.99996996990084897 2.944791858637692e-05 3.5290767791295153e-07 2.2927288681111565e-07;
	setAttr -s 4 ".wl[208].w[0:3]"  0.99997048702147739 2.8941288658528012e-05 3.465489684881848e-07 2.251408956566416e-07;
	setAttr -s 4 ".wl[209].w[0:3]"  0.99997066427131331 2.8767631695412922e-05 3.4437122333221039e-07 2.2372576796507755e-07;
	setAttr -s 4 ".wl[210].w[0:3]"  0.99997048702147739 2.8941288658528012e-05 3.465489684881848e-07 2.251408956566416e-07;
	setAttr -s 4 ".wl[211].w[0:3]"  0.99996996991096354 2.9447908668522537e-05 3.5290755861633186e-07 2.2927280929921265e-07;
	setAttr -s 4 ".wl[212].w[0:3]"  0.99996915644171547 3.0244849478124498e-05 3.6292587951901982e-07 2.3578292675659806e-07;
	setAttr -s 4 ".wl[213].w[0:3]"  0.99996811749530146 3.126264076898701e-05 3.7574850008023151e-07 2.4411542950980873e-07;
	setAttr -s 4 ".wl[214].w[0:3]"  0.99996694779501671 3.2408463415737498e-05 3.9022122725821335e-07 2.5352034026838766e-07;
	setAttr -s 4 ".wl[215].w[0:3]"  0.99996575925055653 3.3572681282649974e-05 4.0496599643201225e-07 2.6310216444975759e-07;
	setAttr -s 4 ".wl[216].w[0:3]"  0.99996467097239938 3.4638629003149252e-05 4.1850079143722327e-07 2.718978060795666e-07;
	setAttr -s 4 ".wl[217].w[0:3]"  0.99996379650074552 3.5495118981059527e-05 4.2939969347650327e-07 2.789805799231538e-07;
	setAttr -s 4 ".wl[218].w[0:3]"  0.99996323004654131 3.6049907167006431e-05 4.364705800138091e-07 2.8357571168233312e-07;
	setAttr -s 4 ".wl[219].w[0:3]"  0.99996303395445463 3.624195748254363e-05 4.3892033254575069e-07 2.8516773029850847e-07;
	setAttr -s 4 ".wl[220].w[0:3]"  2.7062211640024168e-10 6.0154731422958688e-07 0.49999969909103181 0.49999969909103181;
	setAttr -s 4 ".wl[221].w[0:3]"  0.99999858940711783 1.3833380905567139e-06 1.6522476434351603e-08 1.073231523035616e-08;
	setAttr -s 4 ".pm";
	setAttr ".pm[0]" -type "matrix" -0.020929209753067227 -0.99978096010031736 2.5630889732777469e-18 0 -0.99978096010031736 0.020929209753067116 1.2243785526353214e-16 0
		 -1.2246467991473535e-16 -1.2325951644078312e-32 -1 0 4.8972873510666233 -0.1101654861256416 -5.9974472789885606e-16 1;
	setAttr ".pm[1]" -type "matrix" 0.0062144400276027429 0.99998069018113722 1.1989922617067134e-16 0 -0.99998069018113722 0.0062144400276028539 -1.2167680585470244e-16 0
		 -1.2241956284801519e-16 -1.1914075772559789e-16 1 0 2.0337540129675316 0.054985827007361314 2.5552233030170733e-16 1;
	setAttr ".pm[2]" -type "matrix" -0.0035334455848444303 -0.99999375736166451 -2.4236314158270109e-16 0 -0.99999375736166451 0.0035334455848443193 1.2210952813724651e-16 0
		 -1.2125238887907062e-16 2.4279309597032631e-16 -1 0 -3.9761591306332975 -0.0162278791705519 4.7817877612776545e-16 1;
	setAttr ".pm[3]" -type "matrix" 1 6.5485811218124505e-17 1.1989846166796572e-16 0 -6.5485811218124468e-17 1 -1.2210952813724649e-16 0
		 -1.1989846166796579e-16 1.2210952813724654e-16 1 0 0.033889784459489342 4.9984337676028847 -6.0629305576431607e-16 1;
	setAttr ".gm" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 0 1;
	setAttr -s 4 ".ma";
	setAttr -s 4 ".dpf[0:3]"  4 4 4 4;
	setAttr -s 4 ".lw";
	setAttr -s 4 ".lw";
	setAttr ".mmi" yes;
	setAttr ".mi" 5;
	setAttr ".ucm" yes;
	setAttr -s 4 ".ifcl";
	setAttr -s 4 ".ifcl";
createNode tweak -n "tweak1";
	rename -uid "151FC8C0-0000-1D0E-5A94-42DD00000304";
createNode objectSet -n "skinCluster1Set";
	rename -uid "151FC8C0-0000-1D0E-5A94-42DD00000305";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "skinCluster1GroupId";
	rename -uid "151FC8C0-0000-1D0E-5A94-42DD00000306";
	setAttr ".ihi" 0;
createNode groupParts -n "skinCluster1GroupParts";
	rename -uid "151FC8C0-0000-1D0E-5A94-42DD00000307";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "tweakSet1";
	rename -uid "151FC8C0-0000-1D0E-5A94-42DD00000308";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId2";
	rename -uid "151FC8C0-0000-1D0E-5A94-42DD00000309";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts2";
	rename -uid "151FC8C0-0000-1D0E-5A94-42DD0000030A";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode dagPose -n "bindPose1";
	rename -uid "151FC8C0-0000-1D0E-5A94-42DD0000030B";
	setAttr -s 5 ".wm";
	setAttr ".wm[0]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 0 1;
	setAttr -s 5 ".xm";
	setAttr ".xm[0]" -type "matrix" "xform" 1 1 1 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0 0 0 1 1
		 1 1 yes;
	setAttr ".xm[1]" -type "matrix" "xform" 1 1 1 0 0 0 0 -0.0076450012970963321
		 4.8985203263032 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 -0.69966806067124887 0.71446805728215279 4.284231254833405e-17 4.3748550972180813e-17 1
		 1 1 yes;
	setAttr ".xm[2]" -type "matrix" "xform" 1 1 1 0 0 0 0 2.8657750099522699
		 -2.5673907444456745e-16 -3.5095621930145226e-16 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0.99990789355626142 -0.013572192301916896 -8.3105709299774376e-19 6.1226700064292399e-17 1
		 1 1 yes;
	setAttr ".xm[3]" -type "matrix" "xform" 1 1 1 0 0 0 0 6.009566047635321
		 -8.7985174701543656e-15 7.3568844845778947e-16 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0.99998812224887645 -0.0048739471854073648 -2.9844319099111905e-19 6.1231612654872936e-17 1
		 1 1 yes;
	setAttr ".xm[4]" -type "matrix" "xform" 1 1 1 0 0 0 0 1.0223631812646017
		 -3.0453070620772849e-15 -1.2396397803033984e-16 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 -0.70585641401603616 0.7083549412493868 4.3221239904118383e-17 4.3374230573063646e-17 1
		 1 1 yes;
	setAttr -s 5 ".m";
	setAttr -s 5 ".p";
	setAttr -s 5 ".g[0:4]" yes no no no no;
	setAttr ".bp" yes;
createNode skinCluster -n "skinCluster2";
	rename -uid "151FC8C0-0000-1D0E-5A94-440F0000034A";
	setAttr -s 12 ".wl";
	setAttr -s 3 ".wl[0].w[0:2]"  0.89010989010989017 0.098901098901098883 0.010989010989010997;
	setAttr -s 3 ".wl[1].w[0:2]"  0.89010989010989006 0.098901098901098952 0.010989010989010997;
	setAttr -s 3 ".wl[2].w[0:2]"  0.47368421052631576 0.47368421052631576 0.052631578947368439;
	setAttr -s 3 ".wl[3].w[0:2]"  0.47368421052631582 0.47368421052631582 0.052631578947368383;
	setAttr -s 3 ".wl[4].w[0:2]"  0.052631578947368467 0.47368421052631576 0.47368421052631576;
	setAttr -s 3 ".wl[5].w[0:2]"  0.052631578947368356 0.47368421052631582 0.47368421052631582;
	setAttr -s 3 ".wl[6].w[0:2]"  0.052631578947368467 0.47368421052631576 0.47368421052631576;
	setAttr -s 3 ".wl[7].w[0:2]"  0.052631578947368356 0.47368421052631582 0.47368421052631582;
	setAttr -s 3 ".wl[8].w[0:2]"  0.47368421052631576 0.47368421052631576 0.052631578947368439;
	setAttr -s 3 ".wl[9].w[0:2]"  0.47368421052631582 0.47368421052631582 0.052631578947368383;
	setAttr -s 3 ".wl[10].w[0:2]"  0.89010989010989017 0.098901098901098883 0.010989010989010997;
	setAttr -s 3 ".wl[11].w[0:2]"  0.89010989010989006 0.098901098901098952 0.010989010989010997;
	setAttr -s 3 ".pm";
	setAttr ".pm[0]" -type "matrix" 2.2204460492503131e-16 -2.2204460492503131e-16 1 0 1 4.9303806576313238e-32 -2.2204460492503131e-16 0
		 4.9303806576313238e-32 1 2.2204460492503131e-16 0 1 4.9303806576313238e-32 -2.2204460492503131e-16 1;
	setAttr ".pm[1]" -type "matrix" 2.2204460492503131e-16 -2.2204460492503131e-16 1 0 1 4.9303806576313238e-32 -2.2204460492503131e-16 0
		 4.9303806576313238e-32 1 2.2204460492503131e-16 0 -4.9303806576313238e-32 4.9303806576313238e-32 -2.2204460492503131e-16 1;
	setAttr ".pm[2]" -type "matrix" 1 -4.9303806576313238e-32 -2.2204460492503131e-16 0 0 1 -2.2204460492503126e-16 0
		 2.2204460492503136e-16 2.2204460492503131e-16 1 0 -4.4408920985006257e-16 -1 2.2204460492503136e-16 1;
	setAttr ".gm" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 0 1;
	setAttr -s 3 ".ma";
	setAttr -s 3 ".dpf[0:2]"  4 4 4;
	setAttr -s 3 ".lw";
	setAttr -s 3 ".lw";
	setAttr ".mmi" yes;
	setAttr ".mi" 5;
	setAttr ".ucm" yes;
	setAttr -s 3 ".ifcl";
	setAttr -s 3 ".ifcl";
createNode tweak -n "tweak2";
	rename -uid "151FC8C0-0000-1D0E-5A94-440F0000034B";
createNode objectSet -n "skinCluster2Set";
	rename -uid "151FC8C0-0000-1D0E-5A94-440F0000034C";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "skinCluster2GroupId";
	rename -uid "151FC8C0-0000-1D0E-5A94-440F0000034D";
	setAttr ".ihi" 0;
createNode groupParts -n "skinCluster2GroupParts";
	rename -uid "151FC8C0-0000-1D0E-5A94-440F0000034E";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "tweakSet2";
	rename -uid "151FC8C0-0000-1D0E-5A94-440F0000034F";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId4";
	rename -uid "151FC8C0-0000-1D0E-5A94-440F00000350";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts4";
	rename -uid "151FC8C0-0000-1D0E-5A94-440F00000351";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode dagPose -n "bindPose2";
	rename -uid "151FC8C0-0000-1D0E-5A94-440F00000352";
	setAttr -s 4 ".wm";
	setAttr ".wm[0]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 0 1;
	setAttr -s 4 ".xm";
	setAttr ".xm[0]" -type "matrix" "xform" 1 1 1 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0 0 0 1 1
		 1 1 yes;
	setAttr ".xm[1]" -type "matrix" "xform" 1 1 1 0 0 0 0 0
		 -1 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0.5 0.49999999999999989 0.5 0.50000000000000011 1
		 1 1 yes;
	setAttr ".xm[2]" -type "matrix" "xform" 1 1 1 0 0 0 0 1
		 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0 0 0 1 1
		 1 1 yes;
	setAttr ".xm[3]" -type "matrix" "xform" 1 1 1 0 0 0 0 1
		 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 -0.5 -0.5 -0.49999999999999989 0.50000000000000011 1
		 1 1 yes;
	setAttr -s 4 ".m";
	setAttr -s 4 ".p";
	setAttr -s 4 ".g[0:3]" yes no no no;
	setAttr ".bp" yes;
select -ne :time1;
	setAttr ".o" 1;
	setAttr ".unw" 1;
select -ne :hardwareRenderingGlobals;
	setAttr ".otfna" -type "stringArray" 22 "NURBS Curves" "NURBS Surfaces" "Polygons" "Subdiv Surface" "Particles" "Particle Instance" "Fluids" "Strokes" "Image Planes" "UI" "Lights" "Cameras" "Locators" "Joints" "IK Handles" "Deformers" "Motion Trails" "Components" "Hair Systems" "Follicles" "Misc. UI" "Ornaments"  ;
	setAttr ".otfva" -type "Int32Array" 22 0 1 1 1 1 1
		 1 1 1 0 0 0 0 0 0 0 0 0
		 0 0 0 0 ;
	setAttr ".fprt" yes;
select -ne :renderPartition;
	setAttr -s 3 ".st";
select -ne :renderGlobalsList1;
select -ne :defaultShaderList1;
	setAttr -s 4 ".s";
select -ne :postProcessList1;
	setAttr -s 2 ".p";
select -ne :defaultRenderingList1;
select -ne :initialShadingGroup;
	setAttr -s 2 ".dsm";
	setAttr ".ro" yes;
select -ne :initialParticleSE;
	setAttr ".ro" yes;
select -ne :defaultResolution;
	setAttr ".pa" 1;
select -ne :hardwareRenderGlobals;
	setAttr ".ctrs" 256;
	setAttr ".btrs" 512;
connectAttr "polyCube1.out" "unspecifiedShape.i";
connectAttr "skinCubeAnimated_joint1_rotateX.o" "joint1.rx";
connectAttr "skinCubeAnimated_joint1_rotateY.o" "joint1.ry";
connectAttr "skinCubeAnimated_joint1_rotateZ.o" "joint1.rz";
connectAttr "skinCubeAnimated_joint1_scaleX.o" "joint1.sx";
connectAttr "skinCubeAnimated_joint1_scaleY.o" "joint1.sy";
connectAttr "skinCubeAnimated_joint1_scaleZ.o" "joint1.sz";
connectAttr "skinCubeAnimated_joint1_visibility.o" "joint1.v";
connectAttr "skinCubeAnimated_joint1_translateX.o" "joint1.tx";
connectAttr "skinCubeAnimated_joint1_translateY.o" "joint1.ty";
connectAttr "skinCubeAnimated_joint1_translateZ.o" "joint1.tz";
connectAttr "joint1.s" "joint2.is";
connectAttr "skinCubeAnimated_joint2_scaleX.o" "joint2.sx";
connectAttr "skinCubeAnimated_joint2_scaleY.o" "joint2.sy";
connectAttr "skinCubeAnimated_joint2_scaleZ.o" "joint2.sz";
connectAttr "skinCubeAnimated_joint2_rotateX.o" "joint2.rx";
connectAttr "skinCubeAnimated_joint2_rotateY.o" "joint2.ry";
connectAttr "skinCubeAnimated_joint2_rotateZ.o" "joint2.rz";
connectAttr "skinCubeAnimated_joint2_visibility.o" "joint2.v";
connectAttr "skinCubeAnimated_joint2_translateX.o" "joint2.tx";
connectAttr "skinCubeAnimated_joint2_translateY.o" "joint2.ty";
connectAttr "skinCubeAnimated_joint2_translateZ.o" "joint2.tz";
connectAttr "joint2.s" "joint3.is";
connectAttr "skinCluster1GroupId.id" "meshShape.iog.og[2].gid";
connectAttr "skinCluster1Set.mwc" "meshShape.iog.og[2].gco";
connectAttr "groupId2.id" "meshShape.iog.og[3].gid";
connectAttr "tweakSet1.mwc" "meshShape.iog.og[3].gco";
connectAttr "skinCluster1.og[0]" "meshShape.i";
connectAttr "tweak1.vl[0].vt[0]" "meshShape.twl";
connectAttr "polyCylinder1.out" "meshShapeOrig.i";
connectAttr "joint4.s" "joint5.is";
connectAttr "joint5.s" "joint6.is";
connectAttr "joint6.s" "joint7.is";
connectAttr "skinCluster2GroupId.id" "animatedCubeShape.iog.og[4].gid";
connectAttr "skinCluster2Set.mwc" "animatedCubeShape.iog.og[4].gco";
connectAttr "groupId4.id" "animatedCubeShape.iog.og[5].gid";
connectAttr "tweakSet2.mwc" "animatedCubeShape.iog.og[5].gco";
connectAttr "skinCluster2.og[0]" "animatedCubeShape.i";
connectAttr "tweak2.vl[0].vt[0]" "animatedCubeShape.twl";
connectAttr "skinCubeAnimated_polyCube1.out" "animatedCubeShape1Orig.i";
relationship "link" ":lightLinker1" "pasted__phong4SG.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "lambert1SG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "pasted__phong4SG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "lambert1SG.message" ":defaultLightSet.message";
connectAttr "layerManager.dli[0]" "defaultLayer1.id";
connectAttr "renderLayerManager.rlmi[0]" "defaultRenderLayer1.rlid";
connectAttr ":lambert1.oc" "lambert1SG.ss";
connectAttr "TestNormalsMeshShape.iog" "lambert1SG.dsm" -na;
connectAttr "unspecifiedShape.iog" "lambert1SG.dsm" -na;
connectAttr "subdivShape.iog" "lambert1SG.dsm" -na;
connectAttr "polyNoNormalsShape.iog" "lambert1SG.dsm" -na;
connectAttr "polyShape.iog" "lambert1SG.dsm" -na;
connectAttr "lambert1SG.msg" "materialInfo1.sg";
connectAttr ":lambert1.msg" "materialInfo1.m";
connectAttr "skinCluster1GroupParts.og" "skinCluster1.ip[0].ig";
connectAttr "skinCluster1GroupId.id" "skinCluster1.ip[0].gi";
connectAttr "bindPose1.msg" "skinCluster1.bp";
connectAttr "joint4.wm" "skinCluster1.ma[0]";
connectAttr "joint5.wm" "skinCluster1.ma[1]";
connectAttr "joint6.wm" "skinCluster1.ma[2]";
connectAttr "joint7.wm" "skinCluster1.ma[3]";
connectAttr "joint4.liw" "skinCluster1.lw[0]";
connectAttr "joint5.liw" "skinCluster1.lw[1]";
connectAttr "joint6.liw" "skinCluster1.lw[2]";
connectAttr "joint7.liw" "skinCluster1.lw[3]";
connectAttr "joint4.obcc" "skinCluster1.ifcl[0]";
connectAttr "joint5.obcc" "skinCluster1.ifcl[1]";
connectAttr "joint6.obcc" "skinCluster1.ifcl[2]";
connectAttr "joint7.obcc" "skinCluster1.ifcl[3]";
connectAttr "groupParts2.og" "tweak1.ip[0].ig";
connectAttr "groupId2.id" "tweak1.ip[0].gi";
connectAttr "skinCluster1GroupId.msg" "skinCluster1Set.gn" -na;
connectAttr "meshShape.iog.og[2]" "skinCluster1Set.dsm" -na;
connectAttr "skinCluster1.msg" "skinCluster1Set.ub[0]";
connectAttr "tweak1.og[0]" "skinCluster1GroupParts.ig";
connectAttr "skinCluster1GroupId.id" "skinCluster1GroupParts.gi";
connectAttr "groupId2.msg" "tweakSet1.gn" -na;
connectAttr "meshShape.iog.og[3]" "tweakSet1.dsm" -na;
connectAttr "tweak1.msg" "tweakSet1.ub[0]";
connectAttr "meshShapeOrig.w" "groupParts2.ig";
connectAttr "groupId2.id" "groupParts2.gi";
connectAttr "ExplicitSkelRoot.msg" "bindPose1.m[0]";
connectAttr "joint4.msg" "bindPose1.m[1]";
connectAttr "joint5.msg" "bindPose1.m[2]";
connectAttr "joint6.msg" "bindPose1.m[3]";
connectAttr "joint7.msg" "bindPose1.m[4]";
connectAttr "bindPose1.w" "bindPose1.p[0]";
connectAttr "bindPose1.m[0]" "bindPose1.p[1]";
connectAttr "bindPose1.m[1]" "bindPose1.p[2]";
connectAttr "bindPose1.m[2]" "bindPose1.p[3]";
connectAttr "bindPose1.m[3]" "bindPose1.p[4]";
connectAttr "joint4.bps" "bindPose1.wm[1]";
connectAttr "joint5.bps" "bindPose1.wm[2]";
connectAttr "joint6.bps" "bindPose1.wm[3]";
connectAttr "joint7.bps" "bindPose1.wm[4]";
connectAttr "skinCluster2GroupParts.og" "skinCluster2.ip[0].ig";
connectAttr "skinCluster2GroupId.id" "skinCluster2.ip[0].gi";
connectAttr "bindPose2.msg" "skinCluster2.bp";
connectAttr "joint1.wm" "skinCluster2.ma[0]";
connectAttr "joint2.wm" "skinCluster2.ma[1]";
connectAttr "joint3.wm" "skinCluster2.ma[2]";
connectAttr "joint1.liw" "skinCluster2.lw[0]";
connectAttr "joint2.liw" "skinCluster2.lw[1]";
connectAttr "joint3.liw" "skinCluster2.lw[2]";
connectAttr "joint1.obcc" "skinCluster2.ifcl[0]";
connectAttr "joint2.obcc" "skinCluster2.ifcl[1]";
connectAttr "joint3.obcc" "skinCluster2.ifcl[2]";
connectAttr "groupParts4.og" "tweak2.ip[0].ig";
connectAttr "groupId4.id" "tweak2.ip[0].gi";
connectAttr "skinCluster2GroupId.msg" "skinCluster2Set.gn" -na;
connectAttr "animatedCubeShape.iog.og[4]" "skinCluster2Set.dsm" -na;
connectAttr "skinCluster2.msg" "skinCluster2Set.ub[0]";
connectAttr "tweak2.og[0]" "skinCluster2GroupParts.ig";
connectAttr "skinCluster2GroupId.id" "skinCluster2GroupParts.gi";
connectAttr "groupId4.msg" "tweakSet2.gn" -na;
connectAttr "animatedCubeShape.iog.og[5]" "tweakSet2.dsm" -na;
connectAttr "tweak2.msg" "tweakSet2.ub[0]";
connectAttr "animatedCubeShape1Orig.w" "groupParts4.ig";
connectAttr "groupId4.id" "groupParts4.gi";
connectAttr "UsdExportMeshTest.msg" "bindPose2.m[0]";
connectAttr "joint1.msg" "bindPose2.m[1]";
connectAttr "joint2.msg" "bindPose2.m[2]";
connectAttr "joint3.msg" "bindPose2.m[3]";
connectAttr "bindPose2.w" "bindPose2.p[0]";
connectAttr "bindPose2.m[0]" "bindPose2.p[1]";
connectAttr "bindPose2.m[1]" "bindPose2.p[2]";
connectAttr "bindPose2.m[2]" "bindPose2.p[3]";
connectAttr "joint1.bps" "bindPose2.wm[1]";
connectAttr "joint2.bps" "bindPose2.wm[2]";
connectAttr "joint3.bps" "bindPose2.wm[3]";
connectAttr "lambert1SG.pa" ":renderPartition.st" -na;
connectAttr "defaultRenderLayer1.msg" ":defaultRenderingList1.r" -na;
connectAttr "animatedCubeShape.iog" ":initialShadingGroup.dsm" -na;
connectAttr "meshShape.iog" ":initialShadingGroup.dsm" -na;
// End of UsdExportMeshTest.ma
