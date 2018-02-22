//Maya ASCII 2018ff07 scene
//Name: UsdExportMeshTest.ma
//Last modified: Tue, Feb 20, 2018 03:58:04 PM
//Codeset: UTF-8
requires maya "2018ff07";
requires "stereoCamera" "10.0";
requires "stereoCamera" "10.0";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2018";
fileInfo "version" "2018";
fileInfo "cutIdentifier" "201711281015-8e846c9074";
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
createNode shapeEditorManager -n "shapeEditorManager";
	rename -uid "02D4F900-0000-367F-5A8C-B5EA00000278";
createNode poseInterpolatorManager -n "poseInterpolatorManager";
	rename -uid "02D4F900-0000-367F-5A8C-B5EA00000279";
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
	setAttr ".b" -type "string" (
		"// Maya Mel UI Configuration File.\n//\n//  This script is machine generated.  Edit at your own risk.\n//\n//\n\nglobal string $gMainPane;\nif (`paneLayout -exists $gMainPane`) {\n\n\tglobal int $gUseScenePanelConfig;\n\tint    $useSceneConfig = $gUseScenePanelConfig;\n\tint    $nodeEditorPanelVisible = stringArrayContains(\"nodeEditorPanel1\", `getPanel -vis`);\n\tint    $nodeEditorWorkspaceControlOpen = (`workspaceControl -exists nodeEditorPanel1Window` && `workspaceControl -q -visible nodeEditorPanel1Window`);\n\tint    $menusOkayInPanels = `optionVar -q allowMenusInPanels`;\n\tint    $nVisPanes = `paneLayout -q -nvp $gMainPane`;\n\tint    $nPanes = 0;\n\tstring $editorName;\n\tstring $panelName;\n\tstring $itemFilterName;\n\tstring $panelConfig;\n\n\t//\n\t//  get current state of the UI\n\t//\n\tsceneUIReplacement -update $gMainPane;\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Top View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Top View\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"top\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n"
		+ "            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n"
		+ "            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Side View\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Side View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"side\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n"
		+ "            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n"
		+ "            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Front View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Front View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"front\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n"
		+ "            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n"
		+ "            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n"
		+ "            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Persp View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Persp View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"persp\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n"
		+ "            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 1\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n"
		+ "            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n"
		+ "            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1164\n            -height 970\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"ToggledOutliner\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\toutlinerPanel -edit -l (localizedPanelLabel(\"ToggledOutliner\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        outlinerEditor -e \n            -showShapes 0\n            -showAssignedMaterials 0\n            -showTimeEditor 1\n            -showReferenceNodes 1\n            -showReferenceMembers 1\n            -showAttributes 0\n            -showConnected 0\n            -showAnimCurvesOnly 0\n"
		+ "            -showMuteInfo 0\n            -organizeByLayer 1\n            -organizeByClip 1\n            -showAnimLayerWeight 1\n            -autoExpandLayers 1\n            -autoExpand 0\n            -showDagOnly 0\n            -showAssets 1\n            -showContainedOnly 1\n            -showPublishedAsConnected 0\n            -showParentContainers 0\n            -showContainerContents 1\n            -ignoreDagHierarchy 0\n            -expandConnections 0\n            -showUpstreamCurves 1\n            -showUnitlessCurves 1\n            -showCompounds 1\n            -showLeafs 1\n            -showNumericAttrsOnly 0\n            -highlightActive 1\n            -autoSelectNewObjects 0\n            -doNotSelectNewObjects 0\n            -dropIsParent 1\n            -transmitFilters 0\n            -setFilter \"defaultSetFilter\" \n            -showSetMembers 1\n            -allowMultiSelection 1\n            -alwaysToggleSelect 0\n            -directSelect 0\n            -isSet 0\n            -isSetMember 0\n            -displayMode \"DAG\" \n            -expandObjects 0\n"
		+ "            -setsIgnoreFilters 1\n            -containersIgnoreFilters 0\n            -editAttrName 0\n            -showAttrValues 0\n            -highlightSecondary 0\n            -showUVAttrsOnly 0\n            -showTextureNodesOnly 0\n            -attrAlphaOrder \"default\" \n            -animLayerFilterOptions \"allAffecting\" \n            -sortOrder \"none\" \n            -longNames 0\n            -niceNames 1\n            -showNamespace 1\n            -showPinIcons 0\n            -mapMotionTrails 0\n            -ignoreHiddenAttribute 0\n            -ignoreOutlinerColor 0\n            -renderFilterVisible 0\n            -renderFilterIndex 0\n            -selectionOrder \"chronological\" \n            -expandAttribute 0\n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"Outliner\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\toutlinerPanel -edit -l (localizedPanelLabel(\"Outliner\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\t$editorName = $panelName;\n        outlinerEditor -e \n            -showShapes 0\n            -showAssignedMaterials 0\n            -showTimeEditor 1\n            -showReferenceNodes 0\n            -showReferenceMembers 0\n            -showAttributes 0\n            -showConnected 0\n            -showAnimCurvesOnly 0\n            -showMuteInfo 0\n            -organizeByLayer 1\n            -organizeByClip 1\n            -showAnimLayerWeight 1\n            -autoExpandLayers 1\n            -autoExpand 0\n            -showDagOnly 1\n            -showAssets 1\n            -showContainedOnly 1\n            -showPublishedAsConnected 0\n            -showParentContainers 0\n            -showContainerContents 1\n            -ignoreDagHierarchy 0\n            -expandConnections 0\n            -showUpstreamCurves 1\n            -showUnitlessCurves 1\n            -showCompounds 1\n            -showLeafs 1\n            -showNumericAttrsOnly 0\n            -highlightActive 1\n            -autoSelectNewObjects 0\n            -doNotSelectNewObjects 0\n            -dropIsParent 1\n"
		+ "            -transmitFilters 0\n            -setFilter \"defaultSetFilter\" \n            -showSetMembers 1\n            -allowMultiSelection 1\n            -alwaysToggleSelect 0\n            -directSelect 0\n            -displayMode \"DAG\" \n            -expandObjects 0\n            -setsIgnoreFilters 1\n            -containersIgnoreFilters 0\n            -editAttrName 0\n            -showAttrValues 0\n            -highlightSecondary 0\n            -showUVAttrsOnly 0\n            -showTextureNodesOnly 0\n            -attrAlphaOrder \"default\" \n            -animLayerFilterOptions \"allAffecting\" \n            -sortOrder \"none\" \n            -longNames 0\n            -niceNames 1\n            -showNamespace 1\n            -showPinIcons 0\n            -mapMotionTrails 0\n            -ignoreHiddenAttribute 0\n            -ignoreOutlinerColor 0\n            -renderFilterVisible 0\n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"graphEditor\" (localizedPanelLabel(\"Graph Editor\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Graph Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n            outlinerEditor -e \n                -showShapes 1\n                -showAssignedMaterials 0\n                -showTimeEditor 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n                -organizeByLayer 1\n                -organizeByClip 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 1\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showParentContainers 1\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n"
		+ "                -showUpstreamCurves 1\n                -showUnitlessCurves 1\n                -showCompounds 0\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n                -autoSelectNewObjects 1\n                -doNotSelectNewObjects 0\n                -dropIsParent 1\n                -transmitFilters 1\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n"
		+ "                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 1\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                -renderFilterVisible 0\n                $editorName;\n\n\t\t\t$editorName = ($panelName+\"GraphEd\");\n            animCurveEditor -e \n                -displayKeys 1\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 1\n                -displayInfinities 0\n                -displayValues 0\n                -autoFit 1\n                -snapTime \"integer\" \n                -snapValue \"none\" \n                -showResults \"off\" \n                -showBufferCurves \"off\" \n                -smoothness \"fine\" \n                -resultSamples 1\n                -resultScreenSamples 0\n                -resultUpdate \"delayed\" \n                -showUpstreamCurves 1\n                -showCurveNames 0\n                -showActiveCurveNames 0\n                -stackedCurves 0\n"
		+ "                -stackedCurvesMin -1\n                -stackedCurvesMax 1\n                -stackedCurvesSpace 0.2\n                -displayNormalized 0\n                -preSelectionHighlight 0\n                -constrainDrag 0\n                -classicMode 1\n                -valueLinesToggle 1\n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dopeSheetPanel\" (localizedPanelLabel(\"Dope Sheet\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dope Sheet\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n            outlinerEditor -e \n                -showShapes 1\n                -showAssignedMaterials 0\n                -showTimeEditor 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n"
		+ "                -showMuteInfo 0\n                -organizeByLayer 1\n                -organizeByClip 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 0\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showParentContainers 1\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n                -showUpstreamCurves 1\n                -showUnitlessCurves 0\n                -showCompounds 1\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n                -autoSelectNewObjects 0\n                -doNotSelectNewObjects 1\n                -dropIsParent 1\n                -transmitFilters 0\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n"
		+ "                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 0\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                -renderFilterVisible 0\n                $editorName;\n\n\t\t\t$editorName = ($panelName+\"DopeSheetEd\");\n            dopeSheetEditor -e \n                -displayKeys 1\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 0\n                -displayInfinities 0\n"
		+ "                -displayValues 0\n                -autoFit 0\n                -snapTime \"integer\" \n                -snapValue \"none\" \n                -outliner \"dopeSheetPanel1OutlineEd\" \n                -showSummary 1\n                -showScene 0\n                -hierarchyBelow 0\n                -showTicks 1\n                -selectionWindow 0 0 0 0 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"timeEditorPanel\" (localizedPanelLabel(\"Time Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Time Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"clipEditorPanel\" (localizedPanelLabel(\"Trax Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Trax Editor\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\n\t\t\t$editorName = clipEditorNameFromPanel($panelName);\n            clipEditor -e \n                -displayKeys 0\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 0\n                -displayInfinities 0\n                -displayValues 0\n                -autoFit 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -initialized 0\n                -manageSequencer 0 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"sequenceEditorPanel\" (localizedPanelLabel(\"Camera Sequencer\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Camera Sequencer\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = sequenceEditorNameFromPanel($panelName);\n            clipEditor -e \n                -displayKeys 0\n                -displayTangents 0\n                -displayActiveKeys 0\n"
		+ "                -displayActiveKeyTangents 0\n                -displayInfinities 0\n                -displayValues 0\n                -autoFit 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -initialized 0\n                -manageSequencer 1 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperGraphPanel\" (localizedPanelLabel(\"Hypergraph Hierarchy\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypergraph Hierarchy\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"HyperGraphEd\");\n            hyperGraph -e \n                -graphLayoutStyle \"hierarchicalLayout\" \n                -orientation \"horiz\" \n                -mergeConnections 0\n                -zoom 1\n                -animateTransition 0\n                -showRelationships 1\n                -showShapes 0\n                -showDeformers 0\n"
		+ "                -showExpressions 0\n                -showConstraints 0\n                -showConnectionFromSelected 0\n                -showConnectionToSelected 0\n                -showConstraintLabels 0\n                -showUnderworld 0\n                -showInvisible 0\n                -transitionFrames 1\n                -opaqueContainers 0\n                -freeform 0\n                -imagePosition 0 0 \n                -imageScale 1\n                -imageEnabled 0\n                -graphType \"DAG\" \n                -heatMapDisplay 0\n                -updateSelection 1\n                -updateNodeAdded 1\n                -useDrawOverrideColor 0\n                -limitGraphTraversal -1\n                -range 0 0 \n                -iconSize \"smallIcons\" \n                -showCachedConnections 0\n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperShadePanel\" (localizedPanelLabel(\"Hypershade\")) `;\n\tif (\"\" != $panelName) {\n"
		+ "\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypershade\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"visorPanel\" (localizedPanelLabel(\"Visor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Visor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"nodeEditorPanel\" (localizedPanelLabel(\"Node Editor\")) `;\n\tif ($nodeEditorPanelVisible || $nodeEditorWorkspaceControlOpen) {\n\t\tif (\"\" == $panelName) {\n\t\t\tif ($useSceneConfig) {\n\t\t\t\t$panelName = `scriptedPanel -unParent  -type \"nodeEditorPanel\" -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels `;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n"
		+ "                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -connectNodeOnCreation 0\n                -connectOnDrop 0\n                -copyConnectionsOnPaste 0\n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -nodeTitleMode \"name\" \n                -gridSnap 0\n                -gridVisibility 1\n                -crosshairOnEdgeDragging 0\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n                -useAssets 1\n                -syncedSelection 1\n                -extendToShapes 1\n                -editorMode \"default\" \n                $editorName;\n\t\t\t}\n\t\t} else {\n\t\t\t$label = `panel -q -label $panelName`;\n"
		+ "\t\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -connectNodeOnCreation 0\n                -connectOnDrop 0\n                -copyConnectionsOnPaste 0\n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -nodeTitleMode \"name\" \n                -gridSnap 0\n                -gridVisibility 1\n                -crosshairOnEdgeDragging 0\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n"
		+ "                -useAssets 1\n                -syncedSelection 1\n                -extendToShapes 1\n                -editorMode \"default\" \n                $editorName;\n\t\t\tif (!$useSceneConfig) {\n\t\t\t\tpanel -e -l $label $panelName;\n\t\t\t}\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"createNodePanel\" (localizedPanelLabel(\"Create Node\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Create Node\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"polyTexturePlacementPanel\" (localizedPanelLabel(\"UV Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"UV Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"renderWindowPanel\" (localizedPanelLabel(\"Render View\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Render View\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"shapePanel\" (localizedPanelLabel(\"Shape Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tshapePanel -edit -l (localizedPanelLabel(\"Shape Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"posePanel\" (localizedPanelLabel(\"Pose Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tposePanel -edit -l (localizedPanelLabel(\"Pose Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynRelEdPanel\" (localizedPanelLabel(\"Dynamic Relationships\")) `;\n\tif (\"\" != $panelName) {\n"
		+ "\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dynamic Relationships\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"relationshipPanel\" (localizedPanelLabel(\"Relationship Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Relationship Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"referenceEditorPanel\" (localizedPanelLabel(\"Reference Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Reference Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"componentEditorPanel\" (localizedPanelLabel(\"Component Editor\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Component Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynPaintScriptedPanelType\" (localizedPanelLabel(\"Paint Effects\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Paint Effects\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"scriptEditorPanel\" (localizedPanelLabel(\"Script Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Script Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"profilerPanel\" (localizedPanelLabel(\"Profiler Tool\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Profiler Tool\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"contentBrowserPanel\" (localizedPanelLabel(\"Content Browser\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Content Browser\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"Stereo\" (localizedPanelLabel(\"Stereo\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Stereo\")) -mbv $menusOkayInPanels  $panelName;\nstring $editorName = ($panelName+\"Editor\");\n            stereoCameraView -e \n                -camera \"persp\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n"
		+ "                -displayAppearance \"wireframe\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 1\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n"
		+ "                -maxConstantTransparency 1\n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 4 4 \n                -bumpResolution 4 4 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 0\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -controllers 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n"
		+ "                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 0\n                -height 0\n                -sceneRenderFilter 0\n                -displayMode \"centerEye\" \n                -viewColor 0 0 0 1 \n"
		+ "                -useCustomBackground 1\n                $editorName;\n            stereoCameraView -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\tif ($useSceneConfig) {\n        string $configName = `getPanel -cwl (localizedPanelLabel(\"Current Layout\"))`;\n        if (\"\" != $configName) {\n\t\t\tpanelConfiguration -edit -label (localizedPanelLabel(\"Current Layout\")) \n\t\t\t\t-userCreated false\n\t\t\t\t-defaultImage \"vacantCell.xP:/\"\n\t\t\t\t-image \"\"\n\t\t\t\t-sc false\n\t\t\t\t-configString \"global string $gMainPane; paneLayout -e -cn \\\"single\\\" -ps 1 100 100 $gMainPane;\"\n\t\t\t\t-removeAllPanels\n\t\t\t\t-ap true\n\t\t\t\t\t(localizedPanelLabel(\"Persp View\")) \n\t\t\t\t\t\"modelPanel\"\n"
		+ "\t\t\t\t\t\"$panelName = `modelPanel -unParent -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels `;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"default\\\" \\n    -displayAppearance \\\"smoothShaded\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 0\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 0\\n    -activeComponentsXray 0\\n    -displayTextures 1\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 16384\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"base_OpenGL_Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -controllers 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 1\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -greasePencils 1\\n    -shadows 0\\n    -captureSequenceNumber -1\\n    -width 1164\\n    -height 970\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName\"\n"
		+ "\t\t\t\t\t\"modelPanel -edit -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels  $panelName;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"default\\\" \\n    -displayAppearance \\\"smoothShaded\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 0\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 0\\n    -activeComponentsXray 0\\n    -displayTextures 1\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 16384\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"base_OpenGL_Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -controllers 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 1\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -greasePencils 1\\n    -shadows 0\\n    -captureSequenceNumber -1\\n    -width 1164\\n    -height 970\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName\"\n"
		+ "\t\t\t\t$configName;\n\n            setNamedPanelLayout (localizedPanelLabel(\"Current Layout\"));\n        }\n\n        panelHistory -e -clear mainPanelHistory;\n        sceneUIReplacement -clear;\n\t}\n\n\ngrid -spacing 5 -size 12 -divisions 5 -displayAxes yes -displayGridLines yes -displayDivisionLines yes -displayPerspectiveLabels yes -displayOrthographicLabels yes -displayAxesBold yes -perspectiveLabelPosition axis -orthographicLabelPosition axis;\nviewManip -drawCompass 0 -compassAngle 0 -frontParameters \"\" -homeParameters \"\" -selectionLockParameters \"\";\n}\n");
	setAttr ".st" 3;
createNode polyCube -n "skinCubeAnimated_polyCube1";
	rename -uid "40621900-0000-7889-5A79-032800000287";
	setAttr ".h" 2;
	setAttr ".sh" 2;
	setAttr ".cuv" 4;
createNode skinCluster -n "skinCubeAnimated_skinCluster1";
	rename -uid "40621900-0000-7889-5A79-03280000028A";
	setAttr -s 12 ".wl";
	setAttr ".wl[0:11].w"
		1 0 1
		1 0 1
		2 0 0.5 1 0.5
		2 0 0.5 1 0.5
		1 1 1
		1 1 1
		1 1 1
		1 1 1
		2 0 0.5 1 0.5
		2 0 0.5 1 0.5
		1 0 1
		1 0 1;
	setAttr -s 3 ".pm";
	setAttr ".pm[0]" -type "matrix" 0 0 1 0 1 0 0 0
		 0 1 0 0 1 0 0 1;
	setAttr ".pm[1]" -type "matrix" 0 0 1 0 1 0 0 0
		 0 1 0 0 0 0 0 1;
	setAttr ".pm[2]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 -1 0 1;
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
createNode tweak -n "skinCubeAnimated_tweak1";
	rename -uid "40621900-0000-7889-5A79-03280000028B";
createNode objectSet -n "skinCubeAnimated_skinCluster1Set";
	rename -uid "40621900-0000-7889-5A79-03280000028C";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "skinCubeAnimated_skinCluster1GroupId";
	rename -uid "40621900-0000-7889-5A79-03280000028D";
	setAttr ".ihi" 0;
createNode groupParts -n "skinCubeAnimated_skinCluster1GroupParts";
	rename -uid "40621900-0000-7889-5A79-03280000028E";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "skinCubeAnimated_tweakSet1";
	rename -uid "40621900-0000-7889-5A79-03280000028F";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "skinCubeAnimated_groupId2";
	rename -uid "40621900-0000-7889-5A79-032800000290";
	setAttr ".ihi" 0;
createNode groupParts -n "skinCubeAnimated_groupParts2";
	rename -uid "40621900-0000-7889-5A79-032800000291";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode dagPose -n "skinCubeAnimated_bindPose1";
	rename -uid "40621900-0000-7889-5A79-032800000292";
	setAttr -s 3 ".wm";
	setAttr -s 3 ".xm";
	setAttr ".xm[0]" -type "matrix" "xform" 1 1 1 0 0 0 0 0
		 -1 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0.5 0.5 0.5 0.5 1
		 1 1 yes;
	setAttr ".xm[1]" -type "matrix" "xform" 1 1 1 0 0 0 0 1
		 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0 0 0 1 1
		 1 1 yes;
	setAttr ".xm[2]" -type "matrix" "xform" 1 1 1 0 0 0 0 1
		 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 -0.5 -0.5 -0.5 0.5 1
		 1 1 yes;
	setAttr -s 3 ".m";
	setAttr -s 3 ".p";
	setAttr ".bp" yes;
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
	rename -uid "DA02D900-0000-7B70-5A7B-527A000002A1";
	setAttr -s 222 ".wl";
	setAttr ".wl[0:124].w"
		4 0 5.943539431590859e-07 1 0.001203578151990906 2 0.49939791374703296 3 0.49939791374703296
		4 0 5.5288720865994063e-07 1 0.001122940303082332 2 0.49943825340485454 3 0.49943825340485454
		4 0 4.9126671659710998e-07 1 0.0010024262992175007 2 0.49949854121703297 3 0.49949854121703297
		4 0 4.1877071456136117e-07 1 0.00085952361464009842 2 0.49957002880732265 3 0.49957002880732265
		4 0 3.4513113676544971e-07 1 0.00071301393022601273 2 0.49964332046931864 3 0.49964332046931864
		4 0 2.7857295240537668e-07 1 0.00057928916307214279 2 0.49971021613198757 3 0.49971021613198779
		4 0 2.2461569276068424e-07 1 0.0004698613529872588 2 0.49976495701565998 3 0.49976495701565998
		4 0 1.8590954576286436e-07 1 0.00039073238014551091 2 0.49980454085515441 3 0.49980454085515441
		4 0 1.6298022469924866e-07 1 0.0003435819869678071 2 0.49982812751640382 3 0.49982812751640382
		4 0 1.5542765871855553e-07 1 0.00032800341163323184 2 0.49983592058035403 3 0.49983592058035403
		4 0 1.6298022469924866e-07 1 0.0003435819869678071 2 0.49982812751640382 3 0.49982812751640382
		4 0 1.8590945572801051e-07 1 0.00039073219619018457 2 0.49980454094717691 3 0.49980454094717713
		4 0 2.2461542968037463e-07 1 0.00046986081941158595 2 0.49976495728257936 3 0.49976495728257936
		4 0 2.7857259539342391e-07 1 0.00057928844559719994 2 0.49971021649090369 3 0.49971021649090369
		4 0 3.4513073069105222e-07 1 0.00071301312228157069 2 0.49964332087349395 3 0.49964332087349383
		4 0 4.1877007360676457e-07 1 0.00085952235208618642 2 0.49957002943892009 3 0.49957002943892009
		4 0 4.9126568854424493e-07 1 0.0010024242923774762 2 0.49949854222096696 3 0.49949854222096696
		4 0 5.5288604778922817e-07 1 0.0011229380530059824 2 0.49943825453047319 3 0.49943825453047308
		4 0 5.9435266971396483e-07 1 0.0012035756950457754 2 0.49939791497614228 3 0.49939791497614228
		4 0 6.0898569323695503e-07 1 0.00123194673808239 2 0.49938372213811222 3 0.49938372213811222
		4 0 1.0441048254751405e-06 1 0.49429909194808819 2 0.5044423039109549 3 0.0012575600361314
		4 0 9.7728835235946494e-07 1 0.49412011580825166 2 0.50469774691386071 3 0.0011811599895352267
		4 0 8.7757837816257844e-07 1 0.49380799717591495 2 0.50512473492742005 3 0.0010663903182868094
		4 0 7.5950918255550713e-07 1 0.49334740361227769 2 0.50572259792624386 3 0.00092923895229596082
		4 0 6.3851638048365827e-07 1 0.49272930029950318 2 0.50648289677500369 3 0.0007871644091125981
		4 0 5.2794248714525908e-07 1 0.4919667000495716 2 0.50737693919237914 3 0.00065583281556217982
		4 0 4.3714950624253674e-07 1 0.49111657226460953 2 0.50833617922008278 3 0.00054681136580127276
		4 0 3.7115381385270738e-07 1 0.49029914660015173 2 0.50923366017973348 3 0.00046682206630081394
		4 0 3.3161193329455387e-07 1 0.48969063800991292 2 0.50989046249367487 3 0.00041856788447893389
		4 0 3.1849983370273377e-07 1 0.48946323903034361 2 0.51013393334218016 3 0.00040250912764256791
		4 0 3.3161193329455387e-07 1 0.48969063800991292 2 0.50989046249367487 3 0.00041856788447893389
		4 0 3.7115363852121331e-07 1 0.49029914450829232 2 0.50923366248580459 3 0.00046682185226455648
		4 0 4.3714900225138101e-07 1 0.49111656767150119 2 0.50833618442398565 3 0.00054681075551086752
		4 0 5.279418158641694e-07 1 0.4919666955832469 2 0.50737694446398962 3 0.00065583201094758699
		4 0 6.3851563170357443e-07 1 0.49272929668637927 2 0.50648290127626616 3 0.00078716352172302384
		4 0 7.5950801849624131e-07 1 0.49334739947303075 2 0.50572260343075215 3 0.00092923758819864471
		4 0 8.7757653178026279e-07 1 0.49380799214200161 2 0.5051247421051106 3 0.0010663881763560068
		4 0 9.7728628537264861e-07 1 0.49412011121084448 2 0.50469775389202631 3 0.0011811576108438479
		4 0 1.0441025676296233e-06 1 0.49429908754538876 2 0.50444231090101244 3 0.0012575574510311548
		4 0 1.0676323621375294e-06 1 0.49435722232009022 2 0.50435734128091281 3 0.0012843687666348588
		4 0 4.6965236612002403e-06 1 0.99681011007040532 2 0.0030001871985557557 3 0.00018500620737767879
		4 0 4.3537610972405433e-06 1 0.99703410383429869 2 0.0027899269931155463 3 0.0001716154114885995
		4 0 3.8453504029129566e-06 1 0.99736824813574798 2 0.0024761780556585649 3 0.00015172845819059685
		4 0 3.2490116270279259e-06 1 0.99776328042683371 2 0.0021051084893242838 3 0.0001283620722147948
		4 0 2.6459245974211148e-06 1 0.99816651477985718 2 0.0017261558050954231 3 0.00010468349045006351
		4 0 2.1040439966867896e-06 1 0.99853240332047044 2 0.0013821299559763753 3 8.3362679556470007e-05
		4 0 1.6679351533025672e-06 1 0.9988296534625426 2 0.0011025099111158958 3 6.6168691188169126e-05
		4 0 1.3575611613298846e-06 1 0.99904291140659507 2 0.00090182033547998571 3 5.3910696763579048e-05
		4 0 1.1749984851933519e-06 1 0.99916908926012216 2 0.00078304433471046095 3 4.6691406682188824e-05
		4 0 1.1151237669431178e-06 1 0.99921060006451046 2 0.00074396267762868843 3 4.4322134093984743e-05
		4 0 1.1749984851933519e-06 1 0.99916908926012216 2 0.00078304433471046095 3 4.6691406682188824e-05
		4 0 1.3575604973911176e-06 1 0.99904291186134686 2 0.00090181990758904569 3 5.3910670566811026e-05
		4 0 1.6679331927971184e-06 1 0.99882965479518415 2 0.0011025086576618758 3 6.6168613961203676e-05
		4 0 2.1040413049876519e-06 1 0.99853240513297903 2 0.0013821282519719633 3 8.3362573744119779e-05
		4 0 2.6459214991875129e-06 1 0.99816651684497637 2 0.0017261538645982409 3 0.0001046833689261613
		4 0 3.2490066934323254e-06 1 0.99776328368186773 2 0.0021051054323057258 3 0.00012836187913311579
		4 0 3.8453424409414475e-06 1 0.9973682533405539 2 0.0024761731697850609 3 0.00015172814722011301
		4 0 4.3537520643770968e-06 1 0.99703410969674666 2 0.0027899214919392813 3 0.00017161505924971463
		4 0 4.6965137312792767e-06 1 0.996810116484796 2 0.003000181180913484 3 0.00018500582055927547
		4 0 4.8175754370692136e-06 1 0.9967312395763106 2 0.0030742105695208436 3 0.00018973227873148417
		4 0 1.2598145781464495e-05 1 0.99973154676759568 2 0.0002147455730464173 3 4.1109513576361657e-05
		4 0 1.1575514527474944e-05 1 0.99975326086042782 2 0.00019739278321961323 3 3.777084182519986e-05
		4 0 1.0067946272239257e-05 1 0.99978529099939262 2 0.0001717916804495771 3 3.284937388563187e-05
		4 0 8.3164627414506785e-06 1 0.99982253405483756 2 0.00014201716880841983 3 2.7132313612557094e-05
		4 0 6.5685989962430284e-06 1 0.99985973620171309 2 0.00011226734219237058 3 2.1427857098214576e-05
		4 0 5.0250590583245135e-06 1 0.99989262307094773 2 8.5960889620905788e-05 3 1.6390980373090429e-05
		4 0 3.8084959692554183e-06 1 0.99991856865016593 2 6.5201208462110409e-05 3 1.2421645402747237e-05
		4 0 2.9621196959245495e-06 1 0.99993663433866498 2 5.0743078564986106e-05 3 9.6604630740963404e-06
		4 0 2.4743981312193833e-06 1 0.99994705097256587 2 4.2405150896325266e-05 3 8.0694784066337647e-06
		4 0 2.3164435130098533e-06 1 0.99995042561122183 2 3.9703703271219108e-05 3 7.5542419939125109e-06
		4 0 2.4743981312193833e-06 1 0.99994705097256587 2 4.2405150896325266e-05 3 8.0694784066337647e-06
		4 0 2.9621182072233106e-06 1 0.99993663437035096 2 5.0743053214635835e-05 3 9.660458227128402e-06
		4 0 3.8084914426748067e-06 1 0.99991856874641238 2 6.5201131478959096e-05 3 1.2421630665990936e-05
		4 0 5.0250526409426059e-06 1 0.99989262320723193 2 8.5960780645637038e-05 3 1.6390959481525099e-05
		4 0 6.5685913665659589e-06 1 0.99985973636354664 2 0.00011226721282757891 3 2.1427832259238038e-05
		4 0 8.3164502932634407e-06 1 0.99982253431854706 2 0.00014201695807341568 3 2.7132273086222418e-05
		4 0 1.006792583168663e-05 1 0.99978529143192074 2 0.00017179133490784042 3 3.2849307339701909e-05
		4 0 1.1575491026840295e-05 1 0.9997532613572836 2 0.00019739238637480811 3 3.7770765314714263e-05
		4 0 1.259811978172259e-05 1 0.99973154731695979 2 0.00021474513432788607 3 4.1109428930599957e-05
		4 0 1.2960389406750121e-05 1 0.99972385745696424 2 0.00022088993913286329 3 4.229221449615541e-05
		4 0 4.3183004132706475e-05 1 0.99989562606251969 2 4.6765254399498013e-05 3 1.4425678948005051e-05
		4 0 3.9378178563567322e-05 1 0.99990484900150267 2 4.2625922064944362e-05 3 1.3146897868789908e-05
		4 0 3.3795499290582909e-05 1 0.99991837423276553 2 3.6557568539800737e-05 3 1.1272699403939274e-05
		4 0 2.7359005151360509e-05 1 0.99993395651263883 2 2.9569273550881688e-05 3 9.1152086587670648e-06
		4 0 2.1007395847468283e-05 1 0.9999493199433479 2 2.2682605413355906e-05 3 6.9900553913938719e-06
		4 0 1.5483950917614644e-05 1 0.99996266796922173 2 1.6702532040300515e-05 3 5.1455478203045951e-06
		4 0 1.1215620274740327e-05 1 0.99997297387142714 2 1.2087717077999137e-05 3 3.7227912202057063e-06
		4 0 8.3129595278256938e-06 1 0.99997997713679154 2 8.9531349176506689e-06 3 2.7567687629008636e-06
		4 0 6.6763276976971072e-06 1 0.99998392370025679 2 7.1872601561532474e-06 3 2.2127118893334924e-06
		4 0 6.1536065083455752e-06 1 0.99998518382495327 2 6.6235169990075951e-06 3 2.0390515394275574e-06
		4 0 6.6763276976971072e-06 1 0.99998392370025679 2 7.1872601561532474e-06 3 2.2127118893334924e-06
		4 0 8.3129552433164534e-06 1 0.99997997714711007 2 8.9531303058994651e-06 3 2.7567673408052422e-06
		4 0 1.1215606844839575e-05 1 0.99997297390378614 2 1.2087702611620783e-05 3 3.7227867574007329e-06
		4 0 1.5483931262553042e-05 1 0.99996266801661171 2 1.6702510846753429e-05 3 5.14554127884499e-06
		4 0 2.1007371747161907e-05 1 0.99994932000150316 2 2.2682579393659156e-05 3 6.990047356074086e-06
		4 0 2.7358964952833808e-05 1 0.99993395660971718 2 2.9569230097546503e-05 3 9.1151952324037586e-06
		4 0 3.3795432278316072e-05 1 0.99991837439471243 2 3.6557496022880196e-05 3 1.1272676986383629e-05
		4 0 3.937810063624569e-05 1 0.99990484918993872 2 4.2625837658953905e-05 3 1.3146871766126373e-05
		4 0 4.3182917470009973e-05 1 0.99989562627215245 2 4.6765160481313301e-05 3 1.4425649896295433e-05
		4 0 4.4533814900639171e-05 1 0.99989235076044103 2 4.8235477141613703e-05 3 1.4879947516715898e-05
		4 0 0.00023116215207028535 1 0.99974607279085104 2 1.6245524151258788e-05 3 6.5195329273758469e-06
		4 0 0.00020951574633406645 1 0.99976988454705529 2 1.4700559646480571e-05 3 5.8991469641326882e-06
		4 0 0.00017784656590916684 1 0.99980471183771258 2 1.2447189255116688e-05 3 4.9944071231849781e-06
		4 0 0.00014152056342857733 1 0.99984464481859325 2 9.8734207153488438e-06 3 3.9611972628415271e-06
		4 0 0.00010596894453031448 1 0.99988370870288568 2 7.3670946464326632e-06 3 2.9552579375324636e-06
		4 0 7.5436589134057132e-05 1 0.99991724138679572 2 5.2259566832795261e-06 3 2.0960673869388028e-06
		4 0 5.2251606899050199e-05 1 0.99994269309639117 2 3.608250901225417e-06 3 1.4470458086709293e-06
		4 0 3.6826545266437683e-05 1 0.99995961965249658 2 2.5366209714363511e-06 3 1.0171812655678847e-06
		4 0 2.8322633178584638e-05 1 0.99996894870777686 2 1.9476883087363702e-06 3 7.8097073593519071e-07
		4 0 2.5647327702747859e-05 1 0.99997188316302543 2 1.7627209068219232e-06 3 7.0678836510874811e-07
		4 0 2.8322633178584638e-05 1 0.99996894870777686 2 1.9476883087363702e-06 3 7.8097073593519071e-07
		4 0 3.6826525857898448e-05 1 0.99995961967378588 2 2.536619629248602e-06 3 1.0171807269941706e-06
		4 0 5.2251544157499644e-05 1 0.99994269316523399 2 3.6082465473127915e-06 3 1.4470440612438918e-06
		4 0 7.5436494462846265e-05 1 0.99991724149071515 2 5.2259500842735919e-06 3 2.0960647378220523e-06
		4 0 0.0001059688251179315 1 0.99988370883402344 2 7.367086280332681e-06 3 2.955254578231747e-06
		4 0 0.00014152036044291563 1 0.99984464504161286 2 9.8734064221225868e-06 3 3.9611915221799901e-06
		4 0 0.00017784622339052725 1 0.99980471221419176 2 1.2447165027364669e-05 3 4.994397390300701e-06
		4 0 0.00020951534443842452 1 0.99976988498894614 2 1.4700531114913208e-05 3 5.8991355003870513e-06
		4 0 0.00023116170346870423 1 0.99974607328419951 2 1.6245492231150121e-05 3 6.5195201006581553e-06
		4 0 0.00023885714927829505 1 0.99973760678213897 2 1.6795626238845168e-05 3 6.740442343963798e-06
		4 0 0.0034559597781165276 1 0.99653332170357489 2 7.2709016458824966e-06 3 3.4476166627289344e-06
		4 0 0.0031288416116318266 1 0.99686151858053462 2 6.5392257812469103e-06 3 3.1005820523560479e-06
		4 0 0.0026474529895991849 1 0.99734447455809627 2 5.476087270097816e-06 3 2.596365034424393e-06
		4 0 0.0020914248091641993 1 0.99790228150861504 2 4.2695135983814146e-06 3 2.0241686223843559e-06
		4 0 0.0015439965899094635 1 0.99845142485779947 2 3.1060713953186651e-06 3 1.4724808958052532e-06;
	setAttr ".wl[125:221].w"
		4 0 0.0010727312921566812 1 0.99892413438981897 2 2.126356029389576e-06 3 1.0079619949338971e-06
		4 0 0.00071621554279105383 1 0.9992817197768642 2 1.4007301254740707e-06 3 6.6395021929871534e-07
		4 0 0.00048166964668483209 1 0.99951695665264495 2 9.3196731083748163e-07 3 4.4173335930783955e-07
		4 0 0.00035443859624774139 1 0.99964455754142034 2 6.8106282238701322e-07 3 3.2279950963852168e-07
		4 0 0.00031492218190958557 1 0.99968418800902392 2 6.0368637487472463e-07 3 2.8612269167817946e-07
		4 0 0.00035443859624774139 1 0.99964455754142034 2 6.8106282238701322e-07 3 3.2279950963852168e-07
		4 0 0.00048166939076327417 1 0.99951695690931164 2 9.3196680543577474e-07 3 4.4173311967276255e-07
		4 0 0.0007162146964036219 1 0.99928172062575438 2 1.4007284278210816e-06 3 6.6394941427010081e-07
		4 0 0.0010727299912790893 1 0.99892413569461969 2 2.1263533683894305e-06 3 1.007960732921726e-06
		4 0 0.0015439949264155078 1 0.99845142652642105 2 3.1060679172740437e-06 3 1.4724792460931389e-06
		4 0 0.002091421970321796 1 0.99790228435640038 2 4.2695075328792983e-06 3 2.0241657450135464e-06
		4 0 0.0026474482063177656 1 0.99734447935674153 2 5.4760768500062152e-06 3 2.596360090725314e-06
		4 0 0.003128836009773078 1 0.99686152420066609 2 6.539213388547455e-06 3 3.1005761722624108e-06
		4 0 0.0034559535450965724 1 0.99653332795712624 2 7.2708877216611676e-06 3 3.4476100555496435e-06
		4 0 0.0035718639935467789 1 0.99641703272207727 2 7.5318809895556454e-06 3 3.5714033863539848e-06
		4 0 0.4913606222762732 1 0.50863637657036298 2 1.9588737599877461e-06 3 1.0422796038963692e-06
		4 0 0.49095800483147173 1 0.50903931022047766 2 1.7524950890572728e-06 3 9.3245296151063467e-07
		4 0 0.49022898032570528 1 0.50976879223878779 2 1.4538855116721404e-06 3 7.7354999530985436e-07
		4 0 0.48907625904099061 1 0.51092202931189734 2 1.1172349920412138e-06 3 5.9441211998309661e-07
		4 0 0.48734574138957032 1 0.51265303955970576 2 7.9571559554956121e-07 3 4.2333512848567904e-07
		4 0 0.48482587189165743 1 0.51517331836914748 2 5.2855118575692999e-07 3 2.8118800944707772e-07
		4 0 0.48131500452383358 1 0.51868448348328311 2 3.3420353858710409e-07 3 1.777893446334243e-07
		4 0 0.47690971084838135 1 0.52308996515660355 2 2.1148988400152082e-07 3 1.1250513106592661e-07
		4 0 0.47265349497961734 1 0.52734627918918797 2 1.4741370712446513e-07 3 7.8417487414555844e-08
		4 0 0.47076963014415252 1 0.52923017376553338 2 1.2800030275055642e-07 3 6.8090011385412772e-08
		4 0 0.47265349497961734 1 0.52734627918918797 2 1.4741370712446513e-07 3 7.8417487414555844e-08
		4 0 0.47690970445923686 1 0.52308997154592407 2 2.1148976912410905e-07 3 1.1250506994206947e-07
		4 0 0.48131499317309923 1 0.51868449483462864 2 3.3420313967507903e-07 3 1.7778913236599866e-07
		4 0 0.48482586252197984 1 0.51517332773981406 2 5.2855054024892843e-07 3 2.8118766593524279e-07
		4 0 0.48734573450746799 1 0.51265304644313914 2 7.9571472681271934e-07 3 4.2333466614757654e-07
		4 0 0.48907625161436996 1 0.5109220367408861 2 1.1172334465670967e-06 3 5.9441129742658536e-07
		4 0 0.49022897153907824 1 0.50976880102953781 2 1.4538828208568612e-06 3 7.7354856306314023e-07
		4 0 0.49095799682141195 1 0.50903931823549131 2 1.7524918561392726e-06 3 9.3245124063220666e-07
		4 0 0.49136061458762931 1 0.50863638426459901 2 1.9588701105315938e-06 3 1.0422776612249534e-06
		4 0 0.49148932086670793 1 0.5085075649605304 2 2.032638016558684e-06 3 1.0815347451218172e-06
		4 0 0.99641424240889886 1 0.0035833514791219646 2 1.5238862143273506e-06 3 8.82225764921278e-07
		4 0 0.99670082148442785 1 0.0032969814456627606 2 1.3914969197273706e-06 3 8.0557298971613525e-07
		4 0 0.99712781070774958 1 0.0028702991669166617 2 1.1971031532195315e-06 3 6.9302218043424326e-07
		4 0 0.99763076956514318 1 0.0023676946252066001 2 9.72706097975941e-07 3 5.6310355232295124e-07
		4 0 0.99814005504655023 1 0.0018587594222197797 2 7.5086329629395286e-07 3 4.3466793361121753e-07
		4 0 0.99859551918283984 1 0.0014036006844082443 2 5.574420037923187e-07 3 3.2269074812853031e-07
		4 0 0.99895744669726338 1 0.0010419099972110335 2 4.0744788262034825e-07 3 2.3585764296153528e-07
		4 0 0.9992098593122678 1 0.00078965911952409153 2 3.050107819543799e-07 3 1.7655742608205102e-07
		4 0 0.99935499902188696 1 0.00064461099039861555 2 2.4700740833048124e-07 3 1.4298030616691568e-07
		4 0 0.99940186468008407 1 0.00059777466361325205 2 2.2843000773218326e-07 3 1.3222629499538324e-07
		4 0 0.99935499902188696 1 0.00064461099039861555 2 2.4700740833048124e-07 3 1.4298030616691568e-07
		4 0 0.9992098597075153 1 0.00078965872452375083 2 3.050106254539025e-07 3 1.7655733547728846e-07
		4 0 0.99895744790836771 1 0.0010419087868769866 2 4.0744739487850883e-07 3 2.3585736057731537e-07
		4 0 0.9985955209072771 1 0.0014035989610914946 2 5.5744129412347234e-07 3 3.2269033724011631e-07
		4 0 0.99814005709826092 1 0.0018587573718754273 2 7.5086243097356534e-07 3 4.3466743258239097e-07
		4 0 0.99763077289300972 1 0.0023676912996102883 2 9.7270466016897861e-07 3 5.6310271978400636e-07
		4 0 0.99712781612382484 1 0.0028702937546167457 2 1.1971007624014754e-06 3 6.9302079601630138e-07
		4 0 0.99670082766312129 1 0.0032969752713508192 2 1.391494144879039e-06 3 8.0557138287703318e-07
		4 0 0.99641424920120036 1 0.0035833446916891448 2 1.5238831309703657e-06 3 8.8222397939731155e-07
		4 0 0.99631331058415495 1 0.0036842091143805635 2 1.5708712811004905e-06 3 9.0943018339617217e-07
		4 0 0.99983295702475949 1 0.00016601578653058541 2 6.3505337370772887e-07 3 3.9213533617088782e-07
		4 0 0.99984236692062367 1 0.00015666558382416662 2 5.9814991652672474e-07 3 3.6934563572863379e-07
		4 0 0.99985652303915307 1 0.0001425989410297437 2 5.428340333412788e-07 3 3.3518578381518549e-07
		4 0 0.99987348313753677 1 0.00012574550541251907 2 4.768923125659007e-07 3 2.9446473813961293e-07
		4 0 0.99989112294261473 1 0.00010821598459556282 2 4.0871116131520879e-07 3 2.5236162838322941e-07
		4 0 0.99990752251464243 1 9.1918306421759274e-05 2 3.4571670714604343e-07 3 2.1346222868989316e-07
		4 0 0.99992123549576839 1 7.8290022170232919e-05 2 2.9335363981732701e-07 3 1.8112842165612149e-07
		4 0 0.99993137753941375 1 6.8210303111017459e-05 2 2.5482178046889298e-07 3 1.5733569485193587e-07
		4 0 0.99993753964753151 1 6.2085922594492935e-05 2 2.314967701968321e-07 3 1.4293310378186748e-07
		4 0 0.99993959919028264 1 6.0038964915217731e-05 2 2.2371605635190834e-07 3 1.3812874587368207e-07
		4 0 0.99993753964753151 1 6.2085922594492935e-05 2 2.314967701968321e-07 3 1.4293310378186748e-07
		4 0 0.99993137757137707 1 6.8210271340867268e-05 2 2.5482166101393474e-07 3 1.573356210880729e-07
		4 0 0.9999212355859598 1 7.8289932526048021e-05 2 2.9335330147427728e-07 3 1.8112821272447247e-07
		4 0 0.99990752263212468 1 9.191818965579798e-05 2 3.4571626425542872e-07 3 2.1346195519281554e-07
		4 0 0.999891123070484 1 0.00010821585751057273 2 4.0871067650018865e-07 3 2.5236132898989084e-07
		4 0 0.99987348333230386 1 0.00012574531184657064 2 4.7689156997363097e-07 3 2.9446427954757198e-07
		4 0 0.99985652334319675 1 0.00014259863887059935 2 5.4283286835234805e-07 3 3.3518506435457489e-07
		4 0 0.99984236725667575 1 0.00015666524986330423 2 5.9814862366321677e-07 3 3.6934483728367972e-07
		4 0 0.99983295738939637 1 0.00016601542416853516 2 6.3505196740752703e-07 3 3.921344676604172e-07
		4 0 0.99982965723518191 1 0.00016929460333320339 2 6.4801911368622053e-07 3 4.0014237113327582e-07
		4 0 0.99996322997559572 1 3.604997671368927e-05 2 4.3647142792629409e-07 3 2.8357626264570782e-07
		4 0 0.99996379643358879 1 3.5495184814915113e-05 2 4.2940049531348177e-07 3 2.7898110094428821e-07
		4 0 0.99996467090837049 1 3.4638691772853068e-05 2 4.1850155462528424e-07 3 2.7189830198266327e-07
		4 0 0.99996575920678732 1 3.3572724192800179e-05 2 4.0496651706993805e-07 3 2.6310250274519178e-07
		4 0 0.99996694776378126 1 3.2408494039695091e-05 2 3.9022159793310275e-07 3 2.5352058111910305e-07
		4 0 0.99996811746359637 1 3.1262671854934479e-05 2 3.7574887539251148e-07 3 2.4411567337051314e-07
		4 0 0.9999691564151364 1 3.0244875539306634e-05 2 3.6292619349938452e-07 3 2.3578313076431119e-07
		4 0 0.99996996990084897 1 2.944791858637692e-05 2 3.5290767791295153e-07 3 2.2927288681111565e-07
		4 0 0.99997048702147739 1 2.8941288658528012e-05 2 3.465489684881848e-07 3 2.251408956566416e-07
		4 0 0.99997066427131331 1 2.8767631695412922e-05 2 3.4437122333221039e-07 3 2.2372576796507755e-07
		4 0 0.99997048702147739 1 2.8941288658528012e-05 2 3.465489684881848e-07 3 2.251408956566416e-07
		4 0 0.99996996991096354 1 2.9447908668522537e-05 2 3.5290755861633186e-07 3 2.2927280929921265e-07
		4 0 0.99996915644171547 1 3.0244849478124498e-05 2 3.6292587951901982e-07 3 2.3578292675659806e-07
		4 0 0.99996811749530146 1 3.126264076898701e-05 2 3.7574850008023151e-07 3 2.4411542950980873e-07
		4 0 0.99996694779501671 1 3.2408463415737498e-05 2 3.9022122725821335e-07 3 2.5352034026838766e-07
		4 0 0.99996575925055653 1 3.3572681282649974e-05 2 4.0496599643201225e-07 3 2.6310216444975759e-07
		4 0 0.99996467097239938 1 3.4638629003149252e-05 2 4.1850079143722327e-07 3 2.718978060795666e-07
		4 0 0.99996379650074552 1 3.5495118981059527e-05 2 4.2939969347650327e-07 3 2.789805799231538e-07
		4 0 0.99996323004654131 1 3.6049907167006431e-05 2 4.3647058001380905e-07 3 2.8357571168233312e-07
		4 0 0.99996303395445463 1 3.624195748254363e-05 2 4.3892033254575069e-07 3 2.8516773029850847e-07
		4 0 2.706221164002425e-10 1 6.0154731422958879e-07 2 0.49999969909103181 3 0.49999969909103181
		4 0 0.99999858940711783 1 1.3833380905567139e-06 2 1.6522476434351603e-08 3 1.073231523035616e-08;
	setAttr -s 4 ".pm";
	setAttr ".pm[0]" -type "matrix" -0.020929209753067227 -0.99978096010031736 2.5630889732777469e-18 0 -0.99978096010031736 0.020929209753067116 1.2243785526353214e-16 0
		 -1.2246467991473535e-16 -1.2325951644078312e-32 -1 0 4.8972873510666233 -0.1101654861256416 -5.9974472789885606e-16 1;
	setAttr ".pm[1]" -type "matrix" 0.0062144400276027394 0.99998069018113722 1.1989922617067134e-16 0 -0.99998069018113722 0.0062144400276028505 -1.2167680585470244e-16 0
		 -1.2241956284801519e-16 -1.1914075772559789e-16 1 0 2.0337540129675316 0.054985827007361321 2.5552233030170733e-16 1;
	setAttr ".pm[2]" -type "matrix" -0.0035334455848444338 -0.99999375736166451 -2.4236314158270109e-16 0 -0.99999375736166451 0.0035334455848443228 1.2210952813724651e-16 0
		 -1.2125238887907062e-16 2.4279309597032631e-16 -1 0 -3.9761591306332975 -0.016227879170551907 4.7817877612776545e-16 1;
	setAttr ".pm[3]" -type "matrix" 1 6.8955258170078119e-17 1.1989846166796572e-16 0 -6.8955258170078082e-17 1 -1.2210952813724649e-16 0
		 -1.1989846166796579e-16 1.2210952813724654e-16 1 0 0.033889784459489349 4.9984337676028847 -6.0629305576431607e-16 1;
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
	rename -uid "DA02D900-0000-7B70-5A7B-527A000002A3";
createNode objectSet -n "skinCluster1Set";
	rename -uid "DA02D900-0000-7B70-5A7B-527A000002A4";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "skinCluster1GroupId";
	rename -uid "DA02D900-0000-7B70-5A7B-527A000002A5";
	setAttr ".ihi" 0;
createNode groupParts -n "skinCluster1GroupParts";
	rename -uid "DA02D900-0000-7B70-5A7B-527A000002A6";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode objectSet -n "tweakSet1";
	rename -uid "DA02D900-0000-7B70-5A7B-527A000002A7";
	setAttr ".ihi" 0;
	setAttr ".vo" yes;
createNode groupId -n "groupId2";
	rename -uid "DA02D900-0000-7B70-5A7B-527A000002A8";
	setAttr ".ihi" 0;
createNode groupParts -n "groupParts2";
	rename -uid "DA02D900-0000-7B70-5A7B-527A000002A9";
	setAttr ".ihi" 0;
	setAttr ".ic" -type "componentList" 1 "vtx[*]";
createNode dagPose -n "bindPose1";
	rename -uid "DA02D900-0000-7B70-5A7B-527A000002AA";
	setAttr -s 6 ".wm";
	setAttr ".wm[0]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 0 1;
	setAttr ".wm[1]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 0 1;
	setAttr -s 6 ".xm";
	setAttr ".xm[0]" -type "matrix" "xform" 1 1 1 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0 0 0 1 1
		 1 1 yes;
	setAttr ".xm[1]" -type "matrix" "xform" 1 1 1 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0 0 0 1 1
		 1 1 yes;
	setAttr ".xm[2]" -type "matrix" "xform" 1 1 1 0 0 0 0 -0.0076450012970963321
		 4.8985203263032 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 -0.69966806067124887 0.71446805728215279 4.284231254833405e-17 4.3748550972180813e-17 1
		 1 1 yes;
	setAttr ".xm[3]" -type "matrix" "xform" 1 1 1 0 0 0 0 2.8657750099522699
		 -2.5673907444456745e-16 -3.5095621930145226e-16 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0.99990789355626142 -0.013572192301916895 -8.3105709299774366e-19 6.1226700064292399e-17 1
		 1 1 yes;
	setAttr ".xm[4]" -type "matrix" "xform" 1 1 1 0 0 0 0 6.009566047635321
		 -8.7985174701543656e-15 7.3568844845778947e-16 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 0.99998812224887645 -0.0048739471854073648 -2.9844319099111905e-19 6.1231612654872936e-17 1
		 1 1 yes;
	setAttr ".xm[5]" -type "matrix" "xform" 1 1 1 0 0 0 0 1.0223631812646017
		 -3.0453070620772849e-15 -1.2396397803033984e-16 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 1 -0.70585641401603616 0.7083549412493868 4.3221239904118383e-17 4.3374230573063646e-17 1
		 1 1 yes;
	setAttr -s 6 ".m";
	setAttr -s 6 ".p";
	setAttr -s 6 ".g[0:5]" yes yes no no no no;
	setAttr ".bp" yes;
select -ne :time1;
	setAttr ".o" 1;
	setAttr ".unw" 1;
select -ne :hardwareRenderingGlobals;
	setAttr ".otfna" -type "stringArray" 22 "NURBS Curves" "NURBS Surfaces" "Polygons" "Subdiv Surface" "Particles" "Particle Instance" "Fluids" "Strokes" "Image Planes" "UI" "Lights" "Cameras" "Locators" "Joints" "IK Handles" "Deformers" "Motion Trails" "Components" "Hair Systems" "Follicles" "Misc. UI" "Ornaments"  ;
	setAttr ".otfva" -type "Int32Array" 22 0 1 1 1 1 1
		 1 1 1 0 0 0 0 0 0 0 0 0
		 0 0 0 0 ;
	setAttr ".etmr" no;
	setAttr ".tmr" 4096;
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
connectAttr "skinCluster1GroupId.id" "meshShape.iog.og[0].gid";
connectAttr "skinCluster1Set.mwc" "meshShape.iog.og[0].gco";
connectAttr "groupId2.id" "meshShape.iog.og[1].gid";
connectAttr "tweakSet1.mwc" "meshShape.iog.og[1].gco";
connectAttr "skinCluster1.og[0]" "meshShape.i";
connectAttr "tweak1.vl[0].vt[0]" "meshShape.twl";
connectAttr "polyCylinder1.out" "meshShapeOrig.i";
connectAttr "joint4.s" "joint5.is";
connectAttr "joint5.s" "joint6.is";
connectAttr "joint6.s" "joint7.is";
connectAttr "skinCubeAnimated_skinCluster1GroupId.id" "animatedCubeShape.iog.og[0].gid"
		;
connectAttr "skinCubeAnimated_skinCluster1Set.mwc" "animatedCubeShape.iog.og[0].gco"
		;
connectAttr "skinCubeAnimated_groupId2.id" "animatedCubeShape.iog.og[1].gid";
connectAttr "skinCubeAnimated_tweakSet1.mwc" "animatedCubeShape.iog.og[1].gco";
connectAttr "skinCubeAnimated_skinCluster1.og[0]" "animatedCubeShape.i";
connectAttr "skinCubeAnimated_tweak1.vl[0].vt[0]" "animatedCubeShape.twl";
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
connectAttr "skinCubeAnimated_skinCluster1GroupParts.og" "skinCubeAnimated_skinCluster1.ip[0].ig"
		;
connectAttr "skinCubeAnimated_skinCluster1GroupId.id" "skinCubeAnimated_skinCluster1.ip[0].gi"
		;
connectAttr "skinCubeAnimated_bindPose1.msg" "skinCubeAnimated_skinCluster1.bp";
connectAttr "joint1.wm" "skinCubeAnimated_skinCluster1.ma[0]";
connectAttr "joint2.wm" "skinCubeAnimated_skinCluster1.ma[1]";
connectAttr "joint3.wm" "skinCubeAnimated_skinCluster1.ma[2]";
connectAttr "joint1.liw" "skinCubeAnimated_skinCluster1.lw[0]";
connectAttr "joint2.liw" "skinCubeAnimated_skinCluster1.lw[1]";
connectAttr "joint3.liw" "skinCubeAnimated_skinCluster1.lw[2]";
connectAttr "joint1.obcc" "skinCubeAnimated_skinCluster1.ifcl[0]";
connectAttr "joint2.obcc" "skinCubeAnimated_skinCluster1.ifcl[1]";
connectAttr "joint3.obcc" "skinCubeAnimated_skinCluster1.ifcl[2]";
connectAttr "joint1.msg" "skinCubeAnimated_skinCluster1.ptt";
connectAttr "skinCubeAnimated_groupParts2.og" "skinCubeAnimated_tweak1.ip[0].ig"
		;
connectAttr "skinCubeAnimated_groupId2.id" "skinCubeAnimated_tweak1.ip[0].gi";
connectAttr "skinCubeAnimated_skinCluster1GroupId.msg" "skinCubeAnimated_skinCluster1Set.gn"
		 -na;
connectAttr "animatedCubeShape.iog.og[0]" "skinCubeAnimated_skinCluster1Set.dsm"
		 -na;
connectAttr "skinCubeAnimated_skinCluster1.msg" "skinCubeAnimated_skinCluster1Set.ub[0]"
		;
connectAttr "skinCubeAnimated_tweak1.og[0]" "skinCubeAnimated_skinCluster1GroupParts.ig"
		;
connectAttr "skinCubeAnimated_skinCluster1GroupId.id" "skinCubeAnimated_skinCluster1GroupParts.gi"
		;
connectAttr "skinCubeAnimated_groupId2.msg" "skinCubeAnimated_tweakSet1.gn" -na;
connectAttr "animatedCubeShape.iog.og[1]" "skinCubeAnimated_tweakSet1.dsm" -na;
connectAttr "skinCubeAnimated_tweak1.msg" "skinCubeAnimated_tweakSet1.ub[0]";
connectAttr "animatedCubeShape1Orig.w" "skinCubeAnimated_groupParts2.ig";
connectAttr "skinCubeAnimated_groupId2.id" "skinCubeAnimated_groupParts2.gi";
connectAttr "joint1.msg" "skinCubeAnimated_bindPose1.m[0]";
connectAttr "joint2.msg" "skinCubeAnimated_bindPose1.m[1]";
connectAttr "joint3.msg" "skinCubeAnimated_bindPose1.m[2]";
connectAttr "skinCubeAnimated_bindPose1.w" "skinCubeAnimated_bindPose1.p[0]";
connectAttr "skinCubeAnimated_bindPose1.m[0]" "skinCubeAnimated_bindPose1.p[1]";
connectAttr "skinCubeAnimated_bindPose1.m[1]" "skinCubeAnimated_bindPose1.p[2]";
connectAttr "joint1.bps" "skinCubeAnimated_bindPose1.wm[0]";
connectAttr "joint2.bps" "skinCubeAnimated_bindPose1.wm[1]";
connectAttr "joint3.bps" "skinCubeAnimated_bindPose1.wm[2]";
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
connectAttr "meshShape.iog.og[0]" "skinCluster1Set.dsm" -na;
connectAttr "skinCluster1.msg" "skinCluster1Set.ub[0]";
connectAttr "tweak1.og[0]" "skinCluster1GroupParts.ig";
connectAttr "skinCluster1GroupId.id" "skinCluster1GroupParts.gi";
connectAttr "groupId2.msg" "tweakSet1.gn" -na;
connectAttr "meshShape.iog.og[1]" "tweakSet1.dsm" -na;
connectAttr "tweak1.msg" "tweakSet1.ub[0]";
connectAttr "meshShapeOrig.w" "groupParts2.ig";
connectAttr "groupId2.id" "groupParts2.gi";
connectAttr "UsdExportMeshTest.msg" "bindPose1.m[0]";
connectAttr "ExplicitSkelRoot.msg" "bindPose1.m[1]";
connectAttr "joint4.msg" "bindPose1.m[2]";
connectAttr "joint5.msg" "bindPose1.m[3]";
connectAttr "joint6.msg" "bindPose1.m[4]";
connectAttr "joint7.msg" "bindPose1.m[5]";
connectAttr "bindPose1.w" "bindPose1.p[0]";
connectAttr "bindPose1.m[0]" "bindPose1.p[1]";
connectAttr "bindPose1.m[1]" "bindPose1.p[2]";
connectAttr "bindPose1.m[2]" "bindPose1.p[3]";
connectAttr "bindPose1.m[3]" "bindPose1.p[4]";
connectAttr "bindPose1.m[4]" "bindPose1.p[5]";
connectAttr "joint4.bps" "bindPose1.wm[2]";
connectAttr "joint5.bps" "bindPose1.wm[3]";
connectAttr "joint6.bps" "bindPose1.wm[4]";
connectAttr "joint7.bps" "bindPose1.wm[5]";
connectAttr "lambert1SG.pa" ":renderPartition.st" -na;
connectAttr "defaultRenderLayer1.msg" ":defaultRenderingList1.r" -na;
connectAttr "animatedCubeShape.iog" ":initialShadingGroup.dsm" -na;
connectAttr "meshShape.iog" ":initialShadingGroup.dsm" -na;
// End of UsdExportMeshTest.ma
