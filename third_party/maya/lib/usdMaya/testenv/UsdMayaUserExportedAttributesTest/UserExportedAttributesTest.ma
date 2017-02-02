//Maya ASCII 2016 scene
//Name: UserExportedAttributesTest.ma
//Last modified: Wed, Dec 07, 2016 01:06:04 PM
//Codeset: UTF-8
requires maya "2016";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2016";
fileInfo "version" "2016";
fileInfo "cutIdentifier" "201610262200-1005964";
fileInfo "osv" "Linux 3.10.0-327.4.4.el7.x86_64 #1 SMP Thu Dec 17 15:51:24 EST 2015 x86_64";
createNode transform -s -n "persp";
	rename -uid "6D105860-0000-7362-5717-ED8500000223";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 34.420093417711293 -22.186561120823676 19.72342958889244 ;
	setAttr ".r" -type "double3" 64.282924915356716 0 57.194913994746621 ;
createNode camera -s -n "perspShape" -p "persp";
	rename -uid "6D105860-0000-7362-5717-ED8500000224";
	setAttr -k off ".v" no;
	setAttr ".fl" 34.999999999999993;
	setAttr ".coi" 45.453272709454041;
	setAttr ".imn" -type "string" "persp";
	setAttr ".den" -type "string" "persp_depth";
	setAttr ".man" -type "string" "persp_mask";
	setAttr ".hc" -type "string" "viewSet -p %camera";
createNode transform -s -n "top";
	rename -uid "6D105860-0000-7362-5717-ED8500000225";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 0 100.1 ;
createNode camera -s -n "topShape" -p "top";
	rename -uid "6D105860-0000-7362-5717-ED8500000226";
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
	rename -uid "6D105860-0000-7362-5717-ED8500000227";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 -100.1 0 ;
	setAttr ".r" -type "double3" 89.999999999999986 0 0 ;
createNode camera -s -n "frontShape" -p "front";
	rename -uid "6D105860-0000-7362-5717-ED8500000228";
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
	rename -uid "6D105860-0000-7362-5717-ED8500000229";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 100.1 0 0 ;
	setAttr ".r" -type "double3" 90 4.7708320221952805e-14 89.999999999999986 ;
createNode camera -s -n "sideShape" -p "side";
	rename -uid "6D105860-0000-7362-5717-ED850000022A";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 100.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "side";
	setAttr ".den" -type "string" "side_depth";
	setAttr ".man" -type "string" "side_mask";
	setAttr ".hc" -type "string" "viewSet -s %camera";
	setAttr ".o" yes;
createNode transform -n "UserExportedAttributesTest";
	rename -uid "6D105860-0000-7362-5717-EDC100000248";
createNode transform -n "Geom" -p "UserExportedAttributesTest";
	rename -uid "6D105860-0000-7362-5717-EDCA00000249";
createNode transform -n "Cube" -p "Geom";
	rename -uid "6D105860-0000-7362-5717-ED9700000247";
	addAttr -ci true -sn "USD_UserExportedAttributesJson" -ln "USD_UserExportedAttributesJson" 
		-dt "string";
	addAttr -ci true -sn "multipleTagSourceAttrA" -ln "multipleTagSourceAttrA" -dv 10 
		-min 0 -max 100 -at "long";
	addAttr -ci true -sn "multipleTagSourceAttrB" -ln "multipleTagSourceAttrB" -dv 10 
		-min 0 -max 100 -at "long";
	addAttr -ci true -sn "multipleTagSourceAttrC" -ln "multipleTagSourceAttrC" -dv 10 
		-min 0 -max 100 -at "long";
	addAttr -ci true -sn "multiplyTaggedAttr" -ln "multiplyTaggedAttr" -dv 10 -min 0 
		-max 100 -at "long";
	addAttr -ci true -sn "realAttrOne" -ln "realAttrOne" -dv 10 -min 0 -max 100 -at "long";
	addAttr -ci true -sn "realAttrTwo" -ln "realAttrTwo" -dv 2 -min 0 -max 10 -at "double";
	addAttr -ci true -sn "remapAttr" -ln "remapAttr" -dt "string";
	addAttr -ci true -sn "transformAndShapeAttr" -ln "transformAndShapeAttr" -dt "string";
	setAttr ".rp" -type "double3" 0 0 2.5 ;
	setAttr ".sp" -type "double3" 0 0 2.5 ;
	setAttr -k on ".USD_UserExportedAttributesJson" -type "string" "{\n    \"realAttrOne\": {},\n    \"realAttrTwo\": {\n        \"usdAttrName\": \"my:namespace:realAttrTwo\"\n    },\n    \"remapAttr\": {\n        \"usdAttrName\": \"my:namespace:someNewAttr\"\n    },\n    \"bogusAttrOne\": {},\n    \"bogusAttrTwo\": {\n        \"usdAttrName\": \"my:namespace:bogusAttrTwo\"\n    },\n    \"bogusRemapAttr\": {\n        \"usdAttrName\": \"my:namespace:someNewBogusAttr\"\n    },\n    \"transformAndShapeAttr\": {},\n    \"multiplyTaggedAttr\": {},\n    \"multipleTagSourceAttrC\": {\n        \"usdAttrName\": \"userProperties:multiplyTaggedAttr\"\n    },\n    \"multipleTagSourceAttrB\": {\n        \"usdAttrName\": \"userProperties:multiplyTaggedAttr\"\n    },\n    \"multipleTagSourceAttrA\": {\n        \"usdAttrName\": \"userProperties:multiplyTaggedAttr\"\n    }\n}";
	setAttr -k on ".multipleTagSourceAttrA" 20;
	setAttr -k on ".multipleTagSourceAttrB";
	setAttr -k on ".multipleTagSourceAttrC" 30;
	setAttr -k on ".multiplyTaggedAttr" 40;
	setAttr -k on ".realAttrOne" 42;
	setAttr -k on ".realAttrTwo" 3.14;
	setAttr -k on ".remapAttr" -type "string" "a string value";
	setAttr -k on ".transformAndShapeAttr" -type "string" "this node is a transform";
createNode mesh -n "CubeShape" -p "Cube";
	rename -uid "6D105860-0000-7362-5717-ED9700000246";
	addAttr -ci true -sn "USD_UserExportedAttributesJson" -ln "USD_UserExportedAttributesJson" 
		-dt "string";
	addAttr -ci true -sn "transformAndShapeAttr" -ln "transformAndShapeAttr" -dt "string";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
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
	setAttr -s 8 ".pt[0:7]" -type "float3"  -2 -2 4.5 2 
		-2 4.5 -2 2 4.5 2 2 4.5 -2 2 0.5 2 
		2 0.5 -2 -2 0.5 2 -2 0.5;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
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
	setAttr -k on ".USD_UserExportedAttributesJson" -type "string" "{\n    \"transformAndShapeAttr\": {}\n}";
	setAttr -k on ".transformAndShapeAttr" -type "string" "this node is a mesh";
	setAttr ".mgi" -type "string" "ID_ad9cf308-26d0-4e51-9883-5e8beeb09da7";
createNode transform -n "CubeTypedAttrs" -p "Geom";
	rename -uid "964758C0-0000-259C-57E5-BAB80000025A";
	addAttr -ci true -sn "myConstantIntPrimvar" -ln "myConstantIntPrimvar" -at "long";
	addAttr -ci true -sn "myUniformDoublePrimvar" -ln "myUniformDoublePrimvar" -at "double";
	addAttr -ci true -sn "myVertexStringPrimvar" -ln "myVertexStringPrimvar" -dt "string";
	addAttr -ci true -sn "myStringPrimvar" -ln "myStringPrimvar" -dt "string";
	addAttr -ci true -sn "myIntRiAttr" -ln "myIntRiAttr" -at "long";
	addAttr -ci true -sn "myNamespacedStringRiAttr" -ln "myNamespacedStringRiAttr" -dt "string";
	addAttr -ci true -sn "myFaceVaryingIntPrimvar" -ln "myFaceVaryingIntPrimvar" -at "long";
	addAttr -ci true -sn "myIntArrayAttr" -ln "myIntArrayAttr" -dt "Int32Array";
	addAttr -ci true -sn "myFloatArrayPrimvar" -ln "myFloatArrayPrimvar" -dt "floatArray";
	addAttr -ci true -sn "myStringArrayRiAttr" -ln "myStringArrayRiAttr" -dt "stringArray";
	addAttr -ci true -sn "USD_UserExportedAttributesJson" -ln "USD_UserExportedAttributesJson" 
		-dt "string";
	setAttr ".t" -type "double3" 10 0 0 ;
	setAttr ".rp" -type "double3" 0 0 2.5 ;
	setAttr ".sp" -type "double3" 0 0 2.5 ;
	setAttr -k on ".myConstantIntPrimvar" 123;
	setAttr -k on ".myUniformDoublePrimvar" 3.14;
	setAttr -k on ".myVertexStringPrimvar" -type "string" "a vertex string";
	setAttr -k on ".myStringPrimvar" -type "string" "no interp string";
	setAttr -k on ".myIntRiAttr" 42;
	setAttr -k on ".myNamespacedStringRiAttr" -type "string" "UsdRi string";
	setAttr -k on ".myFaceVaryingIntPrimvar" 999;
	setAttr ".myIntArrayAttr" -type "Int32Array" 10 99 98 97 96 95
		 94 93 92 91 90 ;
	setAttr ".myFloatArrayPrimvar" -type "floatArray" 8 1.1 2.2 3.3 4.4000000953674316
		 5.5 6.5999999046325684 7.6999998092651367 8.8000001907348633 ;
	setAttr ".myStringArrayRiAttr" -type "stringArray" 4 "the" "quick" "brown" "fox"  ;
	setAttr ".USD_UserExportedAttributesJson" -type "string" "{\"myUniformDoublePrimvar\": {\"usdAttrType\": \"primvar\", \"interpolation\": \"uniform\"}, \"myStringPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myIntRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myStringArrayRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myIntArrayAttr\": {}, \"myConstantIntPrimvar\": {\"usdAttrType\": \"primvar\", \"interpolation\": \"constant\"}, \"myFloatArrayPrimvar\": {\"usdAttrType\": \"primvar\", \"interpolation\": \"vertex\"}, \"myFaceVaryingIntPrimvar\": {\"usdAttrType\": \"primvar\", \"interpolation\": \"faceVarying\"}, \"myVertexStringPrimvar\": {\"usdAttrType\": \"primvar\", \"interpolation\": \"vertex\"}, \"myNamespacedStringRiAttr\": {\"usdAttrType\": \"usdRi\", \"usdAttrName\": \"myNamespace:myAttr\"}}";
createNode mesh -n "CubeTypedAttrsShape" -p "CubeTypedAttrs";
	rename -uid "964758C0-0000-259C-57E5-BAB80000025B";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
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
	setAttr -s 8 ".pt[0:7]" -type "float3"  -2 -2 4.5 2 
		-2 4.5 -2 2 4.5 2 2 4.5 -2 2 0.5 2 
		2 0.5 -2 -2 0.5 2 -2 0.5;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
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
	setAttr ".mgi" -type "string" "ID_18dd217b-03dc-462d-a3db-9e8bff52bb6f";
createNode transform -n "AllMayaTypesTestingCubes" -p "Geom";
	rename -uid "513E48C0-0000-1298-5847-61D900000260";
createNode transform -n "AllTypesCube" -p "AllMayaTypesTestingCubes";
	rename -uid "1A9428C0-0000-0FD8-5846-1F700000025E";
	addAttr -ci true -sn "myBoolUsdAttr" -ln "myBoolUsdAttr" -min 0 -max 1 -at "bool";
	addAttr -ci true -sn "myBoolPrimvar" -ln "myBoolPrimvar" -min 0 -max 1 -at "bool";
	addAttr -ci true -sn "myBoolUsdRiAttr" -ln "myBoolUsdRiAttr" -min 0 -max 1 -at "bool";
	addAttr -ci true -sn "myLongUsdAttr" -ln "myLongUsdAttr" -at "long";
	addAttr -ci true -sn "myLongPrimvar" -ln "myLongPrimvar" -at "long";
	addAttr -ci true -sn "myLongUsdRiAttr" -ln "myLongUsdRiAttr" -at "long";
	addAttr -ci true -sn "myShortUsdAttr" -ln "myShortUsdAttr" -at "short";
	addAttr -ci true -sn "myShortPrimvar" -ln "myShortPrimvar" -at "short";
	addAttr -ci true -sn "myShortUsdRiAttr" -ln "myShortUsdRiAttr" -at "short";
	addAttr -ci true -sn "myByteUsdAttr" -ln "myByteUsdAttr" -min 0 -max 255 -at "byte";
	addAttr -ci true -sn "myBytePrimvar" -ln "myBytePrimvar" -min 0 -max 255 -at "byte";
	addAttr -ci true -sn "myByteUsdRiAttr" -ln "myByteUsdRiAttr" -min 0 -max 255 -at "byte";
	addAttr -ci true -sn "myCharUsdAttr" -ln "myCharUsdAttr" -min 0 -max 255 -at "char";
	addAttr -ci true -sn "myCharPrimvar" -ln "myCharPrimvar" -min 0 -max 255 -at "char";
	addAttr -ci true -sn "myCharUsdRiAttr" -ln "myCharUsdRiAttr" -min 0 -max 255 -at "char";
	addAttr -ci true -sn "myEnumUsdAttr" -ln "myEnumUsdAttr" -dv 1 -min 1 -max 3 -en 
		"One=1:Two:Three" -at "enum";
	addAttr -ci true -sn "myEnumPrimvar" -ln "myEnumPrimvar" -dv 1 -min 1 -max 3 -en 
		"One=1:Two:Three" -at "enum";
	addAttr -ci true -sn "myEnumUsdRiAttr" -ln "myEnumUsdRiAttr" -dv 1 -min 1 -max 3 
		-en "One=1:Two:Three" -at "enum";
	addAttr -ci true -sn "myFloatUsdAttr" -ln "myFloatUsdAttr" -at "float";
	addAttr -ci true -sn "myFloatPrimvar" -ln "myFloatPrimvar" -at "float";
	addAttr -ci true -sn "myFloatUsdRiAttr" -ln "myFloatUsdRiAttr" -at "float";
	addAttr -ci true -sn "myDoubleUsdAttr" -ln "myDoubleUsdAttr" -at "double";
	addAttr -ci true -sn "myDoublePrimvar" -ln "myDoublePrimvar" -at "double";
	addAttr -ci true -sn "myDoubleUsdRiAttr" -ln "myDoubleUsdRiAttr" -at "double";
	addAttr -ci true -sn "myDoubleAngleUsdAttr" -ln "myDoubleAngleUsdAttr" -at "doubleAngle";
	addAttr -ci true -sn "myDoubleAnglePrimvar" -ln "myDoubleAnglePrimvar" -at "doubleAngle";
	addAttr -ci true -sn "myDoubleAngleUsdRiAttr" -ln "myDoubleAngleUsdRiAttr" -at "doubleAngle";
	addAttr -ci true -sn "myDoubleLinearUsdAttr" -ln "myDoubleLinearUsdAttr" -at "doubleLinear";
	addAttr -ci true -sn "myDoubleLinearPrimvar" -ln "myDoubleLinearPrimvar" -at "doubleLinear";
	addAttr -ci true -sn "myDoubleLinearUsdRiAttr" -ln "myDoubleLinearUsdRiAttr" -at "doubleLinear";
	addAttr -ci true -sn "myStringUsdAttr" -ln "myStringUsdAttr" -dt "string";
	addAttr -ci true -sn "myStringPrimvar" -ln "myStringPrimvar" -dt "string";
	addAttr -ci true -sn "myStringUsdRiAttr" -ln "myStringUsdRiAttr" -dt "string";
	addAttr -ci true -sn "myStringArrayUsdAttr" -ln "myStringArrayUsdAttr" -dt "stringArray";
	addAttr -ci true -sn "myStringArrayPrimvar" -ln "myStringArrayPrimvar" -dt "stringArray";
	addAttr -ci true -sn "myStringArrayUsdRiAttr" -ln "myStringArrayUsdRiAttr" -dt "stringArray";
	addAttr -ci true -sn "myDoubleMatrixUsdAttr" -ln "myDoubleMatrixUsdAttr" -dt "matrix";
	addAttr -ci true -sn "myDoubleMatrixPrimvar" -ln "myDoubleMatrixPrimvar" -dt "matrix";
	addAttr -ci true -sn "myDoubleMatrixUsdRiAttr" -ln "myDoubleMatrixUsdRiAttr" -dt "matrix";
	addAttr -ci true -sn "myFloatMatrixUsdAttr" -ln "myFloatMatrixUsdAttr" -at "fltMatrix";
	addAttr -ci true -sn "myFloatMatrixPrimvar" -ln "myFloatMatrixPrimvar" -at "fltMatrix";
	addAttr -ci true -sn "myFloatMatrixUsdRiAttr" -ln "myFloatMatrixUsdRiAttr" -at "fltMatrix";
	addAttr -ci true -sn "myFloat2UsdAttr" -ln "myFloat2UsdAttr" -dt "float2";
	addAttr -ci true -sn "myFloat2Primvar" -ln "myFloat2Primvar" -dt "float2";
	addAttr -ci true -sn "myFloat2UsdRiAttr" -ln "myFloat2UsdRiAttr" -dt "float2";
	addAttr -ci true -sn "myFloat3UsdAttr" -ln "myFloat3UsdAttr" -dt "float3";
	addAttr -ci true -sn "myFloat3Primvar" -ln "myFloat3Primvar" -dt "float3";
	addAttr -ci true -sn "myFloat3UsdRiAttr" -ln "myFloat3UsdRiAttr" -dt "float3";
	addAttr -ci true -sn "myDouble2UsdAttr" -ln "myDouble2UsdAttr" -dt "double2";
	addAttr -ci true -sn "myDouble2Primvar" -ln "myDouble2Primvar" -dt "double2";
	addAttr -ci true -sn "myDouble2UsdRiAttr" -ln "myDouble2UsdRiAttr" -dt "double2";
	addAttr -ci true -sn "myDouble3UsdAttr" -ln "myDouble3UsdAttr" -dt "double3";
	addAttr -ci true -sn "myDouble3Primvar" -ln "myDouble3Primvar" -dt "double3";
	addAttr -ci true -sn "myDouble3UsdRiAttr" -ln "myDouble3UsdRiAttr" -dt "double3";
	addAttr -ci true -sn "myDouble4UsdAttr" -ln "myDouble4UsdAttr" -dt "double4";
	addAttr -ci true -sn "myDouble4Primvar" -ln "myDouble4Primvar" -dt "double4";
	addAttr -ci true -sn "myDouble4UsdRiAttr" -ln "myDouble4UsdRiAttr" -dt "double4";
	addAttr -ci true -sn "myLong2UsdAttr" -ln "myLong2UsdAttr" -dt "long2";
	addAttr -ci true -sn "myLong2Primvar" -ln "myLong2Primvar" -dt "long2";
	addAttr -ci true -sn "myLong2UsdRiAttr" -ln "myLong2UsdRiAttr" -dt "long2";
	addAttr -ci true -sn "myLong3UsdAttr" -ln "myLong3UsdAttr" -dt "long3";
	addAttr -ci true -sn "myLong3Primvar" -ln "myLong3Primvar" -dt "long3";
	addAttr -ci true -sn "myLong3UsdRiAttr" -ln "myLong3UsdRiAttr" -dt "long3";
	addAttr -ci true -sn "myShort2UsdAttr" -ln "myShort2UsdAttr" -dt "short2";
	addAttr -ci true -sn "myShort2Primvar" -ln "myShort2Primvar" -dt "short2";
	addAttr -ci true -sn "myShort2UsdRiAttr" -ln "myShort2UsdRiAttr" -dt "short2";
	addAttr -ci true -sn "myShort3UsdAttr" -ln "myShort3UsdAttr" -dt "short3";
	addAttr -ci true -sn "myShort3Primvar" -ln "myShort3Primvar" -dt "short3";
	addAttr -ci true -sn "myShort3UsdRiAttr" -ln "myShort3UsdRiAttr" -dt "short3";
	addAttr -ci true -sn "myDoubleArrayUsdAttr" -ln "myDoubleArrayUsdAttr" -dt "doubleArray";
	addAttr -ci true -sn "myDoubleArrayPrimvar" -ln "myDoubleArrayPrimvar" -dt "doubleArray";
	addAttr -ci true -sn "myDoubleArrayUsdRiAttr" -ln "myDoubleArrayUsdRiAttr" -dt "doubleArray";
	addAttr -ci true -sn "myFloatArrayUsdAttr" -ln "myFloatArrayUsdAttr" -dt "floatArray";
	addAttr -ci true -sn "myFloatArrayPrimvar" -ln "myFloatArrayPrimvar" -dt "floatArray";
	addAttr -ci true -sn "myFloatArrayUsdRiAttr" -ln "myFloatArrayUsdRiAttr" -dt "floatArray";
	addAttr -ci true -sn "myIntArrayUsdAttr" -ln "myIntArrayUsdAttr" -dt "Int32Array";
	addAttr -ci true -sn "myIntArrayPrimvar" -ln "myIntArrayPrimvar" -dt "Int32Array";
	addAttr -ci true -sn "myIntArrayUsdRiAttr" -ln "myIntArrayUsdRiAttr" -dt "Int32Array";
	addAttr -ci true -sn "myVectorArrayUsdAttr" -ln "myVectorArrayUsdAttr" -dt "vectorArray";
	addAttr -ci true -sn "myVectorArrayPrimvar" -ln "myVectorArrayPrimvar" -dt "vectorArray";
	addAttr -ci true -sn "myVectorArrayUsdRiAttr" -ln "myVectorArrayUsdRiAttr" -dt "vectorArray";
	addAttr -ci true -sn "myPointArrayUsdAttr" -ln "myPointArrayUsdAttr" -dt "pointArray";
	addAttr -ci true -sn "myPointArrayPrimvar" -ln "myPointArrayPrimvar" -dt "pointArray";
	addAttr -ci true -sn "myPointArrayUsdRiAttr" -ln "myPointArrayUsdRiAttr" -dt "pointArray";
	addAttr -ci true -sn "multiSelectTestAttr" -ln "multiSelectTestAttr" -at "double";
	addAttr -ci true -sn "USD_UserExportedAttributesJson" -ln "USD_UserExportedAttributesJson" 
		-dt "string";
	setAttr ".rp" -type "double3" 0 -10 2.5 ;
	setAttr ".sp" -type "double3" 0 -10 2.5 ;
	setAttr ".myBoolUsdAttr" yes;
	setAttr ".myBoolPrimvar" yes;
	setAttr ".myBoolUsdRiAttr" yes;
	setAttr ".myLongUsdAttr" 42;
	setAttr ".myLongPrimvar" 42;
	setAttr ".myLongUsdRiAttr" 42;
	setAttr ".myShortUsdAttr" 42;
	setAttr ".myShortPrimvar" 42;
	setAttr ".myShortUsdRiAttr" 42;
	setAttr ".myByteUsdAttr" 42;
	setAttr ".myBytePrimvar" 42;
	setAttr ".myByteUsdRiAttr" 42;
	setAttr ".myCharUsdAttr" 42;
	setAttr ".myCharPrimvar" 42;
	setAttr ".myCharUsdRiAttr" 42;
	setAttr ".myEnumUsdAttr" 2;
	setAttr ".myEnumPrimvar" 2;
	setAttr ".myEnumUsdRiAttr" 2;
	setAttr ".myFloatUsdAttr" 1.1;
	setAttr ".myFloatPrimvar" 1.1;
	setAttr ".myFloatUsdRiAttr" 1.1;
	setAttr ".myDoubleUsdAttr" 1.1;
	setAttr ".myDoublePrimvar" 1.1;
	setAttr ".myDoubleUsdRiAttr" 1.1;
	setAttr ".myDoubleAngleUsdAttr" 180.0;
	setAttr ".myDoubleAnglePrimvar" 180.0;
	setAttr ".myDoubleAngleUsdRiAttr" 180.0;
	setAttr ".myDoubleLinearUsdAttr" 1.1;
	setAttr ".myDoubleLinearPrimvar" 1.1;
	setAttr ".myDoubleLinearUsdRiAttr" 1.1;
	setAttr ".myStringUsdAttr" -type "string" "foo";
	setAttr ".myStringPrimvar" -type "string" "foo";
	setAttr ".myStringUsdRiAttr" -type "string" "foo";
	setAttr ".myStringArrayUsdAttr" -type "stringArray" 3 "foo" "bar" "baz"  ;
	setAttr ".myStringArrayPrimvar" -type "stringArray" 3 "foo" "bar" "baz"  ;
	setAttr ".myStringArrayUsdRiAttr" -type "stringArray" 3 "foo" "bar" "baz"  ;
	setAttr ".myDoubleMatrixUsdAttr" -type "matrix"
		1.1 1.1 1.1 1
		2.2 2.2 2.2 1
		3.3 3.3 3.3 1
		1 1 1 1;
	setAttr ".myDoubleMatrixPrimvar" -type "matrix"
		1.1 1.1 1.1 1
		2.2 2.2 2.2 1
		3.3 3.3 3.3 1
		1 1 1 1;
	setAttr ".myDoubleMatrixUsdRiAttr" -type "matrix"
		1.1 1.1 1.1 1
		2.2 2.2 2.2 1
		3.3 3.3 3.3 1
		1 1 1 1;
	setAttr ".myFloatMatrixUsdAttr" -type "matrix"
		1.1 1.1 1.1 1
		2.2 2.2 2.2 1
		3.3 3.3 3.3 1
		1 1 1 1;
	setAttr ".myFloatMatrixPrimvar" -type "matrix"
		1.1 1.1 1.1 1
		2.2 2.2 2.2 1
		3.3 3.3 3.3 1
		1 1 1 1;
	setAttr ".myFloatMatrixUsdRiAttr" -type "matrix"
		1.1 1.1 1.1 1
		2.2 2.2 2.2 1
		3.3 3.3 3.3 1
		1 1 1 1;
	setAttr ".myFloat2UsdAttr" -type "float2" 1.1 2.2 ;
	setAttr ".myFloat2Primvar" -type "float2" 1.1 2.2 ;
	setAttr ".myFloat2UsdRiAttr" -type "float2" 1.1 2.2 ;
	setAttr ".myFloat3UsdAttr" -type "float3" 1.1 2.2 3.3 ;
	setAttr ".myFloat3Primvar" -type "float3" 1.1 2.2 3.3 ;
	setAttr ".myFloat3UsdRiAttr" -type "float3" 1.1 2.2 3.3 ;
	setAttr ".myDouble2UsdAttr" -type "double2" 1.1 2.2 ;
	setAttr ".myDouble2Primvar" -type "double2" 1.1 2.2 ;
	setAttr ".myDouble2UsdRiAttr" -type "double2" 1.1 2.2 ;
	setAttr ".myDouble3UsdAttr" -type "double3" 1.1 2.2 3.3 ;
	setAttr ".myDouble3Primvar" -type "double3" 1.1 2.2 3.3 ;
	setAttr ".myDouble3UsdRiAttr" -type "double3" 1.1 2.2 3.3 ;
	setAttr ".myDouble4UsdAttr" -type "double4" 1.1 2.2 3.3 4.4 ;
	setAttr ".myDouble4Primvar" -type "double4" 1.1 2.2 3.3 4.4 ;
	setAttr ".myDouble4UsdRiAttr" -type "double4" 1.1 2.2 3.3 4.4 ;
	setAttr ".myLong2UsdAttr" -type "long2" 1 2 ;
	setAttr ".myLong2Primvar" -type "long2" 1 2 ;
	setAttr ".myLong2UsdRiAttr" -type "long2" 1 2 ;
	setAttr ".myLong3UsdAttr" -type "long3" 1 2 3 ;
	setAttr ".myLong3Primvar" -type "long3" 1 2 3 ;
	setAttr ".myLong3UsdRiAttr" -type "long3" 1 2 3 ;
	setAttr ".myShort2UsdAttr" -type "short2" 1 2 ;
	setAttr ".myShort2Primvar" -type "short2" 1 2 ;
	setAttr ".myShort2UsdRiAttr" -type "short2" 1 2 ;
	setAttr ".myShort3UsdAttr" -type "short3" 1 2 3 ;
	setAttr ".myShort3Primvar" -type "short3" 1 2 3 ;
	setAttr ".myShort3UsdRiAttr" -type "short3" 1 2 3 ;
	setAttr ".myDoubleArrayUsdAttr" -type "doubleArray" 3 1.1 2.2 3.3 ;
	setAttr ".myDoubleArrayPrimvar" -type "doubleArray" 3 1.1 2.2 3.3 ;
	setAttr ".myDoubleArrayUsdRiAttr" -type "doubleArray" 3 1.1 2.2 3.3 ;
	setAttr ".myFloatArrayUsdAttr" -type "floatArray" 3 1.1 2.2 3.3 ;
	setAttr ".myFloatArrayPrimvar" -type "floatArray" 3 1.1 2.2 3.3 ;
	setAttr ".myFloatArrayUsdRiAttr" -type "floatArray" 3 1.1 2.2 3.3 ;
	setAttr ".myIntArrayUsdAttr" -type "Int32Array" 3 1 2 3 ;
	setAttr ".myIntArrayPrimvar" -type "Int32Array" 3 1 2 3 ;
	setAttr ".myIntArrayUsdRiAttr" -type "Int32Array" 3 1 2 3 ;
	setAttr ".myVectorArrayUsdAttr" -type "vectorArray" 3
		1.1 1.1 1.1
		2.2 2.2 2.2
		3.3 3.3 3.3 ;
	setAttr ".myVectorArrayPrimvar" -type "vectorArray" 3
		1.1 1.1 1.1
		2.2 2.2 2.2
		3.3 3.3 3.3 ;
	setAttr ".myVectorArrayUsdRiAttr" -type "vectorArray" 3
		1.1 1.1 1.1
		2.2 2.2 2.2
		3.3 3.3 3.3 ;
	setAttr ".myPointArrayUsdAttr" -type "pointArray" 3
		1.1 1.1 1.1 0.0
		2.2 2.2 2.2 1.0
		6.6 6.6 6.6 2.0 ;
	setAttr ".myPointArrayPrimvar" -type "pointArray" 3
		1.1 1.1 1.1 0.0
		2.2 2.2 2.2 1.0
		6.6 6.6 6.6 2.0 ;
	setAttr ".myPointArrayUsdRiAttr" -type "pointArray" 3
		1.1 1.1 1.1 0.0
		2.2 2.2 2.2 1.0
		6.6 6.6 6.6 2.0 ;
	setAttr ".multiSelectTestAttr" 1.1;
	setAttr ".USD_UserExportedAttributesJson" -type "string" (
		"{\"myDoubleMatrixUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDoubleUsdAttr\": {}, \"myDoubleLinearUsdAttr\": {}, \"myDouble3Primvar\": {\"usdAttrType\": \"primvar\"}, \"myDouble2Primvar\": {\"usdAttrType\": \"primvar\"}, \"myFloat2UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDouble2UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myShort2UsdAttr\": {}, \"myIntArrayPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myByteUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDouble2UsdAttr\": {}, \"myStringPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myDoubleArrayUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myShort2UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myShort2Primvar\": {\"usdAttrType\": \"primvar\"}, \"myFloatMatrixUsdAttr\": {}, \"myFloat3Primvar\": {\"usdAttrType\": \"primvar\"}, \"myLong3UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myLong3UsdAttr\": {}, \"myIntArrayUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDoubleUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myFloatMatrixUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myBoolUsdAttr\": {}, \"myShort3UsdAttr\": {}, \"myFloatUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myBytePrimvar\": {\"usdAttrType\": \"primvar\"}, \"myLong2UsdAttr\": {}, \"myFloatArrayPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myEnumUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myShortUsdAttr\": {}, \"myStringArrayUsdAttr\": {}, \"myFloatPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myStringUsdAttr\": {}, \"myIntArrayUsdAttr\": {}, \"myFloatMatrixPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myEnumUsdAttr\": {}, \"myFloatArrayUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myVectorArrayPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myLong2UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDoubleAngleUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myBoolUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myLongUsdAttr\": {}, \"myCharPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myFloat2Primvar\": {\"usdAttrType\": \"primvar\"}, \"myFloatArrayUsdAttr\": {}, \"myStringArrayUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myFloat2UsdAttr\": {}, \"myDoubleAnglePrimvar\": {\"usdAttrType\": \"primvar\"}, \"myLongPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myDoublePrimvar\": {\"usdAttrType\": \"primvar\"}, \"myShortPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myVectorArrayUsdAttr\": {}, \"myDoubleAngleUsdAttr\": {}, \"myPointArrayPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myFloat3UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myFloatUsdAttr\": {}, \"myBoolPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myShortUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDoubleLinearPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myDouble3UsdAttr\": {}, \"myDouble4Primvar\": {\"usdAttrType\": \"primvar\"}, \"myPointArrayUsdAttr\": {}, \"myPointArrayUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDoubleLinearUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDouble4UsdAttr\": {}, \"myStringUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDoubleMatrixUsdAttr\": {}, \"myDoubleArrayUsdAttr\": {}, \"myDouble3UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myByteUsdAttr\": {}, \"myCharUsdAttr\": {}, \"myStringArrayPrimvar\": {\"usdAttrType\": \"primvar\"}, \"multiSelectTestAttr\": {}, \"myShort3UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myFloat3UsdAttr\": {}, \"myLong3Primvar\": {\"usdAttrType\": \"primvar\"}, \"myLongUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myCharUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myVectorArrayUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myEnumPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myDoubleArrayPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myDoubleMatrixPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myDouble4UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myShort3Primvar\": {\"usdAttrType\": \"primvar\"}, \"myLong2Primvar\": {\"usdAttrType\": \"primvar\"}}");
createNode mesh -n "AllTypesCubeShape" -p "AllTypesCube";
	rename -uid "1A9428C0-0000-0FD8-5846-1F700000025D";
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
	setAttr ".mgi" -type "string" "ID_436619ce-d862-42fe-8712-e8b846f981ba";
createNode transform -n "AllTypesCastDoubleToFloatCube" -p "AllMayaTypesTestingCubes";
	rename -uid "BDFF38C0-0000-3C37-5848-6E3B00000342";
	addAttr -ci true -sn "myBoolUsdAttr" -ln "myBoolUsdAttr" -min 0 -max 1 -at "bool";
	addAttr -ci true -sn "myBoolPrimvar" -ln "myBoolPrimvar" -min 0 -max 1 -at "bool";
	addAttr -ci true -sn "myBoolUsdRiAttr" -ln "myBoolUsdRiAttr" -min 0 -max 1 -at "bool";
	addAttr -ci true -sn "myLongUsdAttr" -ln "myLongUsdAttr" -at "long";
	addAttr -ci true -sn "myLongPrimvar" -ln "myLongPrimvar" -at "long";
	addAttr -ci true -sn "myLongUsdRiAttr" -ln "myLongUsdRiAttr" -at "long";
	addAttr -ci true -sn "myShortUsdAttr" -ln "myShortUsdAttr" -at "short";
	addAttr -ci true -sn "myShortPrimvar" -ln "myShortPrimvar" -at "short";
	addAttr -ci true -sn "myShortUsdRiAttr" -ln "myShortUsdRiAttr" -at "short";
	addAttr -ci true -sn "myByteUsdAttr" -ln "myByteUsdAttr" -min 0 -max 255 -at "byte";
	addAttr -ci true -sn "myBytePrimvar" -ln "myBytePrimvar" -min 0 -max 255 -at "byte";
	addAttr -ci true -sn "myByteUsdRiAttr" -ln "myByteUsdRiAttr" -min 0 -max 255 -at "byte";
	addAttr -ci true -sn "myCharUsdAttr" -ln "myCharUsdAttr" -min 0 -max 255 -at "char";
	addAttr -ci true -sn "myCharPrimvar" -ln "myCharPrimvar" -min 0 -max 255 -at "char";
	addAttr -ci true -sn "myCharUsdRiAttr" -ln "myCharUsdRiAttr" -min 0 -max 255 -at "char";
	addAttr -ci true -sn "myEnumUsdAttr" -ln "myEnumUsdAttr" -dv 1 -min 1 -max 3 -en 
		"One=1:Two:Three" -at "enum";
	addAttr -ci true -sn "myEnumPrimvar" -ln "myEnumPrimvar" -dv 1 -min 1 -max 3 -en 
		"One=1:Two:Three" -at "enum";
	addAttr -ci true -sn "myEnumUsdRiAttr" -ln "myEnumUsdRiAttr" -dv 1 -min 1 -max 3 
		-en "One=1:Two:Three" -at "enum";
	addAttr -ci true -sn "myFloatUsdAttr" -ln "myFloatUsdAttr" -at "float";
	addAttr -ci true -sn "myFloatPrimvar" -ln "myFloatPrimvar" -at "float";
	addAttr -ci true -sn "myFloatUsdRiAttr" -ln "myFloatUsdRiAttr" -at "float";
	addAttr -ci true -sn "myDoubleUsdAttr" -ln "myDoubleUsdAttr" -at "double";
	addAttr -ci true -sn "myDoublePrimvar" -ln "myDoublePrimvar" -at "double";
	addAttr -ci true -sn "myDoubleUsdRiAttr" -ln "myDoubleUsdRiAttr" -at "double";
	addAttr -ci true -sn "myDoubleAngleUsdAttr" -ln "myDoubleAngleUsdAttr" -at "doubleAngle";
	addAttr -ci true -sn "myDoubleAnglePrimvar" -ln "myDoubleAnglePrimvar" -at "doubleAngle";
	addAttr -ci true -sn "myDoubleAngleUsdRiAttr" -ln "myDoubleAngleUsdRiAttr" -at "doubleAngle";
	addAttr -ci true -sn "myDoubleLinearUsdAttr" -ln "myDoubleLinearUsdAttr" -at "doubleLinear";
	addAttr -ci true -sn "myDoubleLinearPrimvar" -ln "myDoubleLinearPrimvar" -at "doubleLinear";
	addAttr -ci true -sn "myDoubleLinearUsdRiAttr" -ln "myDoubleLinearUsdRiAttr" -at "doubleLinear";
	addAttr -ci true -sn "myStringUsdAttr" -ln "myStringUsdAttr" -dt "string";
	addAttr -ci true -sn "myStringPrimvar" -ln "myStringPrimvar" -dt "string";
	addAttr -ci true -sn "myStringUsdRiAttr" -ln "myStringUsdRiAttr" -dt "string";
	addAttr -ci true -sn "myStringArrayUsdAttr" -ln "myStringArrayUsdAttr" -dt "stringArray";
	addAttr -ci true -sn "myStringArrayPrimvar" -ln "myStringArrayPrimvar" -dt "stringArray";
	addAttr -ci true -sn "myStringArrayUsdRiAttr" -ln "myStringArrayUsdRiAttr" -dt "stringArray";
	addAttr -ci true -sn "myDoubleMatrixUsdAttr" -ln "myDoubleMatrixUsdAttr" -dt "matrix";
	addAttr -ci true -sn "myDoubleMatrixPrimvar" -ln "myDoubleMatrixPrimvar" -dt "matrix";
	addAttr -ci true -sn "myDoubleMatrixUsdRiAttr" -ln "myDoubleMatrixUsdRiAttr" -dt "matrix";
	addAttr -ci true -sn "myFloatMatrixUsdAttr" -ln "myFloatMatrixUsdAttr" -at "fltMatrix";
	addAttr -ci true -sn "myFloatMatrixPrimvar" -ln "myFloatMatrixPrimvar" -at "fltMatrix";
	addAttr -ci true -sn "myFloatMatrixUsdRiAttr" -ln "myFloatMatrixUsdRiAttr" -at "fltMatrix";
	addAttr -ci true -sn "myFloat2UsdAttr" -ln "myFloat2UsdAttr" -dt "float2";
	addAttr -ci true -sn "myFloat2Primvar" -ln "myFloat2Primvar" -dt "float2";
	addAttr -ci true -sn "myFloat2UsdRiAttr" -ln "myFloat2UsdRiAttr" -dt "float2";
	addAttr -ci true -sn "myFloat3UsdAttr" -ln "myFloat3UsdAttr" -dt "float3";
	addAttr -ci true -sn "myFloat3Primvar" -ln "myFloat3Primvar" -dt "float3";
	addAttr -ci true -sn "myFloat3UsdRiAttr" -ln "myFloat3UsdRiAttr" -dt "float3";
	addAttr -ci true -sn "myDouble2UsdAttr" -ln "myDouble2UsdAttr" -dt "double2";
	addAttr -ci true -sn "myDouble2Primvar" -ln "myDouble2Primvar" -dt "double2";
	addAttr -ci true -sn "myDouble2UsdRiAttr" -ln "myDouble2UsdRiAttr" -dt "double2";
	addAttr -ci true -sn "myDouble3UsdAttr" -ln "myDouble3UsdAttr" -dt "double3";
	addAttr -ci true -sn "myDouble3Primvar" -ln "myDouble3Primvar" -dt "double3";
	addAttr -ci true -sn "myDouble3UsdRiAttr" -ln "myDouble3UsdRiAttr" -dt "double3";
	addAttr -ci true -sn "myDouble4UsdAttr" -ln "myDouble4UsdAttr" -dt "double4";
	addAttr -ci true -sn "myDouble4Primvar" -ln "myDouble4Primvar" -dt "double4";
	addAttr -ci true -sn "myDouble4UsdRiAttr" -ln "myDouble4UsdRiAttr" -dt "double4";
	addAttr -ci true -sn "myLong2UsdAttr" -ln "myLong2UsdAttr" -dt "long2";
	addAttr -ci true -sn "myLong2Primvar" -ln "myLong2Primvar" -dt "long2";
	addAttr -ci true -sn "myLong2UsdRiAttr" -ln "myLong2UsdRiAttr" -dt "long2";
	addAttr -ci true -sn "myLong3UsdAttr" -ln "myLong3UsdAttr" -dt "long3";
	addAttr -ci true -sn "myLong3Primvar" -ln "myLong3Primvar" -dt "long3";
	addAttr -ci true -sn "myLong3UsdRiAttr" -ln "myLong3UsdRiAttr" -dt "long3";
	addAttr -ci true -sn "myShort2UsdAttr" -ln "myShort2UsdAttr" -dt "short2";
	addAttr -ci true -sn "myShort2Primvar" -ln "myShort2Primvar" -dt "short2";
	addAttr -ci true -sn "myShort2UsdRiAttr" -ln "myShort2UsdRiAttr" -dt "short2";
	addAttr -ci true -sn "myShort3UsdAttr" -ln "myShort3UsdAttr" -dt "short3";
	addAttr -ci true -sn "myShort3Primvar" -ln "myShort3Primvar" -dt "short3";
	addAttr -ci true -sn "myShort3UsdRiAttr" -ln "myShort3UsdRiAttr" -dt "short3";
	addAttr -ci true -sn "myDoubleArrayUsdAttr" -ln "myDoubleArrayUsdAttr" -dt "doubleArray";
	addAttr -ci true -sn "myDoubleArrayPrimvar" -ln "myDoubleArrayPrimvar" -dt "doubleArray";
	addAttr -ci true -sn "myDoubleArrayUsdRiAttr" -ln "myDoubleArrayUsdRiAttr" -dt "doubleArray";
	addAttr -ci true -sn "myFloatArrayUsdAttr" -ln "myFloatArrayUsdAttr" -dt "floatArray";
	addAttr -ci true -sn "myFloatArrayPrimvar" -ln "myFloatArrayPrimvar" -dt "floatArray";
	addAttr -ci true -sn "myFloatArrayUsdRiAttr" -ln "myFloatArrayUsdRiAttr" -dt "floatArray";
	addAttr -ci true -sn "myIntArrayUsdAttr" -ln "myIntArrayUsdAttr" -dt "Int32Array";
	addAttr -ci true -sn "myIntArrayPrimvar" -ln "myIntArrayPrimvar" -dt "Int32Array";
	addAttr -ci true -sn "myIntArrayUsdRiAttr" -ln "myIntArrayUsdRiAttr" -dt "Int32Array";
	addAttr -ci true -sn "myVectorArrayUsdAttr" -ln "myVectorArrayUsdAttr" -dt "vectorArray";
	addAttr -ci true -sn "myVectorArrayPrimvar" -ln "myVectorArrayPrimvar" -dt "vectorArray";
	addAttr -ci true -sn "myVectorArrayUsdRiAttr" -ln "myVectorArrayUsdRiAttr" -dt "vectorArray";
	addAttr -ci true -sn "myPointArrayUsdAttr" -ln "myPointArrayUsdAttr" -dt "pointArray";
	addAttr -ci true -sn "myPointArrayPrimvar" -ln "myPointArrayPrimvar" -dt "pointArray";
	addAttr -ci true -sn "myPointArrayUsdRiAttr" -ln "myPointArrayUsdRiAttr" -dt "pointArray";
	addAttr -ci true -sn "multiSelectTestAttr" -ln "multiSelectTestAttr" -at "float";
	addAttr -ci true -sn "USD_UserExportedAttributesJson" -ln "USD_UserExportedAttributesJson" 
		-dt "string";
	setAttr ".rp" -type "double3" 10 -10 2.5 ;
	setAttr ".sp" -type "double3" 10 -10 2.5 ;
	setAttr ".myBoolUsdAttr" yes;
	setAttr ".myBoolPrimvar" yes;
	setAttr ".myBoolUsdRiAttr" yes;
	setAttr ".myLongUsdAttr" 42;
	setAttr ".myLongPrimvar" 42;
	setAttr ".myLongUsdRiAttr" 42;
	setAttr ".myShortUsdAttr" 42;
	setAttr ".myShortPrimvar" 42;
	setAttr ".myShortUsdRiAttr" 42;
	setAttr ".myByteUsdAttr" 42;
	setAttr ".myBytePrimvar" 42;
	setAttr ".myByteUsdRiAttr" 42;
	setAttr ".myCharUsdAttr" 42;
	setAttr ".myCharPrimvar" 42;
	setAttr ".myCharUsdRiAttr" 42;
	setAttr ".myEnumUsdAttr" 2;
	setAttr ".myEnumPrimvar" 2;
	setAttr ".myEnumUsdRiAttr" 2;
	setAttr ".myFloatUsdAttr" 1.1;
	setAttr ".myFloatPrimvar" 1.1;
	setAttr ".myFloatUsdRiAttr" 1.1;
	setAttr ".myDoubleUsdAttr" 1.1;
	setAttr ".myDoublePrimvar" 1.1;
	setAttr ".myDoubleUsdRiAttr" 1.1;
	setAttr ".myDoubleAngleUsdAttr" 180.0;
	setAttr ".myDoubleAnglePrimvar" 180.0;
	setAttr ".myDoubleAngleUsdRiAttr" 180.0;
	setAttr ".myDoubleLinearUsdAttr" 1.1;
	setAttr ".myDoubleLinearPrimvar" 1.1;
	setAttr ".myDoubleLinearUsdRiAttr" 1.1;
	setAttr ".myStringUsdAttr" -type "string" "foo";
	setAttr ".myStringPrimvar" -type "string" "foo";
	setAttr ".myStringUsdRiAttr" -type "string" "foo";
	setAttr ".myStringArrayUsdAttr" -type "stringArray" 3 "foo" "bar" "baz"  ;
	setAttr ".myStringArrayPrimvar" -type "stringArray" 3 "foo" "bar" "baz"  ;
	setAttr ".myStringArrayUsdRiAttr" -type "stringArray" 3 "foo" "bar" "baz"  ;
	setAttr ".myDoubleMatrixUsdAttr" -type "matrix"
		1.1 1.1 1.1 1
		2.2 2.2 2.2 1
		3.3 3.3 3.3 1
		1 1 1 1;
	setAttr ".myDoubleMatrixPrimvar" -type "matrix"
		1.1 1.1 1.1 1
		2.2 2.2 2.2 1
		3.3 3.3 3.3 1
		1 1 1 1;
	setAttr ".myDoubleMatrixUsdRiAttr" -type "matrix"
		1.1 1.1 1.1 1
		2.2 2.2 2.2 1
		3.3 3.3 3.3 1
		1 1 1 1;
	setAttr ".myFloatMatrixUsdAttr" -type "matrix"
		1.1 1.1 1.1 1
		2.2 2.2 2.2 1
		3.3 3.3 3.3 1
		1 1 1 1;
	setAttr ".myFloatMatrixPrimvar" -type "matrix"
		1.1 1.1 1.1 1
		2.2 2.2 2.2 1
		3.3 3.3 3.3 1
		1 1 1 1;
	setAttr ".myFloatMatrixUsdRiAttr" -type "matrix"
		1.1 1.1 1.1 1
		2.2 2.2 2.2 1
		3.3 3.3 3.3 1
		1 1 1 1;
	setAttr ".myFloat2UsdAttr" -type "float2" 1.1 2.2 ;
	setAttr ".myFloat2Primvar" -type "float2" 1.1 2.2 ;
	setAttr ".myFloat2UsdRiAttr" -type "float2" 1.1 2.2 ;
	setAttr ".myFloat3UsdAttr" -type "float3" 1.1 2.2 3.3 ;
	setAttr ".myFloat3Primvar" -type "float3" 1.1 2.2 3.3 ;
	setAttr ".myFloat3UsdRiAttr" -type "float3" 1.1 2.2 3.3 ;
	setAttr ".myDouble2UsdAttr" -type "double2" 1.1 2.2 ;
	setAttr ".myDouble2Primvar" -type "double2" 1.1 2.2 ;
	setAttr ".myDouble2UsdRiAttr" -type "double2" 1.1 2.2 ;
	setAttr ".myDouble3UsdAttr" -type "double3" 1.1 2.2 3.3 ;
	setAttr ".myDouble3Primvar" -type "double3" 1.1 2.2 3.3 ;
	setAttr ".myDouble3UsdRiAttr" -type "double3" 1.1 2.2 3.3 ;
	setAttr ".myDouble4UsdAttr" -type "double4" 1.1 2.2 3.3 4.4 ;
	setAttr ".myDouble4Primvar" -type "double4" 1.1 2.2 3.3 4.4 ;
	setAttr ".myDouble4UsdRiAttr" -type "double4" 1.1 2.2 3.3 4.4 ;
	setAttr ".myLong2UsdAttr" -type "long2" 1 2 ;
	setAttr ".myLong2Primvar" -type "long2" 1 2 ;
	setAttr ".myLong2UsdRiAttr" -type "long2" 1 2 ;
	setAttr ".myLong3UsdAttr" -type "long3" 1 2 3 ;
	setAttr ".myLong3Primvar" -type "long3" 1 2 3 ;
	setAttr ".myLong3UsdRiAttr" -type "long3" 1 2 3 ;
	setAttr ".myShort2UsdAttr" -type "short2" 1 2 ;
	setAttr ".myShort2Primvar" -type "short2" 1 2 ;
	setAttr ".myShort2UsdRiAttr" -type "short2" 1 2 ;
	setAttr ".myShort3UsdAttr" -type "short3" 1 2 3 ;
	setAttr ".myShort3Primvar" -type "short3" 1 2 3 ;
	setAttr ".myShort3UsdRiAttr" -type "short3" 1 2 3 ;
	setAttr ".myDoubleArrayUsdAttr" -type "doubleArray" 3 1.1 2.2 3.3 ;
	setAttr ".myDoubleArrayPrimvar" -type "doubleArray" 3 1.1 2.2 3.3 ;
	setAttr ".myDoubleArrayUsdRiAttr" -type "doubleArray" 3 1.1 2.2 3.3 ;
	setAttr ".myFloatArrayUsdAttr" -type "floatArray" 3 1.1 2.2 3.3 ;
	setAttr ".myFloatArrayPrimvar" -type "floatArray" 3 1.1 2.2 3.3 ;
	setAttr ".myFloatArrayUsdRiAttr" -type "floatArray" 3 1.1 2.2 3.3 ;
	setAttr ".myIntArrayUsdAttr" -type "Int32Array" 3 1 2 3 ;
	setAttr ".myIntArrayPrimvar" -type "Int32Array" 3 1 2 3 ;
	setAttr ".myIntArrayUsdRiAttr" -type "Int32Array" 3 1 2 3 ;
	setAttr ".myVectorArrayUsdAttr" -type "vectorArray" 3
		1.1 1.1 1.1
		2.2 2.2 2.2
		3.3 3.3 3.3 ;
	setAttr ".myVectorArrayPrimvar" -type "vectorArray" 3
		1.1 1.1 1.1
		2.2 2.2 2.2
		3.3 3.3 3.3 ;
	setAttr ".myVectorArrayUsdRiAttr" -type "vectorArray" 3
		1.1 1.1 1.1
		2.2 2.2 2.2
		3.3 3.3 3.3 ;
	setAttr ".myPointArrayUsdAttr" -type "pointArray" 3
		1.1 1.1 1.1 0.0
		2.2 2.2 2.2 1.0
		6.6 6.6 6.6 2.0 ;
	setAttr ".myPointArrayPrimvar" -type "pointArray" 3
		1.1 1.1 1.1 0.0
		2.2 2.2 2.2 1.0
		6.6 6.6 6.6 2.0 ;
	setAttr ".myPointArrayUsdRiAttr" -type "pointArray" 3
		1.1 1.1 1.1 0.0
		2.2 2.2 2.2 1.0
		6.6 6.6 6.6 2.0 ;
	setAttr ".multiSelectTestAttr" 1.1;
	setAttr ".USD_UserExportedAttributesJson" -type "string" (
		"{\"myDoubleMatrixUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDoubleUsdAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true}, \"myDoubleLinearUsdAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true}, \"myDouble3Primvar\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"primvar\"}, \"myDouble2Primvar\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"primvar\"}, \"myFloat2UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDouble2UsdRiAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"usdRi\"}, \"myShort2UsdAttr\": {}, \"myIntArrayPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myByteUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDouble2UsdAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true}, \"myStringPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myDoubleArrayUsdRiAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"usdRi\"}, \"myShort2UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myShort2Primvar\": {\"usdAttrType\": \"primvar\"}, \"myFloatMatrixUsdAttr\": {}, \"myFloat3Primvar\": {\"usdAttrType\": \"primvar\"}, \"myLong3UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myLong3UsdAttr\": {}, \"myIntArrayUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDoubleUsdRiAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"usdRi\"}, \"myFloatMatrixUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myBoolUsdAttr\": {}, \"myShort3UsdAttr\": {}, \"myFloatUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myBytePrimvar\": {\"usdAttrType\": \"primvar\"}, \"myLong2UsdAttr\": {}, \"myFloatArrayPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myEnumUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myShortUsdAttr\": {}, \"myStringArrayUsdAttr\": {}, \"myFloatPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myStringUsdAttr\": {}, \"myIntArrayUsdAttr\": {}, \"myFloatMatrixPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myEnumUsdAttr\": {}, \"myFloatArrayUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myVectorArrayPrimvar\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"primvar\"}, \"myLong2UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDoubleAngleUsdRiAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"usdRi\"}, \"myBoolUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myLongUsdAttr\": {}, \"myCharPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myFloat2Primvar\": {\"usdAttrType\": \"primvar\"}, \"myFloatArrayUsdAttr\": {}, \"myStringArrayUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myFloat2UsdAttr\": {}, \"myDoubleAnglePrimvar\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"primvar\"}, \"myLongPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myDoublePrimvar\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"primvar\"}, \"myShortPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myVectorArrayUsdAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true}, \"myDoubleAngleUsdAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true}, \"myPointArrayPrimvar\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"primvar\"}, \"myFloat3UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myFloatUsdAttr\": {}, \"myBoolPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myShortUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDoubleLinearPrimvar\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"primvar\"}, \"myDouble3UsdAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true}, \"myDouble4Primvar\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"primvar\"}, \"myPointArrayUsdAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true}, \"myPointArrayUsdRiAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"usdRi\"}, \"myDoubleLinearUsdRiAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"usdRi\"}, \"myDouble4UsdAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true}, \"myStringUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myDoubleMatrixUsdAttr\": {}, \"myDoubleArrayUsdAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true}, \"myDouble3UsdRiAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"usdRi\"}, \"myByteUsdAttr\": {}, \"myCharUsdAttr\": {}, \"myStringArrayPrimvar\": {\"usdAttrType\": \"primvar\"}, \"multiSelectTestAttr\": {}, \"myShort3UsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myFloat3UsdAttr\": {}, \"myLong3Primvar\": {\"usdAttrType\": \"primvar\"}, \"myLongUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myCharUsdRiAttr\": {\"usdAttrType\": \"usdRi\"}, \"myVectorArrayUsdRiAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"usdRi\"}, \"myEnumPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myDoubleArrayPrimvar\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"primvar\"}, \"myDoubleMatrixPrimvar\": {\"usdAttrType\": \"primvar\"}, \"myDouble4UsdRiAttr\": {\"translateMayaDoubleToUsdSinglePrecision\": true, \"usdAttrType\": \"usdRi\"}, \"myShort3Primvar\": {\"usdAttrType\": \"primvar\"}, \"myLong2Primvar\": {\"usdAttrType\": \"primvar\"}}");
createNode mesh -n "AllTypesCastDoubleToFloatCubeShape" -p "AllTypesCastDoubleToFloatCube";
	rename -uid "BDFF38C0-0000-3C37-5848-6E3B00000343";
	addAttr -ci true -sn "mgi" -ln "mayaGprimID" -dt "string";
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
	setAttr -s 8 ".pt[0:7]" -type "float3"  10 0 0 10 0 0 10 
		0 0 10 0 0 10 0 0 10 0 0 10 0 0 10 0 0;
	setAttr -s 8 ".vt[0:7]"  -2.5 -12.5 5 2.5 -12.5 5 -2.5 -7.5 5 2.5 -7.5 5
		 -2.5 -7.5 0 2.5 -7.5 0 -2.5 -12.5 0 2.5 -12.5 0;
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
	setAttr ".mgi" -type "string" "ID_74261580-09ab-4121-9fa7-e76f52bbffce";
createNode lightLinker -s -n "lightLinker1";
	rename -uid "658638C0-0000-07C4-5848-793A00000258";
	setAttr -s 2 ".lnk";
	setAttr -s 2 ".slnk";
createNode displayLayerManager -n "layerManager";
	rename -uid "658638C0-0000-07C4-5848-793A00000259";
createNode displayLayer -n "defaultLayer";
	rename -uid "6D105860-0000-7362-5717-ED8500000241";
createNode renderLayerManager -n "renderLayerManager";
	rename -uid "658638C0-0000-07C4-5848-793A0000025B";
createNode renderLayer -n "defaultRenderLayer";
	rename -uid "6D105860-0000-7362-5717-ED8500000243";
	setAttr ".g" yes;
createNode script -n "uiConfigurationScriptNode";
	rename -uid "6D105860-0000-7362-5717-EE040000024F";
	setAttr ".b" -type "string" (
		"// Maya Mel UI Configuration File.\n//\n//  This script is machine generated.  Edit at your own risk.\n//\n//\n\nglobal string $gMainPane;\nif (`paneLayout -exists $gMainPane`) {\n\n\tglobal int $gUseScenePanelConfig;\n\tint    $useSceneConfig = $gUseScenePanelConfig;\n\tint    $menusOkayInPanels = `optionVar -q allowMenusInPanels`;\tint    $nVisPanes = `paneLayout -q -nvp $gMainPane`;\n\tint    $nPanes = 0;\n\tstring $editorName;\n\tstring $panelName;\n\tstring $itemFilterName;\n\tstring $panelConfig;\n\n\t//\n\t//  get current state of the UI\n\t//\n\tsceneUIReplacement -update $gMainPane;\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Top View\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `modelPanel -unParent -l (localizedPanelLabel(\"Top View\")) -mbv $menusOkayInPanels `;\n\t\t\t$editorName = $panelName;\n            modelEditor -e \n                -camera \"top\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"smoothShaded\" \n"
		+ "                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 0\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n"
		+ "                -rendererName \"base_OpenGL_Renderer\" \n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 256 256 \n                -bumpResolution 512 512 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 1\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n"
		+ "                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 1\n                -height 1\n                -sceneRenderFilter 0\n                $editorName;\n            modelEditor -e -viewSelected 0 $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n"
		+ "\t\tmodelPanel -edit -l (localizedPanelLabel(\"Top View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"top\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n"
		+ "            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n"
		+ "            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n"
		+ "\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Side View\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `modelPanel -unParent -l (localizedPanelLabel(\"Side View\")) -mbv $menusOkayInPanels `;\n\t\t\t$editorName = $panelName;\n            modelEditor -e \n                -camera \"side\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"smoothShaded\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 0\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n"
		+ "                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n                -rendererName \"base_OpenGL_Renderer\" \n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 256 256 \n                -bumpResolution 512 512 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 1\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n"
		+ "                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n"
		+ "                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 1\n                -height 1\n                -sceneRenderFilter 0\n                $editorName;\n            modelEditor -e -viewSelected 0 $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Side View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"side\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n"
		+ "            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n"
		+ "            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n"
		+ "            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Front View\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `modelPanel -unParent -l (localizedPanelLabel(\"Front View\")) -mbv $menusOkayInPanels `;\n\t\t\t$editorName = $panelName;\n            modelEditor -e \n                -camera \"front\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"smoothShaded\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n"
		+ "                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 0\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n                -rendererName \"base_OpenGL_Renderer\" \n                -objectFilterShowInHUD 1\n"
		+ "                -isFiltered 0\n                -colorResolution 256 256 \n                -bumpResolution 512 512 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 1\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n"
		+ "                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 1\n                -height 1\n                -sceneRenderFilter 0\n                $editorName;\n            modelEditor -e -viewSelected 0 $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Front View\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"front\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n"
		+ "            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n"
		+ "            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Persp View\")) `;\n\tif (\"\" == $panelName) {\n"
		+ "\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `modelPanel -unParent -l (localizedPanelLabel(\"Persp View\")) -mbv $menusOkayInPanels `;\n\t\t\t$editorName = $panelName;\n            modelEditor -e \n                -camera \"persp\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"smoothShaded\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 0\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n"
		+ "                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n                -rendererName \"base_OpenGL_Renderer\" \n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 256 256 \n                -bumpResolution 512 512 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 1\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n"
		+ "                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n"
		+ "                -motionTrails 1\n                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 1479\n                -height 1068\n                -sceneRenderFilter 0\n                $editorName;\n            modelEditor -e -viewSelected 0 $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Persp View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"persp\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n"
		+ "            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n"
		+ "            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n"
		+ "            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1479\n            -height 1068\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"Outliner\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `outlinerPanel -unParent -l (localizedPanelLabel(\"Outliner\")) -mbv $menusOkayInPanels `;\n\t\t\t$editorName = $panelName;\n            outlinerEditor -e \n                -docTag \"isolOutln_fromSeln\" \n                -showShapes 1\n                -showReferenceNodes 1\n                -showReferenceMembers 1\n                -showAttributes 0\n                -showConnected 0\n                -showAnimCurvesOnly 0\n                -showMuteInfo 0\n                -organizeByLayer 1\n                -showAnimLayerWeight 1\n"
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
		+ "\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"createNodePanel\" -l (localizedPanelLabel(\"Create Node\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Create Node\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"polyTexturePlacementPanel\" (localizedPanelLabel(\"UV Editor\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"polyTexturePlacementPanel\" -l (localizedPanelLabel(\"UV Editor\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"UV Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"renderWindowPanel\" (localizedPanelLabel(\"Render View\")) `;\n"
		+ "\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"renderWindowPanel\" -l (localizedPanelLabel(\"Render View\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Render View\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"blendShapePanel\" (localizedPanelLabel(\"Blend Shape\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\tblendShapePanel -unParent -l (localizedPanelLabel(\"Blend Shape\")) -mbv $menusOkayInPanels ;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tblendShapePanel -edit -l (localizedPanelLabel(\"Blend Shape\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynRelEdPanel\" (localizedPanelLabel(\"Dynamic Relationships\")) `;\n\tif (\"\" == $panelName) {\n"
		+ "\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"dynRelEdPanel\" -l (localizedPanelLabel(\"Dynamic Relationships\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dynamic Relationships\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"relationshipPanel\" (localizedPanelLabel(\"Relationship Editor\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"relationshipPanel\" -l (localizedPanelLabel(\"Relationship Editor\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Relationship Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"referenceEditorPanel\" (localizedPanelLabel(\"Reference Editor\")) `;\n"
		+ "\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"referenceEditorPanel\" -l (localizedPanelLabel(\"Reference Editor\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Reference Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"componentEditorPanel\" (localizedPanelLabel(\"Component Editor\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"componentEditorPanel\" -l (localizedPanelLabel(\"Component Editor\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Component Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynPaintScriptedPanelType\" (localizedPanelLabel(\"Paint Effects\")) `;\n"
		+ "\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"dynPaintScriptedPanelType\" -l (localizedPanelLabel(\"Paint Effects\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Paint Effects\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"scriptEditorPanel\" (localizedPanelLabel(\"Script Editor\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"scriptEditorPanel\" -l (localizedPanelLabel(\"Script Editor\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Script Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"profilerPanel\" (localizedPanelLabel(\"Profiler Tool\")) `;\n"
		+ "\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"profilerPanel\" -l (localizedPanelLabel(\"Profiler Tool\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Profiler Tool\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperShadePanel\" (localizedPanelLabel(\"Hypershade\")) `;\n\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"hyperShadePanel\" -l (localizedPanelLabel(\"Hypershade\")) -mbv $menusOkayInPanels `;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypershade\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"nodeEditorPanel\" (localizedPanelLabel(\"Node Editor\")) `;\n"
		+ "\tif (\"\" == $panelName) {\n\t\tif ($useSceneConfig) {\n\t\t\t$panelName = `scriptedPanel -unParent  -type \"nodeEditorPanel\" -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels `;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -nodeTitleMode \"name\" \n                -gridSnap 0\n                -gridVisibility 1\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n                -useAssets 1\n                -syncedSelection 1\n"
		+ "                -extendToShapes 1\n                -activeTab -1\n                -editorMode \"default\" \n                $editorName;\n\t\t}\n\t} else {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -nodeTitleMode \"name\" \n                -gridSnap 0\n                -gridVisibility 1\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n"
		+ "                -showTransforms 1\n                -useAssets 1\n                -syncedSelection 1\n                -extendToShapes 1\n                -activeTab -1\n                -editorMode \"default\" \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\tif ($useSceneConfig) {\n        string $configName = `getPanel -cwl (localizedPanelLabel(\"Current Layout\"))`;\n        if (\"\" != $configName) {\n\t\t\tpanelConfiguration -edit -label (localizedPanelLabel(\"Current Layout\")) \n\t\t\t\t-defaultImage \"vacantCell.xP:/\"\n\t\t\t\t-image \"\"\n\t\t\t\t-sc false\n\t\t\t\t-configString \"global string $gMainPane; paneLayout -e -cn \\\"vertical2\\\" -ps 1 27 100 -ps 2 73 100 $gMainPane;\"\n\t\t\t\t-removeAllPanels\n\t\t\t\t-ap false\n\t\t\t\t\t(localizedPanelLabel(\"Outliner\")) \n\t\t\t\t\t\"outlinerPanel\"\n"
		+ "\t\t\t\t\t\"$panelName = `outlinerPanel -unParent -l (localizedPanelLabel(\\\"Outliner\\\")) -mbv $menusOkayInPanels `;\\n$editorName = $panelName;\\noutlinerEditor -e \\n    -docTag \\\"isolOutln_fromSeln\\\" \\n    -showShapes 1\\n    -showReferenceNodes 1\\n    -showReferenceMembers 1\\n    -showAttributes 0\\n    -showConnected 0\\n    -showAnimCurvesOnly 0\\n    -showMuteInfo 0\\n    -organizeByLayer 1\\n    -showAnimLayerWeight 1\\n    -autoExpandLayers 1\\n    -autoExpand 0\\n    -showDagOnly 1\\n    -showAssets 1\\n    -showContainedOnly 1\\n    -showPublishedAsConnected 0\\n    -showContainerContents 1\\n    -ignoreDagHierarchy 0\\n    -expandConnections 0\\n    -showUpstreamCurves 1\\n    -showUnitlessCurves 1\\n    -showCompounds 1\\n    -showLeafs 1\\n    -showNumericAttrsOnly 0\\n    -highlightActive 1\\n    -autoSelectNewObjects 0\\n    -doNotSelectNewObjects 0\\n    -dropIsParent 1\\n    -transmitFilters 0\\n    -setFilter \\\"defaultSetFilter\\\" \\n    -showSetMembers 1\\n    -allowMultiSelection 1\\n    -alwaysToggleSelect 0\\n    -directSelect 0\\n    -displayMode \\\"DAG\\\" \\n    -expandObjects 0\\n    -setsIgnoreFilters 1\\n    -containersIgnoreFilters 0\\n    -editAttrName 0\\n    -showAttrValues 0\\n    -highlightSecondary 0\\n    -showUVAttrsOnly 0\\n    -showTextureNodesOnly 0\\n    -attrAlphaOrder \\\"default\\\" \\n    -animLayerFilterOptions \\\"allAffecting\\\" \\n    -sortOrder \\\"none\\\" \\n    -longNames 0\\n    -niceNames 1\\n    -showNamespace 1\\n    -showPinIcons 0\\n    -mapMotionTrails 0\\n    -ignoreHiddenAttribute 0\\n    -ignoreOutlinerColor 0\\n    $editorName\"\n"
		+ "\t\t\t\t\t\"outlinerPanel -edit -l (localizedPanelLabel(\\\"Outliner\\\")) -mbv $menusOkayInPanels  $panelName;\\n$editorName = $panelName;\\noutlinerEditor -e \\n    -docTag \\\"isolOutln_fromSeln\\\" \\n    -showShapes 1\\n    -showReferenceNodes 1\\n    -showReferenceMembers 1\\n    -showAttributes 0\\n    -showConnected 0\\n    -showAnimCurvesOnly 0\\n    -showMuteInfo 0\\n    -organizeByLayer 1\\n    -showAnimLayerWeight 1\\n    -autoExpandLayers 1\\n    -autoExpand 0\\n    -showDagOnly 1\\n    -showAssets 1\\n    -showContainedOnly 1\\n    -showPublishedAsConnected 0\\n    -showContainerContents 1\\n    -ignoreDagHierarchy 0\\n    -expandConnections 0\\n    -showUpstreamCurves 1\\n    -showUnitlessCurves 1\\n    -showCompounds 1\\n    -showLeafs 1\\n    -showNumericAttrsOnly 0\\n    -highlightActive 1\\n    -autoSelectNewObjects 0\\n    -doNotSelectNewObjects 0\\n    -dropIsParent 1\\n    -transmitFilters 0\\n    -setFilter \\\"defaultSetFilter\\\" \\n    -showSetMembers 1\\n    -allowMultiSelection 1\\n    -alwaysToggleSelect 0\\n    -directSelect 0\\n    -displayMode \\\"DAG\\\" \\n    -expandObjects 0\\n    -setsIgnoreFilters 1\\n    -containersIgnoreFilters 0\\n    -editAttrName 0\\n    -showAttrValues 0\\n    -highlightSecondary 0\\n    -showUVAttrsOnly 0\\n    -showTextureNodesOnly 0\\n    -attrAlphaOrder \\\"default\\\" \\n    -animLayerFilterOptions \\\"allAffecting\\\" \\n    -sortOrder \\\"none\\\" \\n    -longNames 0\\n    -niceNames 1\\n    -showNamespace 1\\n    -showPinIcons 0\\n    -mapMotionTrails 0\\n    -ignoreHiddenAttribute 0\\n    -ignoreOutlinerColor 0\\n    $editorName\"\n"
		+ "\t\t\t\t-ap false\n\t\t\t\t\t(localizedPanelLabel(\"Persp View\")) \n\t\t\t\t\t\"modelPanel\"\n"
		+ "\t\t\t\t\t\"$panelName = `modelPanel -unParent -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels `;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"default\\\" \\n    -displayAppearance \\\"smoothShaded\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 0\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 0\\n    -activeComponentsXray 0\\n    -displayTextures 0\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 16384\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"base_OpenGL_Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 1\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -greasePencils 1\\n    -shadows 0\\n    -captureSequenceNumber -1\\n    -width 1479\\n    -height 1068\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName\"\n"
		+ "\t\t\t\t\t\"modelPanel -edit -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels  $panelName;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"default\\\" \\n    -displayAppearance \\\"smoothShaded\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 0\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 0\\n    -activeComponentsXray 0\\n    -displayTextures 0\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 16384\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"base_OpenGL_Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 1\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -greasePencils 1\\n    -shadows 0\\n    -captureSequenceNumber -1\\n    -width 1479\\n    -height 1068\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName\"\n"
		+ "\t\t\t\t$configName;\n\n            setNamedPanelLayout (localizedPanelLabel(\"Current Layout\"));\n        }\n\n        panelHistory -e -clear mainPanelHistory;\n        setFocus `paneLayout -q -p1 $gMainPane`;\n        sceneUIReplacement -deleteRemaining;\n        sceneUIReplacement -clear;\n\t}\n\n\ngrid -spacing 5 -size 12 -divisions 5 -displayAxes yes -displayGridLines yes -displayDivisionLines yes -displayPerspectiveLabels no -displayOrthographicLabels no -displayAxesBold yes -perspectiveLabelPosition axis -orthographicLabelPosition edge;\nviewManip -drawCompass 0 -compassAngle 0 -frontParameters \"\" -homeParameters \"\" -selectionLockParameters \"\";\n}\n");
	setAttr ".st" 3;
createNode script -n "sceneConfigurationScriptNode";
	rename -uid "6D105860-0000-7362-5717-EE0400000250";
	setAttr ".b" -type "string" "playbackOptions -min 1 -max 120 -ast 1 -aet 200 ";
	setAttr ".st" 6;
createNode polyCube -n "polyCube1";
	rename -uid "1A9428C0-0000-0FD8-5846-1F700000025C";
	setAttr ".cuv" 4;
createNode transformGeometry -n "transformGeometry1";
	rename -uid "1A9428C0-0000-0FD8-5846-1FA700000269";
	setAttr ".txf" -type "matrix" 5 0 0 0 0 5 0 0
		 0 0 5 0 5 -10 2.5 1;
createNode transformGeometry -n "transformGeometry2";
	rename -uid "513E48C0-0000-1298-5847-624800000267";
	setAttr ".txf" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -5 0 0 1;
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
	setAttr -s 2 ".st";
select -ne :renderGlobalsList1;
select -ne :defaultShaderList1;
	setAttr -s 4 ".s";
select -ne :postProcessList1;
	setAttr -s 2 ".p";
select -ne :defaultRenderingList1;
select -ne :initialShadingGroup;
	setAttr -s 4 ".dsm";
	setAttr ".ro" yes;
select -ne :initialParticleSE;
	setAttr ".ro" yes;
select -ne :defaultRenderGlobals;
	setAttr ".ren" -type "string" "px_render";
	setAttr ".outf" 3;
select -ne :defaultResolution;
	setAttr ".pa" 1;
select -ne :hardwareRenderGlobals;
	setAttr ".ctrs" 256;
	setAttr ".btrs" 512;
connectAttr "transformGeometry2.og" "AllTypesCubeShape.i";
relationship "link" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
connectAttr "layerManager.dli[0]" "defaultLayer.id";
connectAttr "renderLayerManager.rlmi[0]" "defaultRenderLayer.rlid";
connectAttr "polyCube1.out" "transformGeometry1.ig";
connectAttr "transformGeometry1.og" "transformGeometry2.ig";
connectAttr "defaultRenderLayer.msg" ":defaultRenderingList1.r" -na;
connectAttr "CubeShape.iog" ":initialShadingGroup.dsm" -na;
connectAttr "CubeTypedAttrsShape.iog" ":initialShadingGroup.dsm" -na;
connectAttr "AllTypesCubeShape.iog" ":initialShadingGroup.dsm" -na;
connectAttr "AllTypesCastDoubleToFloatCubeShape.iog" ":initialShadingGroup.dsm" 
		-na;
// End of UserExportedAttributesTest.ma
