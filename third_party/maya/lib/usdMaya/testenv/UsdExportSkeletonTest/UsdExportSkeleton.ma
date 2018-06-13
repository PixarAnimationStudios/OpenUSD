//Maya ASCII 2018ff08 scene
//Name: UsdExportSkeleton.ma
//Last modified: Fri, Jun 08, 2018 04:31:15 PM
//Codeset: UTF-8
requires maya "2018ff08";
requires -nodeType "px_renderGlobals" "px_render" "1.0";
requires "stereoCamera" "10.0";
requires "stereoCamera" "10.0";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2018";
fileInfo "version" "2018";
fileInfo "cutIdentifier" "201804062215-77a9351045";
fileInfo "osv" "Linux 3.10.0-693.2.2.el7.x86_64 #1 SMP Sat Sep 9 03:55:24 EDT 2017 x86_64";
createNode transform -s -n "persp";
	rename -uid "774FC900-0000-4580-5B1B-0DD80000034D";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 4.6361394867374148 -28.582411177346028 94.054970877479533 ;
	setAttr ".r" -type "double3" 73.282924915357441 6.3611093629270351e-15 32.79491399474604 ;
createNode camera -s -n "perspShape" -p "persp";
	rename -uid "774FC900-0000-4580-5B1B-0DD80000034E";
	setAttr -k off ".v" no;
	setAttr ".fl" 34.999999999999993;
	setAttr ".coi" 62.297566157215243;
	setAttr ".imn" -type "string" "persp";
	setAttr ".den" -type "string" "persp_depth";
	setAttr ".man" -type "string" "persp_mask";
	setAttr ".hc" -type "string" "viewSet -p %camera";
createNode transform -s -n "top";
	rename -uid "774FC900-0000-4580-5B1B-0DD80000034F";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 0 1000.1 ;
createNode camera -s -n "topShape" -p "top";
	rename -uid "774FC900-0000-4580-5B1B-0DD800000350";
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
	rename -uid "774FC900-0000-4580-5B1B-0DD800000351";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 -1000.1 0 ;
	setAttr ".r" -type "double3" 89.999999999999986 0 0 ;
createNode camera -s -n "frontShape" -p "front";
	rename -uid "774FC900-0000-4580-5B1B-0DD800000352";
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
	rename -uid "774FC900-0000-4580-5B1B-0DD800000353";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 1000.1 0 0 ;
	setAttr ".r" -type "double3" 90 1.2722218725854067e-14 89.999999999999986 ;
createNode camera -s -n "sideShape" -p "side";
	rename -uid "774FC900-0000-4580-5B1B-0DD800000354";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "side";
	setAttr ".den" -type "string" "side_depth";
	setAttr ".man" -type "string" "side_mask";
	setAttr ".hc" -type "string" "viewSet -s %camera";
	setAttr ".o" yes;
createNode transform -n "SkelChar";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000362";
	setAttr ".s" -type "double3" 0.92000000000588966 0.91999999999850046 0.92000000000021631 ;
createNode joint -n "Hips" -p "SkelChar";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000036A";
	setAttr ".t" -type "double3" 9.5965901891759131e-11 2.2584633827209464 101.63701629638675 ;
	setAttr -av ".tx";
	setAttr -av ".ty";
	setAttr -av ".tz";
	setAttr ".jo" -type "double3" -7.9513867036587919e-16 0 4.2241741863187335e-16 ;
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 2.2584633827209473 101.63701629638672 1;
	setAttr ".radi" 0.40638670325279236;
createNode joint -n "Torso" -p "Hips";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000036B";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 2.2584633827209473 101.63701629638672 1;
	setAttr ".radi" 1.8711608648300171;
createNode joint -n "Chest" -p "Torso";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000036C";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 2.2584633827209473 120.34862518310547 1;
	setAttr ".radi" 1.020599365234375;
createNode joint -n "UpChest" -p "Chest";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000036D";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 2.2584633827209473 130.55461883544922 1;
	setAttr ".radi" 0.42463281750679016;
createNode joint -n "Neck" -p "UpChest";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000036E";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 4.3805253505706787 147.77543640136719 1;
	setAttr ".radi" 1.2369636297225952;
createNode joint -n "Head" -p "Neck";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000036F";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 4.3805253505706778 160.14507293701172 1;
	setAttr ".radi" 0.88176339864730835;
createNode joint -n "LEye" -p "Head";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000370";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 3.2256303710847396 0.56876397167332671 0 0 -0.56876397167332671 3.2256303710847396 0 0
		 0 0 3.275390625 0 4.0960006713867188 -6.4838860034942636 173.41873168945312 1;
	setAttr ".radi" 0.88176339864730835;
createNode joint -n "REye" -p "Head";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000371";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" -3.2256300819318429 0.56876561154333516 -3.4959882187710523e-17 0 0.56876561154333516 3.2256300819318429 -3.9959328307050317e-16 0
		 3.4959882187710523e-17 3.9959328307050317e-16 3.275390625 0 -4.0960006713867188 -6.4838860034942636 173.41873168945312 1;
	setAttr ".radi" 0.88176339864730835;
createNode joint -n "LShldr" -p "UpChest";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000372";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1.3435885659607294e-07 0 -0.9999999999999909 0 0 1 0 0
		 0.9999999999999909 0 1.3435885659607294e-07 0 1.9238375425338745 5.0415034294128418 142.83612060546875 1;
	setAttr ".radi" 1.7381423711776733;
createNode joint -n "LArm" -p "LShldr";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000373";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1.3435885659607294e-07 0 -0.9999999999999909 0 0 1 0 0
		 0.9999999999999909 0 1.3435885659607294e-07 0 19.305261492729027 5.0415034294128409 142.83612294081706 1;
	setAttr ".radi" 2.9660220146179199;
createNode joint -n "LElbow" -p "LArm";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000374";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1.3435885659607294e-07 0 -0.9999999999999909 0 0 1 0 0
		 0.9999999999999909 0 1.3435885659607294e-07 0 48.965482592582276 5.04150342941284 142.83612692593039 1;
	setAttr ".radi" 2.5184221267700195;
createNode joint -n "LHand" -p "LElbow";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000375";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1.8052302345804093e-14 0.9999999999999909 -1.3435885659607172e-07 0 -1.3435885659607172e-07 1.3435885659607294e-07 0.99999999999998179 0
		 0.9999999999999909 0 1.3435885659607294e-07 0 74.149703860282244 5.0415034294128356 142.83613030965344 1;
	setAttr ".radi" 0.23839864134788513;
createNode joint -n "RShldr" -p "UpChest";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000038A";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 2.2204460492503131e-16 -8.6595605623549341e-17 -1.0000000000000002 0 8.6595605623549341e-17 1.0000000000000004 -8.6595605623549341e-17 0
		 -1.0000000000000002 8.6595605623549341e-17 2.2204460492503131e-16 0 -1.9238375425338745 5.0415034294128418 142.83612060546875 1;
	setAttr ".radi" 1.7381423711776733;
createNode joint -n "RArm" -p "RShldr";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000038B";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 2.2204460492503131e-16 -8.6595605623549341e-17 -1.0000000000000002 0 8.6595605623549341e-17 1.0000000000000004 -8.6595605623549341e-17 0
		 -1.0000000000000002 8.6595605623549341e-17 2.2204460492503131e-16 0 -19.305261492729191 5.0415034294128427 142.83612060546881 1;
	setAttr ".radi" 2.9660220146179199;
createNode joint -n "RElbow" -p "RArm";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000038C";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 2.2204460492503131e-16 -8.6595605623549341e-17 -1.0000000000000002 0 8.6595605623549341e-17 1.0000000000000004 -8.6595605623549341e-17 0
		 -1.0000000000000002 8.6595605623549341e-17 2.2204460492503131e-16 0 -48.965482592582717 5.0415034294128445 142.83612060546875 1;
	setAttr ".radi" 2.5184221267700195;
createNode joint -n "RHand" -p "RElbow";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000038D";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 8.6595635457207783e-17 0.99999999999999134 -1.3435885668266856e-07 0 -2.2204459329014272e-16 1.3435885668266859e-07 0.99999999999999112 0
		 -1.0000000000000002 8.6595605623549341e-17 2.2204460492503131e-16 0 -74.149703860282926 5.0415034294128418 142.83612060546864 1;
	setAttr ".radi" 0.23839864134788513;
createNode transform -n "ExtraJoints" -p "RHand";
	rename -uid "774FC900-0000-4580-5B1B-0F5B00000805";
	setAttr ".s" -type "double3" 0.99999998934028389 0.99999997423461684 0.999999996142239 ;
	setAttr ".sh" -type "double3" 2.994370581478931e-08 3.5231482256504765e-08 -5.7205388424880342e-08 ;
createNode joint -n "RHandPropAttach" -p "ExtraJoints";
	rename -uid "774FC900-0000-4580-5B1B-0F3000000801";
	setAttr ".t" -type "double3" -1.4147119031804323e-09 -8.8590468294569291e-11 0 ;
	setAttr ".s" -type "double3" 1.0000000564413349 1.0000000014434824 0.999999982398045 ;
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 8.6595635457207783e-17 0.99999999999999134 -1.3435885668266856e-07 0 -2.2204459329014272e-16 1.3435885668266859e-07 0.99999999999999112 0
		 -1.0000000000000002 8.6595605623549341e-17 2.2204460492503131e-16 0 -74.149703860282926 5.0415034294128418 142.83612060546864 1;
	setAttr ".radi" 0.23839864134788513;
createNode joint -n "LLeg" -p "Hips";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003A2";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1 0 0 0 0 -1 1.2246467991473532e-16 0
		 0 -1.2246467991473532e-16 -1 0 8.6687994003295898 3.6546738147735596 93.179023742675781 1;
	setAttr ".radi" 4.4576716423034668;
createNode joint -n "LKnee" -p "LLeg";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003A3";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1 0 0 0 0 -1 1.2246467991473532e-16 0
		 0 -1.2246467991473532e-16 -1 0 8.6687994003295827 3.6546738147735538 48.602306365966797 1;
	setAttr ".radi" 3.8527603149414062;
createNode joint -n "LFoot" -p "LKnee";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003A4";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1 0 0 0 0 -1 1.2246467991473532e-16 0
		 0 -1.2246467991473532e-16 -1 0 8.6687994003295952 3.6546738147735898 10.074703216552734 1;
	setAttr ".radi" 1.2246989011764526;
createNode joint -n "LToes" -p "LFoot";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003A5";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" 1 0 0 0 0 -1.3435885614054135e-07 0.9999999999999909 0
		 0 -0.9999999999999909 -1.3435885614054135e-07 0 8.6687994003295969 -6.7921750545501407 3.6831393241882333 1;
	setAttr ".radi" 0.80000001192092896;
createNode joint -n "RLeg" -p "Hips";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003A7";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" -1 0 0 0 0 -1 0 0
		 0 0 -1 0 -8.6687994003295898 3.6546738147735596 93.179023742675781 1;
	setAttr ".radi" 4.4576716423034668;
createNode joint -n "RKnee" -p "RLeg";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003A8";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" -1 0 0 0 0 -1 0 0
		 0 0 -1 0 -8.6687994003295827 3.6546738147735591 48.602306365966797 1;
	setAttr ".radi" 3.8527603149414062;
createNode joint -n "RFoot" -p "RKnee";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003A9";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" -1 0 0 0 0 -1 0 0
		 0 0 -1 0 -8.6687994003295952 3.6546738147736 10.074703216552734 1;
	setAttr ".radi" 1.2246989011764526;
createNode joint -n "RToes" -p "RFoot";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003AA";
	setAttr ".ssc" no;
	setAttr ".bps" -type "matrix" -1 0 0 0 0 -1.3435885626300603e-07 0.9999999999999909 0
		 0 -0.9999999999999909 -1.3435885626300603e-07 0 -8.6687994003295969 -6.79217505455013 3.6831393241882324 1;
	setAttr ".radi" 0.80000001192092896;
createNode lightLinker -s -n "lightLinker1";
	rename -uid "2AA81900-0000-61AC-5B1B-102000000278";
	setAttr -s 2 ".lnk";
	setAttr -s 2 ".slnk";
createNode shapeEditorManager -n "shapeEditorManager";
	rename -uid "2AA81900-0000-61AC-5B1B-102000000279";
createNode poseInterpolatorManager -n "poseInterpolatorManager";
	rename -uid "2AA81900-0000-61AC-5B1B-10200000027A";
createNode displayLayerManager -n "layerManager";
	rename -uid "2AA81900-0000-61AC-5B1B-10200000027B";
createNode displayLayer -n "defaultLayer";
	rename -uid "774FC900-0000-4580-5B1B-0DD80000035E";
createNode renderLayerManager -n "renderLayerManager";
	rename -uid "2AA81900-0000-61AC-5B1B-10200000027D";
createNode renderLayer -n "defaultRenderLayer";
	rename -uid "774FC900-0000-4580-5B1B-0DD800000360";
	setAttr ".g" yes;
createNode px_renderGlobals -s -n "px_renderGlobals1";
	rename -uid "774FC900-0000-4580-5B1B-0DD800000361";
	setAttr ".fgh" -type "string" "studio_shading";
createNode animCurveTL -n "SkelChar_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000363";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -8.4976140071971855 2 -13.180064874791341 3 -8.4976140071971855;
createNode animCurveTL -n "SkelChar_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000364";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -3.167572340141966 2 -3.3704203101125003 3 -3.167572340141966;
createNode animCurveTL -n "SkelChar_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000365";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -2.3918947072328507 2 -2.3682889263774314 3 -2.3918947072328507;
createNode animCurveTA -n "SkelChar_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000366";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -5.3192885550162776 2 -5.5100573629262044 3 -5.3192885550162776;
createNode animCurveTA -n "SkelChar_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000367";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.2765626669414045 2 0.97674459097922195 3 1.2765626669414045;
createNode animCurveTA -n "SkelChar_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000368";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -5.7370631743402694 2 3.1080952133423874 3 -5.7370631743402694;
createNode animCurveTL -n "Hips_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003B5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 9.5964125534919731e-11 2 1.4954739668837647e-09 3 9.5964125534919731e-11;
createNode animCurveTA -n "Hips_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003B6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -0.00053405135248259965 2 -0.00053405365405577009 3 -0.00053405135248259965;
createNode animCurveTU -n "Hips_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003B7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999998998897199 2 0.99999998998897244 3 0.99999998998897199;
createNode animCurveTL -n "Hips_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003B8";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 2.2584633827209473 2 2.2584633827209473 3 2.2584633827209473;
createNode animCurveTA -n "Hips_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003B9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -12.27470110014607 2 -12.274701100146217 3 -12.27470110014607;
createNode animCurveTU -n "Hips_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003BA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000002366907 2 1.0000000002366911 3 1.0000000002366907;
createNode animCurveTL -n "Hips_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003BB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 101.63701629638672 2 101.63701629638672 3 101.63701629638672;
createNode animCurveTA -n "Hips_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003BC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -0.0010185830985349898 2 -0.0010185827412264184 3 -0.0010185830985349898;
createNode animCurveTU -n "Hips_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003BD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999998975228122 2 0.99999998975228133 3 0.99999998975228122;
createNode animCurveTL -n "Torso_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003BE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -3.5527136788005009e-15 2 -1.7763568394002505e-15 3 -3.5527136788005009e-15;
createNode animCurveTA -n "Torso_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003BF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -3.4572418322564431 2 -3.4572416130018429 3 -3.4572418322564431;
createNode animCurveTU -n "Torso_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003C0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999999511344495 2 0.99999999501818615 3 0.99999999511344495;
createNode animCurveTL -n "Torso_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003C1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0 2 3.0531133177191805e-15 3 0;
createNode animCurveTA -n "Torso_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003C2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 13.453725729412941 2 13.453725729524496 3 13.453725729412941;
createNode animCurveTU -n "Torso_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003C3";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.9999999983370742 2 0.99999999819910979 3 0.9999999983370742;
createNode animCurveTL -n "Torso_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003C4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.4210854715202004e-14 2 0 3 1.4210854715202004e-14;
createNode animCurveTA -n "Torso_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003C5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -0.096094133168423648 2 -0.096094120610438746 3 -0.096094133168423648;
createNode animCurveTU -n "Torso_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003C6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000172515142 2 1.000000017030789 3 1.0000000172515142;
createNode animCurveTL -n "Chest_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003C7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -7.8213879817212728e-10 2 -7.3980288561870111e-10 3 -7.8213879817212728e-10;
createNode animCurveTA -n "Chest_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003C8";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 9.7625403889831279 2 9.762540389238815 3 9.7625403889831279;
createNode animCurveTU -n "Chest_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003C9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999998873917051 2 0.99999998879159968 3 0.99999998873917051;
createNode animCurveTL -n "Chest_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003CA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 9.759117958196839e-10 2 8.9943519299140462e-10 3 9.759117958196839e-10;
createNode animCurveTA -n "Chest_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003CB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.5873227308843314 2 1.5873228379226489 3 1.5873227308843314;
createNode animCurveTU -n "Chest_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003CC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999997492929238 2 0.99999997492815251 3 0.99999997492929238;
createNode animCurveTL -n "Chest_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003CD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 18.792453765869141 2 18.792453765869141 3 18.792453765869141;
createNode animCurveTA -n "Chest_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003CE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.27263698766995742 2 0.27263699014455989 3 0.27263698766995742;
createNode animCurveTU -n "Chest_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003CF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000054203138 2 1.0000000054729734 3 1.0000000054203138;
createNode animCurveTL -n "UpChest_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003D0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.1121770171484968e-10 2 5.9237947880319552e-11 3 1.1121770171484968e-10;
createNode animCurveTA -n "UpChest_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003D1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 8.8035665444735773 2 8.8035665439087545 3 8.8035665444735773;
createNode animCurveTU -n "UpChest_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003D2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.000000006646425 2 1.0000000066326808 3 1.000000006646425;
createNode animCurveTL -n "UpChest_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003D3";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.3258727449283469e-10 2 5.3976378922016011e-11 3 1.3258727449283469e-10;
createNode animCurveTA -n "UpChest_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003D4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -2.4240119111545222 2 -2.4240119111531184 3 -2.4240119111545222;
createNode animCurveTU -n "UpChest_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003D5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999997277679409 2 0.99999997277995567 3 0.99999997277679409;
createNode animCurveTL -n "UpChest_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003D6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 10.223960876464844 2 10.223960876464844 3 10.223960876464844;
createNode animCurveTA -n "UpChest_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003D7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.13958333485424282 2 0.13958334820636462 3 0.13958333485424282;
createNode animCurveTU -n "UpChest_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003D8";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000189295872 2 1.0000000189413032 3 1.0000000189295872;
createNode animCurveTL -n "Neck_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003D9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -5.3880633288372337e-10 2 -2.2122570442206779e-10 3 -5.3880633288372337e-10;
createNode animCurveTA -n "Neck_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003DA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 11.501721870735199 2 12.198031860061178 3 11.501721870735199;
createNode animCurveTU -n "Neck_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003DB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000138180707 2 1.0000000178377864 3 1.0000000138180707;
createNode animCurveTL -n "Neck_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003DC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 2.1220619678497314 2 2.1220619678497314 3 2.1220619678497314;
createNode animCurveTA -n "Neck_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003DD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -2.7049001747316694 2 -2.1987617341475802 3 -2.7049001747316694;
createNode animCurveTU -n "Neck_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003DE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000035413696 2 0.99999999206702106 3 1.0000000035413696;
createNode animCurveTL -n "Neck_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003DF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 17.220817565917969 2 17.220817565917969 3 17.220817565917969;
createNode animCurveTA -n "Neck_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003E0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 13.998443841452902 2 14.604697912790758 3 13.998443841452902;
createNode animCurveTU -n "Neck_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003E1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000108936307 2 1.0000000034025052 3 1.0000000108936307;
createNode animCurveTL -n "Head_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003E2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 4.106146533899846e-10 2 5.4531090754039724e-10 3 4.106146533899846e-10;
createNode animCurveTA -n "Head_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003E3";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -12.220314625482075 2 -14.396499747961519 3 -12.220314625482075;
createNode animCurveTU -n "Head_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003E4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000276760586 2 1.000000024499252 3 1.0000000276760586;
createNode animCurveTL -n "Head_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003E5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 9.4699004193898872e-10 2 1.1958292134295334e-09 3 9.4699004193898872e-10;
createNode animCurveTA -n "Head_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003E6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -6.8766314870876357 2 5.8648677914276615 3 -6.8766314870876357;
createNode animCurveTU -n "Head_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003E7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999996876966413 2 0.99999999289715624 3 0.99999996876966413;
createNode animCurveTL -n "Head_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003E8";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 12.369636535644531 2 12.474614143371582 3 12.369636535644531;
createNode animCurveTA -n "Head_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003E9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -33.320960079119935 2 -1.3459532327407817 3 -33.320960079119935;
createNode animCurveTU -n "Head_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003EA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999998540505397 2 1.0000000164818563 3 0.99999998540505397;
createNode animCurveTL -n "LEye_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003EB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 4.0960006713867188 2 4.0960893630981445 3 4.0960006713867188;
createNode animCurveTA -n "LEye_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003EC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 3.2506914706275061 2 1.2560139279146125 3 3.2506914706275061;
createNode animCurveTU -n "LEye_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003ED";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 3.4394531217774666 2 3.4394532189225808 3 3.4394531217774666;
createNode animCurveTL -n "LEye_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003EE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -10.700614929199219 2 -10.700430870056152 3 -10.700614929199219;
createNode animCurveTA -n "LEye_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003EF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 4.180426740051641e-08 2 -3.0294096539913467e-09 3 4.180426740051641e-08;
createNode animCurveTU -n "LEye_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003F0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 3.4394532688218939 2 3.4394530706722488 3 3.4394532688218939;
createNode animCurveTL -n "LEye_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003F1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 13.273658752441406 2 13.273593902587891 3 13.273658752441406;
createNode animCurveTA -n "LEye_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003F2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 4.4255858795926111 2 33.545892156039208 3 4.4255858795926111;
createNode animCurveTU -n "LEye_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003F3";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 3.4394530800816709 2 3.4394531572354929 3 3.4394530800816709;
createNode animCurveTL -n "REye_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003F4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -4.0960006713867188 2 -4.095667839050293 3 -4.0960006713867188;
createNode animCurveTA -n "REye_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003F5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -176.09930696861994 2 -177.8628161318903 3 -176.09930696861994;
createNode animCurveTU -n "REye_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003F6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -3.4394532348051974 2 -3.4394531228821714 3 -3.4394532348051974;
createNode animCurveTL -n "REye_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003F7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -10.700614929199219 2 -10.70130443572998 3 -10.700614929199219;
createNode animCurveTA -n "REye_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003F8";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 3.9509212240019492e-08 2 2.9364908646513358e-08 3 3.9509212240019492e-08;
createNode animCurveTU -n "REye_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003F9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -3.4394530482808912 2 -3.4394530793473428 3 -3.4394530482808912;
createNode animCurveTL -n "REye_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003FA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 13.273658752441406 2 13.27390193939209 3 13.273658752441406;
createNode animCurveTA -n "REye_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003FB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -26.71131289364823 2 2.4171838225978117 3 -26.71131289364823;
createNode animCurveTU -n "REye_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003FC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -3.4394530608964482 2 -3.439452831567531 3 -3.4394530608964482;
createNode animCurveTL -n "LShldr_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003FD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 2.0826797485351562 2 2.0826797485351562 3 2.0826797485351562;
createNode animCurveTA -n "LShldr_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003FE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -179.99999976900727 2 -179.99999980922135 3 -179.99999974906228;
createNode animCurveTU -n "LShldr_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DE9000003FF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000315571085 2 1.0000000315571085 3 1.0000000315571085;
createNode animCurveTL -n "LShldr_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000400";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 2.3618929386138916 2 2.3618929386138916 3 2.3618929386138916;
createNode animCurveTA -n "LShldr_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000401";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 80.499997995824245 2 80.499997995824245 3 80.499997995824245;
createNode animCurveTU -n "LShldr_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000402";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1 2 1 3 1;
createNode animCurveTL -n "LShldr_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000403";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 14.671091079711914 2 15.096872329711914 3 14.671091079711914;
createNode animCurveTA -n "LShldr_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000404";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -179.99999977919023 2 -179.99999981946405 3 -179.99999975314967;
createNode animCurveTU -n "LShldr_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000405";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000315571085 2 1.0000000315571085 3 1.0000000315571085;
createNode animCurveTL -n "LArm_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000406";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1.2137775229348335e-09 2 -1.2137775229348335e-09 3 -1.2137775229348335e-09;
createNode animCurveTA -n "LArm_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000407";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 13.265305903182057 2 13.859910777883986 3 13.265305903182057;
createNode animCurveTU -n "LArm_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000408";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000512317915 2 0.99999996920928125 3 1.0000000512317915;
createNode animCurveTL -n "LArm_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000409";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -3.2038371955422917e-11 2 -3.2045477382780518e-11 3 -3.2031266528065316e-11;
createNode animCurveTA -n "LArm_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000040A";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 66.148830088038096 2 66.206538281840466 3 66.148830088038096;
createNode animCurveTU -n "LArm_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000040B";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000116302732 2 1.0000000181250424 3 1.0000000116302732;
createNode animCurveTL -n "LArm_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000040C";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 17.381423950195312 2 17.381423950195312 3 17.381423950195312;
createNode animCurveTA -n "LArm_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000040D";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 36.256841194794347 2 36.456363963987791 3 36.256841194794347;
createNode animCurveTU -n "LArm_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000040E";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999998910714516 2 0.99999998631089093 3 0.99999998910714516;
createNode animCurveTL -n "LElbow_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000040F";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 7.6132522508487455e-10 2 1.000088900582341e-11 3 -4.4875392291032767e-10;
createNode animCurveTA -n "LElbow_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000410";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 84.413514390139753 2 83.102747511558121 3 84.413514390139753;
createNode animCurveTU -n "LElbow_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000411";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1 2 1 3 1;
createNode animCurveTL -n "LElbow_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000412";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1.360277224193851e-09 2 3.7336178593250224e-10 3 5.6871840570238419e-11;
createNode animCurveTA -n "LElbow_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000413";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 3.2659147343360329e-10 2 3.2131399884124192e-09 3 -6.7239944892805582e-10;
createNode animCurveTU -n "LElbow_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000414";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000147165231 2 1.0000000003323812 3 1.0000000147165231;
createNode animCurveTL -n "LElbow_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000415";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 30.333671569824219 2 30.320154190063477 3 30.333671569824219;
createNode animCurveTA -n "LElbow_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000416";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -6.6913457625053295e-11 2 5.929965045020369e-09 3 2.9347238479522147e-09;
createNode animCurveTU -n "LElbow_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000417";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000147165231 2 1.0000000003323812 3 1.0000000147165231;
createNode animCurveTL -n "LHand_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000418";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0113900827946054e-09 2 8.8986240598387667e-10 3 1.1357386142663017e-09;
createNode animCurveTA -n "LHand_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000419";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 9.4048014317523432 2 9.0280162909249473 3 9.4048014317523432;
createNode animCurveTU -n "LHand_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000041A";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999998949677482 2 1.0000000368328097 3 0.99999998949677482;
createNode animCurveTL -n "LHand_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000041B";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -2.0036452497151913e-09 2 -3.489404321044276e-09 3 -2.7350779419066384e-09;
createNode animCurveTA -n "LHand_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000041C";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -13.734152351238938 2 -14.558416577035784 3 -13.734152351238938;
createNode animCurveTU -n "LHand_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000041D";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000120038723 2 1.0000000642367337 3 1.0000000120038723;
createNode animCurveTL -n "LHand_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000041E";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 25.75604248046875 2 25.744564056396484 3 25.75604248046875;
createNode animCurveTA -n "LHand_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE90000041F";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 109.20888964176878 2 109.36848686163717 3 109.20888964176878;
createNode animCurveTU -n "LHand_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DE900000420";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999996735470298 2 1.0000000295733837 3 0.99999996735470298;
createNode animCurveTL -n "RShldr_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004D5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1.8617918491363525 2 -1.8617918491363525 3 -1.8617918491363525;
createNode animCurveTA -n "RShldr_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004D6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1.0639719820522221e-07 2 -1.7115830012485515e-07 3 -1.0639719820522221e-07;
createNode animCurveTU -n "RShldr_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004D7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1.000000081324149 2 -1.000000081324149 3 -1.000000081324149;
createNode animCurveTL -n "RShldr_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004D8";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 2.7839109897613525 2 2.7839109897613525 3 2.7839109897613525;
createNode animCurveTA -n "RShldr_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004D9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -80.500001364082735 2 -80.500001364082735 3 -80.500001364082735;
createNode animCurveTU -n "RShldr_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004DA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1 2 -1 3 -1;
createNode animCurveTL -n "RShldr_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004DB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 15.962133407592773 2 16.307249069213867 3 15.962133407592773;
createNode animCurveTA -n "RShldr_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004DC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -179.99999990503372 2 -179.99999983989611 3 -179.99999990503372;
createNode animCurveTU -n "RShldr_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004DD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -0.99999996211485942 2 -0.99999996211485942 3 -0.99999996211485942;
createNode animCurveTL -n "RArm_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004DE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1.2138343663536943e-09 2 -1.2138059446442639e-09 3 -1.2138343663536943e-09;
createNode animCurveTA -n "RArm_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004DF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 6.5189066507482964 2 7.0352016793738761 3 6.5189066507482964;
createNode animCurveTU -n "RArm_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004E0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000249810348 2 0.99999997926901929 3 1.0000000249810348;
createNode animCurveTL -n "RArm_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004E1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 3.2017055673350114e-11 2 3.2024161100707715e-11 3 3.2017055673350114e-11;
createNode animCurveTA -n "RArm_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004E2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 66.891138763427875 2 66.947897880034972 3 66.891138763427875;
createNode animCurveTU -n "RArm_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004E3";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000020425719 2 0.99999998617432051 3 1.0000000020425719;
createNode animCurveTL -n "RArm_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004E4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 17.381423950195312 2 17.381423950195312 3 17.381423950195312;
createNode animCurveTA -n "RArm_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004E5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 28.958808130848272 2 29.129375415268832 3 28.958808130848272;
createNode animCurveTU -n "RArm_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004E6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999995951713616 2 1.0000000088873724 3 0.99999995951713616;
createNode animCurveTL -n "RElbow_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004E7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 4.808313747162174e-10 2 2.8222046921655419e-10 3 4.808313747162174e-10;
createNode animCurveTA -n "RElbow_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004E8";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 82.095369097709479 2 80.9922132608026 3 82.095369097709479;
createNode animCurveTU -n "RElbow_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004E9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1 2 1 3 1;
createNode animCurveTL -n "RElbow_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004EA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1.7617338698983076e-09 2 -1.8066970142172067e-09 3 -1.7621459846850485e-09;
createNode animCurveTA -n "RElbow_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004EB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -7.1022947430674776e-10 2 2.900817021758661e-10 3 -1.6233252743645089e-09;
createNode animCurveTU -n "RElbow_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004EC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000129776527 2 1.0000000728763476 3 1.0000000129776527;
createNode animCurveTL -n "RElbow_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004ED";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 30.082653045654297 2 30.069862365722656 3 30.082653045654297;
createNode animCurveTA -n "RElbow_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004EE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1.3295232357693667e-09 2 -3.7438006336721641e-09 3 -1.4804807348305788e-09;
createNode animCurveTU -n "RElbow_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004EF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000129776527 2 1.0000000728763476 3 1.0000000129776527;
createNode animCurveTL -n "RHand_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004F0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1.4147119031804323e-09 2 -2.1884574152863934e-09 3 -1.9798989114860888e-09;
createNode animCurveTA -n "RHand_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004F1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.016236480720247717 2 -0.51963881952695368 3 0.016236480720247717;
createNode animCurveTU -n "RHand_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004F2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000564413349 2 0.99999998932725431 3 1.0000000564413349;
createNode animCurveTL -n "RHand_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004F3";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -8.8590468294569291e-11 2 -1.616911049495684e-10 3 -1.2141754268668592e-10;
createNode animCurveTA -n "RHand_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004F4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -32.571757423506597 2 -33.167860163498958 3 -32.571757423506597;
createNode animCurveTU -n "RHand_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004F5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000014434824 2 1.0000000346019118 3 1.0000000014434824;
createNode animCurveTL -n "RHand_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004F6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 25.542905807495117 2 25.53204345703125 3 25.542905807495117;
createNode animCurveTA -n "RHand_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004F7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 121.62020324963169 2 121.965812917425 3 121.62020324963169;
createNode animCurveTU -n "RHand_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000004F8";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.999999982398045 2 1.0000000027951692 3 0.999999982398045;
createNode animCurveTL -n "LLeg_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005AD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 8.6687994003295898 2 8.6687994003295898 3 8.6687994003295898;
createNode animCurveTA -n "LLeg_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005AE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -172.23862363113486 2 -173.62044812273257 3 -172.23862363113486;
createNode animCurveTU -n "LLeg_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005AF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000247001422 2 1.000000006668079 3 1.0000000247001422;
createNode animCurveTL -n "LLeg_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005B0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.3962104320526123 2 1.3962104320526123 3 1.3962104320526123;
createNode animCurveTA -n "LLeg_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005B1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 13.2124025956337 2 7.6258252890400673 3 13.2124025956337;
createNode animCurveTU -n "LLeg_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005B2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000995617371 2 1.0000000363602135 3 1.0000000995617371;
createNode animCurveTL -n "LLeg_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005B3";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -8.4579925537109375 2 -8.4579925537109375 3 -8.4579925537109375;
createNode animCurveTA -n "LLeg_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005B4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.18071860389196415 2 -0.77022586239143087 3 0.18071860389196415;
createNode animCurveTU -n "LLeg_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005B5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000001297712933 2 1.0000000223455425 3 1.0000001297712933;
createNode animCurveTL -n "LKnee_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005B6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -2.3611101962472958e-10 2 7.4319661536037529e-10 3 -2.3611101962472958e-10;
createNode animCurveTA -n "LKnee_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005B7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 6.1218925608778445 2 6.1228356445628505 3 6.1218925608778445;
createNode animCurveTU -n "LKnee_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005B8";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1 2 1 3 1;
createNode animCurveTL -n "LKnee_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005B9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -2.9524649391987623e-10 2 7.0542682806262746e-11 3 -2.9524649391987623e-10;
createNode animCurveTA -n "LKnee_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005BA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 3.6938276813723587e-09 2 5.1182384506940197e-10 3 3.6938276813723587e-09;
createNode animCurveTU -n "LKnee_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005BB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999999765619818 2 1.0000000247619896 3 0.99999999765619818;
createNode animCurveTL -n "LKnee_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005BC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 44.167129516601562 2 44.167072296142578 3 44.167129516601562;
createNode animCurveTA -n "LKnee_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005BD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 5.3122646734530341e-08 2 5.3575487900091006e-08 3 5.3122646734530341e-08;
createNode animCurveTU -n "LKnee_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005BE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999999765619818 2 1.0000000247619896 3 0.99999999765619818;
createNode animCurveTL -n "LFoot_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005BF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -7.7974610102060637e-10 2 -6.1651128646644793e-10 3 -7.7974610102060637e-10;
createNode animCurveTA -n "LFoot_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005C0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -8.6526825420225837 2 -5.759576457689473 3 -8.6526825420225837;
createNode animCurveTU -n "LFoot_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005C1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000000291010425 2 0.99999997448839972 3 1.0000000291010425;
createNode animCurveTL -n "LFoot_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005C2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -7.064215878926916e-10 2 -4.1464076616648526e-10 3 -7.064215878926916e-10;
createNode animCurveTA -n "LFoot_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005C3";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1.6643399404684964 2 -5.6211527611431462 3 -1.6643399404684964;
createNode animCurveTU -n "LFoot_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005C4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999996725402118 2 1.0000000100067725 3 0.99999996725402118;
createNode animCurveTL -n "LFoot_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005C5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 38.173595428466797 2 38.173545837402344 3 38.173595428466797;
createNode animCurveTA -n "LFoot_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005C6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -25.499336805295695 2 -16.501984515280544 3 -25.499336805295695;
createNode animCurveTU -n "LFoot_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005C7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999997983768096 2 0.99999998353774811 3 0.99999997983768096;
createNode animCurveTL -n "LToes_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005C8";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 3.5980107782052073e-12 2 3.4939162674163526e-11 3 3.5980107782052073e-12;
createNode animCurveTA -n "LToes_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005C9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -90.000006830188283 2 -90.000006830188283 3 -90.000006830188283;
createNode animCurveTU -n "LToes_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005CA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1 2 1 3 1;
createNode animCurveTL -n "LToes_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005CB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 10.44684886932373 2 10.44684886932373 3 10.44684886932373;
createNode animCurveTA -n "LToes_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005CC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -5.0589394143931729e-10 2 -2.9677754599428103e-10 3 -5.0589394143931729e-10;
createNode animCurveTU -n "LToes_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005CD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000001192092967 2 1.0000001192092967 3 1.0000001192092967;
createNode animCurveTL -n "LToes_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005CE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 6.391563892364502 2 6.391563892364502 3 6.391563892364502;
createNode animCurveTA -n "LToes_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005CF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -2.1757340337703447e-10 2 -1.0664004842672524e-10 3 -2.1757340337703447e-10;
createNode animCurveTU -n "LToes_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005D0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000001192092967 2 1.0000001192092967 3 1.0000001192092967;
createNode animCurveTL -n "RLeg_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005DA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -8.6687994003295898 2 -8.6687994003295898 3 -8.6687994003295898;
createNode animCurveTA -n "RLeg_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005DB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -14.843473263412033 2 -13.237686834790395 3 -14.843473263412033;
createNode animCurveTU -n "RLeg_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005DC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1.000000012945627 2 -0.99999997483651371 3 -1.000000012945627;
createNode animCurveTL -n "RLeg_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005DD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.3962104320526123 2 1.3962104320526123 3 1.3962104320526123;
createNode animCurveTA -n "RLeg_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005DE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 17.91704540290106 2 13.825653978700267 3 17.91704540290106;
createNode animCurveTU -n "RLeg_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005DF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -0.99999998845329285 2 -1.0000000141029759 3 -0.99999998845329285;
createNode animCurveTL -n "RLeg_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005E0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -8.4579925537109375 2 -8.4579925537109375 3 -8.4579925537109375;
createNode animCurveTA -n "RLeg_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005E1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -21.514784549819627 2 -21.308771958314711 3 -21.514784549819627;
createNode animCurveTU -n "RLeg_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005E2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -0.99999997064977875 2 -0.99999999535000628 3 -0.99999997064977875;
createNode animCurveTL -n "RKnee_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005E3";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0041993903087132e-09 2 1.0724008348006464e-09 3 1.0041993903087132e-09;
createNode animCurveTA -n "RKnee_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005E4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 25.038633960537705 2 28.355451372608972 3 25.038633960537705;
createNode animCurveTU -n "RKnee_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005E5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1 2 1 3 1;
createNode animCurveTL -n "RKnee_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005E6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 8.4052942384005291e-10 2 9.6844487984526495e-10 3 8.4052942384005291e-10;
createNode animCurveTA -n "RKnee_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005E7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -3.434516680516202e-09 2 4.4666298735755759e-10 3 -3.434516680516202e-09;
createNode animCurveTU -n "RKnee_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005E8";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999999386907135 2 1.0000000007528005 3 0.99999999386907135;
createNode animCurveTL -n "RKnee_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005E9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 43.477073669433594 2 43.435253143310547 3 43.477073669433594;
createNode animCurveTA -n "RKnee_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005EA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -1.4864923988634103e-08 2 -1.0928256068098383e-08 3 -1.4864923988634103e-08;
createNode animCurveTU -n "RKnee_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005EB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999999386907135 2 1.0000000007528005 3 0.99999999386907135;
createNode animCurveTL -n "RFoot_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005EC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 4.205702452964033e-10 2 1.5614531889696082e-10 3 4.205702452964033e-10;
createNode animCurveTA -n "RFoot_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005ED";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -8.3732736136562398 2 -13.063615055282231 3 -8.3732736136562398;
createNode animCurveTU -n "RFoot_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005EE";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.000000006317425 2 1.0000000087469181 3 1.000000006317425;
createNode animCurveTL -n "RFoot_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005EF";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -2.4452884161973998e-11 2 -8.6233509222211069e-10 3 -2.4452884161973998e-11;
createNode animCurveTA -n "RFoot_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005F0";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -7.025112116602358 2 -5.4768309593383311 3 -7.025112116602358;
createNode animCurveTU -n "RFoot_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005F1";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.99999999536963158 2 1.0000000202714154 3 0.99999999536963158;
createNode animCurveTL -n "RFoot_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005F2";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 37.577182769775391 2 37.541034698486328 3 37.577182769775391;
createNode animCurveTA -n "RFoot_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005F3";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -7.5192351814914558 2 -16.822786915978526 3 -7.5192351814914558;
createNode animCurveTU -n "RFoot_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005F4";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 0.999999989630458 2 1.0000000168280176 3 0.999999989630458;
createNode animCurveTL -n "RToes_translateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005F5";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 8.1822548736454337e-11 2 -1.1089795748375764e-10 3 8.1822548736454337e-11;
createNode animCurveTA -n "RToes_rotateX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005F6";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -90.000006830188283 2 -90.000006830188283 3 -90.000006830188283;
createNode animCurveTU -n "RToes_scaleX";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005F7";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1 2 1 3 1;
createNode animCurveTL -n "RToes_translateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005F8";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 10.44684886932373 2 10.44684886932373 3 10.44684886932373;
createNode animCurveTA -n "RToes_rotateY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005F9";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 -5.1182943482972947e-10 2 7.9191990580834486e-10 3 -5.1182943482972947e-10;
createNode animCurveTU -n "RToes_scaleY";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005FA";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000001192092967 2 1.0000001192092967 3 1.0000001192092967;
createNode animCurveTL -n "RToes_translateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005FB";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 6.391563892364502 2 6.391563892364502 3 6.391563892364502;
createNode animCurveTA -n "RToes_rotateZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005FC";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 7.8467614617178565e-11 2 6.6810481669838465e-11 3 7.8467614617178565e-11;
createNode animCurveTU -n "RToes_scaleZ";
	rename -uid "774FC900-0000-4580-5B1B-0DEA000005FD";
	setAttr ".tan" 18;
	setAttr ".wgt" no;
	setAttr -s 3 ".ktv[0:2]"  1 1.0000001192092967 2 1.0000001192092967 3 1.0000001192092967;
createNode dagPose -n "bindPose";
	rename -uid "774FC900-0000-4580-5B1B-0DEA00000607";
	setAttr -s 24 ".wm";
	setAttr -s 66 ".xm";
	setAttr ".xm[0]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 2.2584633827209473 101.63701629638672 1;
	setAttr ".xm[1]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 0 1;
	setAttr ".xm[2]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 18.71160888671875 1;
	setAttr ".xm[3]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 10.20599365234375 1;
	setAttr ".xm[4]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 2.1220619678497314 17.220817565917969 1;
	setAttr ".xm[5]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 -8.8817841970012523e-16 12.369636535644531 1;
	setAttr ".xm[6]" -type "matrix" 3.2256303710847396 0.56876397167332671 0 0 -0.56876397167332671 3.2256303710847396 0 0
		 0 0 3.275390625 0 4.0960006713867188 -10.864411354064941 13.273658752441406 1;
	setAttr ".xm[7]" -type "matrix" -3.2256300819318429 0.56876561154333516 -3.4959882187710523e-17 0 0.56876561154333516 3.2256300819318429 -3.9959328307050317e-16 0
		 3.4959882187710523e-17 3.9959328307050317e-16 3.275390625 0 -4.0960006713867188 -10.864411354064941 13.273658752441406 1;
	setAttr ".xm[8]" -type "matrix" 1.3435885659607294e-07 0 -0.9999999999999909 0 0 1 0 0
		 0.9999999999999909 0 1.3435885659607294e-07 0 1.9238375425338745 2.7830400466918945 12.281501770019531 1;
	setAttr ".xm[9]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -5.6843418860808015e-14 -8.8817841970012523e-16 17.381423950195312 1;
	setAttr ".xm[10]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 5.6843418860808015e-14 -8.8817841970012523e-16 29.660221099853516 1;
	setAttr ".xm[11]" -type "matrix" 1.3435885659607294e-07 0.9999999999999909 0 0 -0.9999999999999909 1.3435885659607294e-07 0 0
		 0 0 1 0 1.1368683772161603e-13 -4.4408920985006262e-15 25.184221267700195 1;
	setAttr ".xm[12]" -type "matrix" 0.28631744267572168 0.70472976161616596 0.6491365689221773 0 -0.61279787464555635 0.65550430156105111 -0.44135345865286651 0
		 -0.73654673093397394 -0.27142231619264529 0.61953921540375134 0 -3.5745439529418945 -2.9135732650756836 5.2789177894592285 1;
	setAttr ".xm[13]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 1.7934098650584929e-11 -4.7563730731781106e-11 3.5160152912139893 1;
	setAttr ".xm[14]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 1.2568079910124652e-10 3.4836489248846192e-10 3.3434429168701172 1;
	setAttr ".xm[15]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 -7.1054273576010019e-15 2 1;
	setAttr ".xm[16]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -3.6757771968841553 0.08203125 12.191150665283203 1;
	setAttr ".xm[17]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 4.8494911193847656 1;
	setAttr ".xm[18]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -1.1102230246251565e-15 2.8421709430404007e-14 3.3542518615722656 1;
	setAttr ".xm[19]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 2.2204460492503131e-16 0 2 1;
	setAttr ".xm[20]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -1.1220852136611938 0.3968658447265625 12.418510437011719 1;
	setAttr ".xm[21]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 2.8421709430404007e-14 5.40960693359375 1;
	setAttr ".xm[22]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -1.7763568394002505e-15 2.8421709430404007e-14 3.86602783203125 1;
	setAttr ".xm[23]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 4.4408920985006262e-16 2.8421709430404007e-14 2 1;
	setAttr ".xm[24]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 1.3965789079666138 0.0495758056640625 12.100906372070312 1;
	setAttr ".xm[25]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -8.8817841970012523e-16 -2.8421709430404007e-14 5.0502471923828125 1;
	setAttr ".xm[26]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 2.8421709430404007e-14 3.5308189392089844 1;
	setAttr ".xm[27]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 -2.8421709430404007e-14 2 1;
	setAttr ".xm[28]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 3.761754035949707 -0.4139251708984375 11.303215026855469 1;
	setAttr ".xm[29]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 3.9312591552734375 1;
	setAttr ".xm[30]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -5.3290705182007514e-15 2.8421709430404007e-14 2.9286079406738281 1;
	setAttr ".xm[31]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 2 1;
	setAttr ".xm[32]" -type "matrix" 2.2204460492503131e-16 -8.6595605623549341e-17 -1.0000000000000002 0 8.6595605623549341e-17 1.0000000000000004 -8.6595605623549341e-17 0
		 -1.0000000000000002 8.6595605623549341e-17 2.2204460492503131e-16 0 -1.9238375425338745 2.7830400466918945 12.281501770019531 1;
	setAttr ".xm[33]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -5.6843418860808015e-14 -8.8817841970012523e-16 17.381423950195312 1;
	setAttr ".xm[34]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 5.6843418860808015e-14 -8.8817841970012523e-16 29.660221099853516 1;
	setAttr ".xm[35]" -type "matrix" 1.3435885659607294e-07 0.9999999999999909 0 0 -0.9999999999999909 1.3435885659607294e-07 0 0
		 0 0 1 0 1.1368683772161603e-13 -4.4408920985006262e-15 25.184221267700195 1;
	setAttr ".xm[36]" -type "matrix" 0.28631744267572168 0.70472976161616596 0.6491365689221773 0 -0.61279787464555635 0.65550430156105111 -0.44135345865286651 0
		 -0.73654673093397394 -0.27142231619264529 0.61953921540375134 0 -3.5745439529418945 -2.9135732650756836 5.2789177894592285 1;
	setAttr ".xm[37]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 1.7934098650584929e-11 -4.7563730731781106e-11 3.5160152912139893 1;
	setAttr ".xm[38]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 1.2568079910124652e-10 3.4836489248846192e-10 3.3434429168701172 1;
	setAttr ".xm[39]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 -7.1054273576010019e-15 2 1;
	setAttr ".xm[40]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -3.6757771968841553 0.08203125 12.191150665283203 1;
	setAttr ".xm[41]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 4.8494911193847656 1;
	setAttr ".xm[42]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -1.1102230246251565e-15 2.8421709430404007e-14 3.3542518615722656 1;
	setAttr ".xm[43]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 2.2204460492503131e-16 0 2 1;
	setAttr ".xm[44]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -1.1220852136611938 0.3968658447265625 12.418510437011719 1;
	setAttr ".xm[45]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 2.8421709430404007e-14 5.40960693359375 1;
	setAttr ".xm[46]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -1.7763568394002505e-15 2.8421709430404007e-14 3.86602783203125 1;
	setAttr ".xm[47]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 4.4408920985006262e-16 2.8421709430404007e-14 2 1;
	setAttr ".xm[48]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 1.3965789079666138 0.0495758056640625 12.100906372070312 1;
	setAttr ".xm[49]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -8.8817841970012523e-16 -2.8421709430404007e-14 5.0502471923828125 1;
	setAttr ".xm[50]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 2.8421709430404007e-14 3.5308189392089844 1;
	setAttr ".xm[51]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 -2.8421709430404007e-14 2 1;
	setAttr ".xm[52]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 3.761754035949707 -0.4139251708984375 11.303215026855469 1;
	setAttr ".xm[53]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 3.9312591552734375 1;
	setAttr ".xm[54]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -5.3290705182007514e-15 2.8421709430404007e-14 2.9286079406738281 1;
	setAttr ".xm[55]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 0 0 2 1;
	setAttr ".xm[56]" -type "matrix" 1 0 0 0 0 -1 1.2246467991473532e-16 0
		 0 -1.2246467991473532e-16 -1 0 8.6687994003295898 1.3962104320526123 -8.4579925537109375 1;
	setAttr ".xm[57]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -7.1054273576010019e-15 4.4408920985006262e-16 44.576717376708984 1;
	setAttr ".xm[58]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 1.2434497875801753e-14 -4.0856207306205761e-14 38.527603149414062 1;
	setAttr ".xm[59]" -type "matrix" 1 0 0 0 0 1.3435885626300603e-07 -0.9999999999999909 0
		 0 0.9999999999999909 1.3435885626300603e-07 0 1.7763568394002505e-15 10.44684886932373 6.391563892364502 1;
	setAttr ".xm[60]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -1.7763568394002505e-15 0 8 1;
	setAttr ".xm[61]" -type "matrix" -1 0 0 0 0 -1 0 0
		 0 0 -1 0 -8.6687994003295898 1.3962104320526123 -8.4579925537109375 1;
	setAttr ".xm[62]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -7.1054273576010019e-15 4.4408920985006262e-16 44.576717376708984 1;
	setAttr ".xm[63]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 1.2434497875801753e-14 -4.0856207306205761e-14 38.527603149414062 1;
	setAttr ".xm[64]" -type "matrix" 1 0 0 0 0 1.3435885626300603e-07 -0.9999999999999909 0
		 0 0.9999999999999909 1.3435885626300603e-07 0 1.7763568394002505e-15 10.44684886932373 6.391563892364502 1;
	setAttr ".xm[65]" -type "matrix" 1 0 0 0 0 1 0 0
		 0 0 1 0 -1.7763568394002505e-15 0 8 1;
	setAttr -s 54 ".m";
	setAttr -s 66 ".p";
	setAttr ".bp" yes;
createNode script -n "uiConfigurationScriptNode";
	rename -uid "774FC900-0000-4580-5B1B-0E6F00000609";
	setAttr ".b" -type "string" (
		"// Maya Mel UI Configuration File.\n//\n//  This script is machine generated.  Edit at your own risk.\n//\n//\n\nglobal string $gMainPane;\nif (`paneLayout -exists $gMainPane`) {\n\n\tglobal int $gUseScenePanelConfig;\n\tint    $useSceneConfig = $gUseScenePanelConfig;\n\tint    $nodeEditorPanelVisible = stringArrayContains(\"nodeEditorPanel1\", `getPanel -vis`);\n\tint    $nodeEditorWorkspaceControlOpen = (`workspaceControl -exists nodeEditorPanel1Window` && `workspaceControl -q -visible nodeEditorPanel1Window`);\n\tint    $menusOkayInPanels = `optionVar -q allowMenusInPanels`;\n\tint    $nVisPanes = `paneLayout -q -nvp $gMainPane`;\n\tint    $nPanes = 0;\n\tstring $editorName;\n\tstring $panelName;\n\tstring $itemFilterName;\n\tstring $panelConfig;\n\n\t//\n\t//  get current state of the UI\n\t//\n\tsceneUIReplacement -update $gMainPane;\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Top View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Top View\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"top\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n"
		+ "            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n"
		+ "            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n"
		+ "\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Side View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Side View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"side\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n"
		+ "            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n"
		+ "            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n"
		+ "            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Front View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Front View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"front\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n"
		+ "            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n"
		+ "            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n"
		+ "            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Persp View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Persp View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"persp\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n"
		+ "            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 16384\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"base_OpenGL_Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n"
		+ "            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n"
		+ "            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 742\n            -height 502\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"ToggledOutliner\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\toutlinerPanel -edit -l (localizedPanelLabel(\"ToggledOutliner\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\t$editorName = $panelName;\n        outlinerEditor -e \n            -showShapes 0\n            -showAssignedMaterials 0\n            -showTimeEditor 1\n            -showReferenceNodes 1\n            -showReferenceMembers 1\n            -showAttributes 0\n            -showConnected 0\n            -showAnimCurvesOnly 0\n            -showMuteInfo 0\n            -organizeByLayer 1\n            -organizeByClip 1\n            -showAnimLayerWeight 1\n            -autoExpandLayers 1\n            -autoExpand 0\n            -showDagOnly 0\n            -showAssets 1\n            -showContainedOnly 1\n            -showPublishedAsConnected 0\n            -showParentContainers 0\n            -showContainerContents 1\n            -ignoreDagHierarchy 0\n            -expandConnections 0\n            -showUpstreamCurves 1\n            -showUnitlessCurves 1\n            -showCompounds 1\n            -showLeafs 1\n            -showNumericAttrsOnly 0\n            -highlightActive 1\n            -autoSelectNewObjects 0\n            -doNotSelectNewObjects 0\n            -dropIsParent 1\n"
		+ "            -transmitFilters 0\n            -setFilter \"defaultSetFilter\" \n            -showSetMembers 1\n            -allowMultiSelection 1\n            -alwaysToggleSelect 0\n            -directSelect 0\n            -isSet 0\n            -isSetMember 0\n            -displayMode \"DAG\" \n            -expandObjects 0\n            -setsIgnoreFilters 1\n            -containersIgnoreFilters 0\n            -editAttrName 0\n            -showAttrValues 0\n            -highlightSecondary 0\n            -showUVAttrsOnly 0\n            -showTextureNodesOnly 0\n            -attrAlphaOrder \"default\" \n            -animLayerFilterOptions \"allAffecting\" \n            -sortOrder \"none\" \n            -longNames 0\n            -niceNames 1\n            -showNamespace 1\n            -showPinIcons 0\n            -mapMotionTrails 0\n            -ignoreHiddenAttribute 0\n            -ignoreOutlinerColor 0\n            -renderFilterVisible 0\n            -renderFilterIndex 0\n            -selectionOrder \"chronological\" \n            -expandAttribute 0\n            $editorName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"Outliner\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\toutlinerPanel -edit -l (localizedPanelLabel(\"Outliner\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        outlinerEditor -e \n            -showShapes 0\n            -showAssignedMaterials 0\n            -showTimeEditor 1\n            -showReferenceNodes 0\n            -showReferenceMembers 0\n            -showAttributes 0\n            -showConnected 0\n            -showAnimCurvesOnly 0\n            -showMuteInfo 0\n            -organizeByLayer 1\n            -organizeByClip 1\n            -showAnimLayerWeight 1\n            -autoExpandLayers 1\n            -autoExpand 0\n            -showDagOnly 1\n            -showAssets 1\n            -showContainedOnly 1\n            -showPublishedAsConnected 0\n            -showParentContainers 0\n            -showContainerContents 1\n            -ignoreDagHierarchy 0\n"
		+ "            -expandConnections 0\n            -showUpstreamCurves 1\n            -showUnitlessCurves 1\n            -showCompounds 1\n            -showLeafs 1\n            -showNumericAttrsOnly 0\n            -highlightActive 1\n            -autoSelectNewObjects 0\n            -doNotSelectNewObjects 0\n            -dropIsParent 1\n            -transmitFilters 0\n            -setFilter \"defaultSetFilter\" \n            -showSetMembers 1\n            -allowMultiSelection 1\n            -alwaysToggleSelect 0\n            -directSelect 0\n            -displayMode \"DAG\" \n            -expandObjects 0\n            -setsIgnoreFilters 1\n            -containersIgnoreFilters 0\n            -editAttrName 0\n            -showAttrValues 0\n            -highlightSecondary 0\n            -showUVAttrsOnly 0\n            -showTextureNodesOnly 0\n            -attrAlphaOrder \"default\" \n            -animLayerFilterOptions \"allAffecting\" \n            -sortOrder \"none\" \n            -longNames 0\n            -niceNames 1\n            -showNamespace 1\n            -showPinIcons 0\n"
		+ "            -mapMotionTrails 0\n            -ignoreHiddenAttribute 0\n            -ignoreOutlinerColor 0\n            -renderFilterVisible 0\n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"graphEditor\" (localizedPanelLabel(\"Graph Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Graph Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n            outlinerEditor -e \n                -showShapes 1\n                -showAssignedMaterials 0\n                -showTimeEditor 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n                -organizeByLayer 1\n                -organizeByClip 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n"
		+ "                -autoExpand 1\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showParentContainers 1\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n                -showUpstreamCurves 1\n                -showUnitlessCurves 1\n                -showCompounds 0\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n                -autoSelectNewObjects 1\n                -doNotSelectNewObjects 0\n                -dropIsParent 1\n                -transmitFilters 1\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n"
		+ "                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 1\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                -renderFilterVisible 0\n                $editorName;\n\n\t\t\t$editorName = ($panelName+\"GraphEd\");\n            animCurveEditor -e \n                -displayKeys 1\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 1\n                -displayInfinities 0\n                -displayValues 0\n                -autoFit 1\n                -autoFitTime 0\n                -snapTime \"integer\" \n                -snapValue \"none\" \n                -showResults \"off\" \n"
		+ "                -showBufferCurves \"off\" \n                -smoothness \"fine\" \n                -resultSamples 1\n                -resultScreenSamples 0\n                -resultUpdate \"delayed\" \n                -showUpstreamCurves 1\n                -showCurveNames 0\n                -showActiveCurveNames 0\n                -stackedCurves 0\n                -stackedCurvesMin -1\n                -stackedCurvesMax 1\n                -stackedCurvesSpace 0.2\n                -displayNormalized 0\n                -preSelectionHighlight 0\n                -constrainDrag 0\n                -classicMode 1\n                -valueLinesToggle 1\n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dopeSheetPanel\" (localizedPanelLabel(\"Dope Sheet\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dope Sheet\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n"
		+ "            outlinerEditor -e \n                -showShapes 1\n                -showAssignedMaterials 0\n                -showTimeEditor 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n                -organizeByLayer 1\n                -organizeByClip 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 0\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showParentContainers 1\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n                -showUpstreamCurves 1\n                -showUnitlessCurves 0\n                -showCompounds 1\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n"
		+ "                -autoSelectNewObjects 0\n                -doNotSelectNewObjects 1\n                -dropIsParent 1\n                -transmitFilters 0\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 0\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                -renderFilterVisible 0\n"
		+ "                $editorName;\n\n\t\t\t$editorName = ($panelName+\"DopeSheetEd\");\n            dopeSheetEditor -e \n                -displayKeys 1\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 0\n                -displayInfinities 0\n                -displayValues 0\n                -autoFit 0\n                -autoFitTime 0\n                -snapTime \"integer\" \n                -snapValue \"none\" \n                -outliner \"dopeSheetPanel1OutlineEd\" \n                -showSummary 1\n                -showScene 0\n                -hierarchyBelow 0\n                -showTicks 1\n                -selectionWindow 0 0 0 0 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"timeEditorPanel\" (localizedPanelLabel(\"Time Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Time Editor\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"clipEditorPanel\" (localizedPanelLabel(\"Trax Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Trax Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = clipEditorNameFromPanel($panelName);\n            clipEditor -e \n                -displayKeys 0\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 0\n                -displayInfinities 0\n                -displayValues 0\n                -autoFit 0\n                -autoFitTime 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -initialized 0\n                -manageSequencer 0 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"sequenceEditorPanel\" (localizedPanelLabel(\"Camera Sequencer\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Camera Sequencer\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = sequenceEditorNameFromPanel($panelName);\n            clipEditor -e \n                -displayKeys 0\n                -displayTangents 0\n                -displayActiveKeys 0\n                -displayActiveKeyTangents 0\n                -displayInfinities 0\n                -displayValues 0\n                -autoFit 0\n                -autoFitTime 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -initialized 0\n                -manageSequencer 1 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperGraphPanel\" (localizedPanelLabel(\"Hypergraph Hierarchy\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypergraph Hierarchy\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\n\t\t\t$editorName = ($panelName+\"HyperGraphEd\");\n            hyperGraph -e \n                -graphLayoutStyle \"hierarchicalLayout\" \n                -orientation \"horiz\" \n                -mergeConnections 0\n                -zoom 1\n                -animateTransition 0\n                -showRelationships 1\n                -showShapes 0\n                -showDeformers 0\n                -showExpressions 0\n                -showConstraints 0\n                -showConnectionFromSelected 0\n                -showConnectionToSelected 0\n                -showConstraintLabels 0\n                -showUnderworld 0\n                -showInvisible 0\n                -transitionFrames 1\n                -opaqueContainers 0\n                -freeform 0\n                -imagePosition 0 0 \n                -imageScale 1\n                -imageEnabled 0\n                -graphType \"DAG\" \n                -heatMapDisplay 0\n                -updateSelection 1\n                -updateNodeAdded 1\n                -useDrawOverrideColor 0\n                -limitGraphTraversal -1\n"
		+ "                -range 0 0 \n                -iconSize \"smallIcons\" \n                -showCachedConnections 0\n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperShadePanel\" (localizedPanelLabel(\"Hypershade\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypershade\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"visorPanel\" (localizedPanelLabel(\"Visor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Visor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"nodeEditorPanel\" (localizedPanelLabel(\"Node Editor\")) `;\n\tif ($nodeEditorPanelVisible || $nodeEditorWorkspaceControlOpen) {\n"
		+ "\t\tif (\"\" == $panelName) {\n\t\t\tif ($useSceneConfig) {\n\t\t\t\t$panelName = `scriptedPanel -unParent  -type \"nodeEditorPanel\" -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels `;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -connectNodeOnCreation 0\n                -connectOnDrop 0\n                -copyConnectionsOnPaste 0\n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -nodeTitleMode \"name\" \n                -gridSnap 0\n                -gridVisibility 1\n                -crosshairOnEdgeDragging 0\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n"
		+ "                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n                -useAssets 1\n                -syncedSelection 1\n                -extendToShapes 1\n                -editorMode \"default\" \n                $editorName;\n\t\t\t}\n\t\t} else {\n\t\t\t$label = `panel -q -label $panelName`;\n\t\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -connectNodeOnCreation 0\n                -connectOnDrop 0\n                -copyConnectionsOnPaste 0\n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n"
		+ "                -nodeTitleMode \"name\" \n                -gridSnap 0\n                -gridVisibility 1\n                -crosshairOnEdgeDragging 0\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n                -useAssets 1\n                -syncedSelection 1\n                -extendToShapes 1\n                -editorMode \"default\" \n                $editorName;\n\t\t\tif (!$useSceneConfig) {\n\t\t\t\tpanel -e -l $label $panelName;\n\t\t\t}\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"createNodePanel\" (localizedPanelLabel(\"Create Node\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Create Node\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"polyTexturePlacementPanel\" (localizedPanelLabel(\"UV Editor\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"UV Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"renderWindowPanel\" (localizedPanelLabel(\"Render View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Render View\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"shapePanel\" (localizedPanelLabel(\"Shape Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tshapePanel -edit -l (localizedPanelLabel(\"Shape Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"posePanel\" (localizedPanelLabel(\"Pose Editor\")) `;\n\tif (\"\" != $panelName) {\n"
		+ "\t\t$label = `panel -q -label $panelName`;\n\t\tposePanel -edit -l (localizedPanelLabel(\"Pose Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynRelEdPanel\" (localizedPanelLabel(\"Dynamic Relationships\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dynamic Relationships\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"relationshipPanel\" (localizedPanelLabel(\"Relationship Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Relationship Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"referenceEditorPanel\" (localizedPanelLabel(\"Reference Editor\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Reference Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"componentEditorPanel\" (localizedPanelLabel(\"Component Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Component Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynPaintScriptedPanelType\" (localizedPanelLabel(\"Paint Effects\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Paint Effects\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"scriptEditorPanel\" (localizedPanelLabel(\"Script Editor\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Script Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"profilerPanel\" (localizedPanelLabel(\"Profiler Tool\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Profiler Tool\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"contentBrowserPanel\" (localizedPanelLabel(\"Content Browser\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Content Browser\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"Stereo\" (localizedPanelLabel(\"Stereo\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Stereo\")) -mbv $menusOkayInPanels  $panelName;\nstring $editorName = ($panelName+\"Editor\");\n            stereoCameraView -e \n                -camera \"persp\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"smoothShaded\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 0\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n"
		+ "                -textureDisplay \"modulate\" \n                -textureMaxSize 16384\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 4 4 \n                -bumpResolution 4 4 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 0\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n"
		+ "                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -controllers 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n"
		+ "                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 0\n                -height 0\n                -sceneRenderFilter 0\n                -displayMode \"centerEye\" \n                -viewColor 0 0 0 1 \n                -useCustomBackground 1\n                $editorName;\n            stereoCameraView -e -viewSelected 0 $editorName;\n            stereoCameraView -e \n                -pluginObjects \"gpuCacheDisplayFilter\" 1 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\tif ($useSceneConfig) {\n        string $configName = `getPanel -cwl (localizedPanelLabel(\"Current Layout\"))`;\n        if (\"\" != $configName) {\n\t\t\tpanelConfiguration -edit -label (localizedPanelLabel(\"Current Layout\")) \n\t\t\t\t-userCreated false\n\t\t\t\t-defaultImage \"vacantCell.xP:/\"\n\t\t\t\t-image \"\"\n\t\t\t\t-sc false\n\t\t\t\t-configString \"global string $gMainPane; paneLayout -e -cn \\\"single\\\" -ps 1 100 100 $gMainPane;\"\n\t\t\t\t-removeAllPanels\n"
		+ "\t\t\t\t-ap true\n\t\t\t\t\t(localizedPanelLabel(\"Persp View\")) \n\t\t\t\t\t\"modelPanel\"\n"
		+ "\t\t\t\t\t\"$panelName = `modelPanel -unParent -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels `;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"default\\\" \\n    -displayAppearance \\\"smoothShaded\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 0\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 0\\n    -activeComponentsXray 0\\n    -displayTextures 0\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 16384\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"base_OpenGL_Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -controllers 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 1\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -greasePencils 1\\n    -shadows 0\\n    -captureSequenceNumber -1\\n    -width 742\\n    -height 502\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName;\\nmodelEditor -e \\n    -pluginObjects \\\"gpuCacheDisplayFilter\\\" 1 \\n    $editorName\"\n"
		+ "\t\t\t\t\t\"modelPanel -edit -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels  $panelName;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"default\\\" \\n    -displayAppearance \\\"smoothShaded\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 0\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 0\\n    -activeComponentsXray 0\\n    -displayTextures 0\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 16384\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"base_OpenGL_Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -controllers 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 1\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -greasePencils 1\\n    -shadows 0\\n    -captureSequenceNumber -1\\n    -width 742\\n    -height 502\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName;\\nmodelEditor -e \\n    -pluginObjects \\\"gpuCacheDisplayFilter\\\" 1 \\n    $editorName\"\n"
		+ "\t\t\t\t$configName;\n\n            setNamedPanelLayout (localizedPanelLabel(\"Current Layout\"));\n        }\n\n        panelHistory -e -clear mainPanelHistory;\n        sceneUIReplacement -clear;\n\t}\n\n\ngrid -spacing 5 -size 12 -divisions 5 -displayAxes yes -displayGridLines yes -displayDivisionLines yes -displayPerspectiveLabels no -displayOrthographicLabels no -displayAxesBold yes -perspectiveLabelPosition axis -orthographicLabelPosition edge;\nviewManip -drawCompass 0 -compassAngle 0 -frontParameters \"\" -homeParameters \"\" -selectionLockParameters \"\";\n}\n");
	setAttr ".st" 3;
createNode script -n "sceneConfigurationScriptNode";
	rename -uid "774FC900-0000-4580-5B1B-0E6F0000060A";
	setAttr ".b" -type "string" "playbackOptions -min 1 -max 120 -ast 1 -aet 200 ";
	setAttr ".st" 6;
createNode nodeGraphEditorInfo -n "hyperShadePrimaryNodeEditorSavedTabsInfo";
	rename -uid "774FC900-0000-4580-5B1B-0F6E00000806";
	setAttr ".pee" yes;
	setAttr ".tgi[0].tn" -type "string" "Untitled_1";
	setAttr ".tgi[0].vl" -type "double2" 0.59523807158545394 -31.54761779402935 ;
	setAttr ".tgi[0].vh" -type "double2" 114.88094781599366 -0.59523807158545905 ;
	setAttr -s 261 ".tgi[0].ni";
	setAttr ".tgi[0].ni[0].x" -205.71427917480469;
	setAttr ".tgi[0].ni[0].y" 9477.142578125;
	setAttr ".tgi[0].ni[0].nvs" 1922;
	setAttr ".tgi[0].ni[1].x" -205.71427917480469;
	setAttr ".tgi[0].ni[1].y" 27818.572265625;
	setAttr ".tgi[0].ni[1].nvs" 1922;
	setAttr ".tgi[0].ni[2].x" -205.71427917480469;
	setAttr ".tgi[0].ni[2].y" 1054.2857666015625;
	setAttr ".tgi[0].ni[2].nvs" 1922;
	setAttr ".tgi[0].ni[3].x" -205.71427917480469;
	setAttr ".tgi[0].ni[3].y" 23698.572265625;
	setAttr ".tgi[0].ni[3].nvs" 1922;
	setAttr ".tgi[0].ni[4].x" -205.71427917480469;
	setAttr ".tgi[0].ni[4].y" 6554.28564453125;
	setAttr ".tgi[0].ni[4].nvs" 1922;
	setAttr ".tgi[0].ni[5].x" -205.71427917480469;
	setAttr ".tgi[0].ni[5].y" 27818.572265625;
	setAttr ".tgi[0].ni[5].nvs" 1922;
	setAttr ".tgi[0].ni[6].x" -205.71427917480469;
	setAttr ".tgi[0].ni[6].y" -914.28570556640625;
	setAttr ".tgi[0].ni[6].nvs" 1922;
	setAttr ".tgi[0].ni[7].x" -191.42857360839844;
	setAttr ".tgi[0].ni[7].y" 10518.5712890625;
	setAttr ".tgi[0].ni[7].nvs" 2162;
	setAttr ".tgi[0].ni[8].x" -208.57142639160156;
	setAttr ".tgi[0].ni[8].y" 29258.572265625;
	setAttr ".tgi[0].ni[8].nvs" 1922;
	setAttr ".tgi[0].ni[9].x" -205.71427917480469;
	setAttr ".tgi[0].ni[9].y" 25274.28515625;
	setAttr ".tgi[0].ni[9].nvs" 1922;
	setAttr ".tgi[0].ni[10].x" -205.71427917480469;
	setAttr ".tgi[0].ni[10].y" 2000;
	setAttr ".tgi[0].ni[10].nvs" 1922;
	setAttr ".tgi[0].ni[11].x" -191.42857360839844;
	setAttr ".tgi[0].ni[11].y" 24598.572265625;
	setAttr ".tgi[0].ni[11].nvs" 2162;
	setAttr ".tgi[0].ni[12].x" 137.14285278320312;
	setAttr ".tgi[0].ni[12].y" 10631.4287109375;
	setAttr ".tgi[0].ni[12].nvs" 1922;
	setAttr ".tgi[0].ni[13].x" -205.71427917480469;
	setAttr ".tgi[0].ni[13].y" 27857.142578125;
	setAttr ".tgi[0].ni[13].nvs" 1922;
	setAttr ".tgi[0].ni[14].x" -205.71427917480469;
	setAttr ".tgi[0].ni[14].y" 10060;
	setAttr ".tgi[0].ni[14].nvs" 1922;
	setAttr ".tgi[0].ni[15].x" -34.285713195800781;
	setAttr ".tgi[0].ni[15].y" 57.142856597900391;
	setAttr ".tgi[0].ni[15].nvs" 1923;
	setAttr ".tgi[0].ni[16].x" -205.71427917480469;
	setAttr ".tgi[0].ni[16].y" 27818.572265625;
	setAttr ".tgi[0].ni[16].nvs" 1922;
	setAttr ".tgi[0].ni[17].x" -205.71427917480469;
	setAttr ".tgi[0].ni[17].y" 20594.28515625;
	setAttr ".tgi[0].ni[17].nvs" 1922;
	setAttr ".tgi[0].ni[18].x" -205.71427917480469;
	setAttr ".tgi[0].ni[18].y" 21768.572265625;
	setAttr ".tgi[0].ni[18].nvs" 1922;
	setAttr ".tgi[0].ni[19].x" -205.71427917480469;
	setAttr ".tgi[0].ni[19].y" 16324.2861328125;
	setAttr ".tgi[0].ni[19].nvs" 1922;
	setAttr ".tgi[0].ni[20].x" -205.71427917480469;
	setAttr ".tgi[0].ni[20].y" -3828.571533203125;
	setAttr ".tgi[0].ni[20].nvs" 1922;
	setAttr ".tgi[0].ni[21].x" -205.71427917480469;
	setAttr ".tgi[0].ni[21].y" 20192.857421875;
	setAttr ".tgi[0].ni[21].nvs" 1922;
	setAttr ".tgi[0].ni[22].x" -205.71427917480469;
	setAttr ".tgi[0].ni[22].y" 12400;
	setAttr ".tgi[0].ni[22].nvs" 1922;
	setAttr ".tgi[0].ni[23].x" -205.71427917480469;
	setAttr ".tgi[0].ni[23].y" 18435.71484375;
	setAttr ".tgi[0].ni[23].nvs" 1922;
	setAttr ".tgi[0].ni[24].x" -205.71427917480469;
	setAttr ".tgi[0].ni[24].y" 10241.4287109375;
	setAttr ".tgi[0].ni[24].nvs" 1922;
	setAttr ".tgi[0].ni[25].x" -205.71427917480469;
	setAttr ".tgi[0].ni[25].y" 10380;
	setAttr ".tgi[0].ni[25].nvs" 1922;
	setAttr ".tgi[0].ni[26].x" -205.71427917480469;
	setAttr ".tgi[0].ni[26].y" 17852.857421875;
	setAttr ".tgi[0].ni[26].nvs" 1922;
	setAttr ".tgi[0].ni[27].x" -205.71427917480469;
	setAttr ".tgi[0].ni[27].y" 25857.142578125;
	setAttr ".tgi[0].ni[27].nvs" 1922;
	setAttr ".tgi[0].ni[28].x" -205.71427917480469;
	setAttr ".tgi[0].ni[28].y" -2844.28564453125;
	setAttr ".tgi[0].ni[28].nvs" 1922;
	setAttr ".tgi[0].ni[29].x" -191.42857360839844;
	setAttr ".tgi[0].ni[29].y" 10958.5712890625;
	setAttr ".tgi[0].ni[29].nvs" 2162;
	setAttr ".tgi[0].ni[30].x" -205.71427917480469;
	setAttr ".tgi[0].ni[30].y" 10561.4287109375;
	setAttr ".tgi[0].ni[30].nvs" 1922;
	setAttr ".tgi[0].ni[31].x" -191.42857360839844;
	setAttr ".tgi[0].ni[31].y" 22398.572265625;
	setAttr ".tgi[0].ni[31].nvs" 2162;
	setAttr ".tgi[0].ni[32].x" -205.71427917480469;
	setAttr ".tgi[0].ni[32].y" 27818.572265625;
	setAttr ".tgi[0].ni[32].nvs" 1922;
	setAttr ".tgi[0].ni[33].x" -205.71427917480469;
	setAttr ".tgi[0].ni[33].y" 27818.572265625;
	setAttr ".tgi[0].ni[33].nvs" 1922;
	setAttr ".tgi[0].ni[34].x" -191.42857360839844;
	setAttr ".tgi[0].ni[34].y" 22838.572265625;
	setAttr ".tgi[0].ni[34].nvs" 2162;
	setAttr ".tgi[0].ni[35].x" -205.71427917480469;
	setAttr ".tgi[0].ni[35].y" 17490;
	setAttr ".tgi[0].ni[35].nvs" 1922;
	setAttr ".tgi[0].ni[36].x" -170;
	setAttr ".tgi[0].ni[36].y" -28497.142578125;
	setAttr ".tgi[0].ni[36].nvs" 1922;
	setAttr ".tgi[0].ni[37].x" -205.71427917480469;
	setAttr ".tgi[0].ni[37].y" 16907.142578125;
	setAttr ".tgi[0].ni[37].nvs" 1922;
	setAttr ".tgi[0].ni[38].x" -205.71427917480469;
	setAttr ".tgi[0].ni[38].y" 15905.7138671875;
	setAttr ".tgi[0].ni[38].nvs" 1922;
	setAttr ".tgi[0].ni[39].x" -205.71427917480469;
	setAttr ".tgi[0].ni[39].y" 8492.857421875;
	setAttr ".tgi[0].ni[39].nvs" 1922;
	setAttr ".tgi[0].ni[40].x" -205.71427917480469;
	setAttr ".tgi[0].ni[40].y" 4551.4287109375;
	setAttr ".tgi[0].ni[40].nvs" 1922;
	setAttr ".tgi[0].ni[41].x" -205.71427917480469;
	setAttr ".tgi[0].ni[41].y" 19601.427734375;
	setAttr ".tgi[0].ni[41].nvs" 1922;
	setAttr ".tgi[0].ni[42].x" -205.71427917480469;
	setAttr ".tgi[0].ni[42].y" -3025.71435546875;
	setAttr ".tgi[0].ni[42].nvs" 1922;
	setAttr ".tgi[0].ni[43].x" -205.71427917480469;
	setAttr ".tgi[0].ni[43].y" 22351.427734375;
	setAttr ".tgi[0].ni[43].nvs" 1922;
	setAttr ".tgi[0].ni[44].x" -208.57142639160156;
	setAttr ".tgi[0].ni[44].y" 28312.857421875;
	setAttr ".tgi[0].ni[44].nvs" 1922;
	setAttr ".tgi[0].ni[45].x" -205.71427917480469;
	setAttr ".tgi[0].ni[45].y" 9214.2861328125;
	setAttr ".tgi[0].ni[45].nvs" 1922;
	setAttr ".tgi[0].ni[46].x" -205.71427917480469;
	setAttr ".tgi[0].ni[46].y" -3245.71435546875;
	setAttr ".tgi[0].ni[46].nvs" 1922;
	setAttr ".tgi[0].ni[47].x" -205.71427917480469;
	setAttr ".tgi[0].ni[47].y" 8311.4287109375;
	setAttr ".tgi[0].ni[47].nvs" 1922;
	setAttr ".tgi[0].ni[48].x" -205.71427917480469;
	setAttr ".tgi[0].ni[48].y" 14740;
	setAttr ".tgi[0].ni[48].nvs" 1922;
	setAttr ".tgi[0].ni[49].x" -61.428569793701172;
	setAttr ".tgi[0].ni[49].y" 617.14288330078125;
	setAttr ".tgi[0].ni[49].nvs" 2498;
	setAttr ".tgi[0].ni[50].x" -205.71427917480469;
	setAttr ".tgi[0].ni[50].y" 7647.14306640625;
	setAttr ".tgi[0].ni[50].nvs" 1922;
	setAttr ".tgi[0].ni[51].x" -205.71427917480469;
	setAttr ".tgi[0].ni[51].y" 27874.28515625;
	setAttr ".tgi[0].ni[51].nvs" 1922;
	setAttr ".tgi[0].ni[52].x" -191.42857360839844;
	setAttr ".tgi[0].ni[52].y" 10078.5712890625;
	setAttr ".tgi[0].ni[52].nvs" 2162;
	setAttr ".tgi[0].ni[53].x" -205.71427917480469;
	setAttr ".tgi[0].ni[53].y" -2080;
	setAttr ".tgi[0].ni[53].nvs" 1922;
	setAttr ".tgi[0].ni[54].x" -205.71427917480469;
	setAttr ".tgi[0].ni[54].y" 27857.142578125;
	setAttr ".tgi[0].ni[54].nvs" 1922;
	setAttr ".tgi[0].ni[55].x" -205.71427917480469;
	setAttr ".tgi[0].ni[55].y" 8894.2861328125;
	setAttr ".tgi[0].ni[55].nvs" 1922;
	setAttr ".tgi[0].ni[56].x" -205.71427917480469;
	setAttr ".tgi[0].ni[56].y" 9075.7138671875;
	setAttr ".tgi[0].ni[56].nvs" 1922;
	setAttr ".tgi[0].ni[57].x" -205.71427917480469;
	setAttr ".tgi[0].ni[57].y" 4987.14306640625;
	setAttr ".tgi[0].ni[57].nvs" 1922;
	setAttr ".tgi[0].ni[58].x" -205.71427917480469;
	setAttr ".tgi[0].ni[58].y" 7318.5712890625;
	setAttr ".tgi[0].ni[58].nvs" 1922;
	setAttr ".tgi[0].ni[59].x" -205.71427917480469;
	setAttr ".tgi[0].ni[59].y" 21185.71484375;
	setAttr ".tgi[0].ni[59].nvs" 1922;
	setAttr ".tgi[0].ni[60].x" -205.71427917480469;
	setAttr ".tgi[0].ni[60].y" 27857.142578125;
	setAttr ".tgi[0].ni[60].nvs" 1922;
	setAttr ".tgi[0].ni[61].x" -205.71427917480469;
	setAttr ".tgi[0].ni[61].y" 27818.572265625;
	setAttr ".tgi[0].ni[61].nvs" 1922;
	setAttr ".tgi[0].ni[62].x" -205.71427917480469;
	setAttr ".tgi[0].ni[62].y" 15504.2861328125;
	setAttr ".tgi[0].ni[62].nvs" 1922;
	setAttr ".tgi[0].ni[63].x" -191.42857360839844;
	setAttr ".tgi[0].ni[63].y" 23278.572265625;
	setAttr ".tgi[0].ni[63].nvs" 2162;
	setAttr ".tgi[0].ni[64].x" -170;
	setAttr ".tgi[0].ni[64].y" -29278.572265625;
	setAttr ".tgi[0].ni[64].nvs" 1922;
	setAttr ".tgi[0].ni[65].x" -205.71427917480469;
	setAttr ".tgi[0].ni[65].y" 8411.4287109375;
	setAttr ".tgi[0].ni[65].nvs" 1922;
	setAttr ".tgi[0].ni[66].x" -205.71427917480469;
	setAttr ".tgi[0].ni[66].y" 27874.28515625;
	setAttr ".tgi[0].ni[66].nvs" 1922;
	setAttr ".tgi[0].ni[67].x" -205.71427917480469;
	setAttr ".tgi[0].ni[67].y" 5790;
	setAttr ".tgi[0].ni[67].nvs" 1922;
	setAttr ".tgi[0].ni[68].x" -205.71427917480469;
	setAttr ".tgi[0].ni[68].y" 20011.427734375;
	setAttr ".tgi[0].ni[68].nvs" 1922;
	setAttr ".tgi[0].ni[69].x" -205.71427917480469;
	setAttr ".tgi[0].ni[69].y" 251.42857360839844;
	setAttr ".tgi[0].ni[69].nvs" 1922;
	setAttr ".tgi[0].ni[70].x" -208.57142639160156;
	setAttr ".tgi[0].ni[70].y" 29478.572265625;
	setAttr ".tgi[0].ni[70].nvs" 1922;
	setAttr ".tgi[0].ni[71].x" -205.71427917480469;
	setAttr ".tgi[0].ni[71].y" -2662.857177734375;
	setAttr ".tgi[0].ni[71].nvs" 1922;
	setAttr ".tgi[0].ni[72].x" -205.71427917480469;
	setAttr ".tgi[0].ni[72].y" 27835.71484375;
	setAttr ".tgi[0].ni[72].nvs" 1922;
	setAttr ".tgi[0].ni[73].x" -205.71427917480469;
	setAttr ".tgi[0].ni[73].y" -2261.428466796875;
	setAttr ".tgi[0].ni[73].nvs" 1922;
	setAttr ".tgi[0].ni[74].x" -170;
	setAttr ".tgi[0].ni[74].y" -28687.142578125;
	setAttr ".tgi[0].ni[74].nvs" 1922;
	setAttr ".tgi[0].ni[75].x" -208.57142639160156;
	setAttr ".tgi[0].ni[75].y" 29077.142578125;
	setAttr ".tgi[0].ni[75].nvs" 1922;
	setAttr ".tgi[0].ni[76].x" -170;
	setAttr ".tgi[0].ni[76].y" -28877.142578125;
	setAttr ".tgi[0].ni[76].nvs" 1922;
	setAttr ".tgi[0].ni[77].x" -205.71427917480469;
	setAttr ".tgi[0].ni[77].y" 9295.7138671875;
	setAttr ".tgi[0].ni[77].nvs" 1922;
	setAttr ".tgi[0].ni[78].x" -205.71427917480469;
	setAttr ".tgi[0].ni[78].y" 27818.572265625;
	setAttr ".tgi[0].ni[78].nvs" 1922;
	setAttr ".tgi[0].ni[79].x" -205.71427917480469;
	setAttr ".tgi[0].ni[79].y" 471.42855834960938;
	setAttr ".tgi[0].ni[79].nvs" 1922;
	setAttr ".tgi[0].ni[80].x" -205.71427917480469;
	setAttr ".tgi[0].ni[80].y" 9658.5712890625;
	setAttr ".tgi[0].ni[80].nvs" 1922;
	setAttr ".tgi[0].ni[81].x" -205.71427917480469;
	setAttr ".tgi[0].ni[81].y" 18655.71484375;
	setAttr ".tgi[0].ni[81].nvs" 1922;
	setAttr ".tgi[0].ni[82].x" -205.71427917480469;
	setAttr ".tgi[0].ni[82].y" 8048.5712890625;
	setAttr ".tgi[0].ni[82].nvs" 1922;
	setAttr ".tgi[0].ni[83].x" -205.71427917480469;
	setAttr ".tgi[0].ni[83].y" 27818.572265625;
	setAttr ".tgi[0].ni[83].nvs" 1922;
	setAttr ".tgi[0].ni[84].x" -205.71427917480469;
	setAttr ".tgi[0].ni[84].y" 19238.572265625;
	setAttr ".tgi[0].ni[84].nvs" 1922;
	setAttr ".tgi[0].ni[85].x" -205.71427917480469;
	setAttr ".tgi[0].ni[85].y" 13574.2861328125;
	setAttr ".tgi[0].ni[85].nvs" 1922;
	setAttr ".tgi[0].ni[86].x" -205.71427917480469;
	setAttr ".tgi[0].ni[86].y" 6152.85693359375;
	setAttr ".tgi[0].ni[86].nvs" 1922;
	setAttr ".tgi[0].ni[87].x" -205.71427917480469;
	setAttr ".tgi[0].ni[87].y" 14157.142578125;
	setAttr ".tgi[0].ni[87].nvs" 1922;
	setAttr ".tgi[0].ni[88].x" -205.71427917480469;
	setAttr ".tgi[0].ni[88].y" 14338.5712890625;
	setAttr ".tgi[0].ni[88].nvs" 1922;
	setAttr ".tgi[0].ni[89].x" -208.57142639160156;
	setAttr ".tgi[0].ni[89].y" 28895.71484375;
	setAttr ".tgi[0].ni[89].nvs" 1922;
	setAttr ".tgi[0].ni[90].x" -205.71427917480469;
	setAttr ".tgi[0].ni[90].y" 19830;
	setAttr ".tgi[0].ni[90].nvs" 1922;
	setAttr ".tgi[0].ni[91].x" -205.71427917480469;
	setAttr ".tgi[0].ni[91].y" 7465.71435546875;
	setAttr ".tgi[0].ni[91].nvs" 1922;
	setAttr ".tgi[0].ni[92].x" -205.71427917480469;
	setAttr ".tgi[0].ni[92].y" 8712.857421875;
	setAttr ".tgi[0].ni[92].nvs" 1922;
	setAttr ".tgi[0].ni[93].x" -208.57142639160156;
	setAttr ".tgi[0].ni[93].y" 28675.71484375;
	setAttr ".tgi[0].ni[93].nvs" 1922;
	setAttr ".tgi[0].ni[94].x" -205.71427917480469;
	setAttr ".tgi[0].ni[94].y" 22934.28515625;
	setAttr ".tgi[0].ni[94].nvs" 1922;
	setAttr ".tgi[0].ni[95].x" -205.71427917480469;
	setAttr ".tgi[0].ni[95].y" 7828.5712890625;
	setAttr ".tgi[0].ni[95].nvs" 1922;
	setAttr ".tgi[0].ni[96].x" -208.57142639160156;
	setAttr ".tgi[0].ni[96].y" 29668.572265625;
	setAttr ".tgi[0].ni[96].nvs" 1922;
	setAttr ".tgi[0].ni[97].x" -205.71427917480469;
	setAttr ".tgi[0].ni[97].y" -4411.4287109375;
	setAttr ".tgi[0].ni[97].nvs" 1922;
	setAttr ".tgi[0].ni[98].x" 137.14285278320312;
	setAttr ".tgi[0].ni[98].y" -29002.857421875;
	setAttr ".tgi[0].ni[98].nvs" 1923;
	setAttr ".tgi[0].ni[99].x" -205.71427917480469;
	setAttr ".tgi[0].ni[99].y" 27818.572265625;
	setAttr ".tgi[0].ni[99].nvs" 1922;
	setAttr ".tgi[0].ni[100].x" -191.42857360839844;
	setAttr ".tgi[0].ni[100].y" -2681.428466796875;
	setAttr ".tgi[0].ni[100].nvs" 2162;
	setAttr ".tgi[0].ni[101].x" -205.71427917480469;
	setAttr ".tgi[0].ni[101].y" 15724.2861328125;
	setAttr ".tgi[0].ni[101].nvs" 1922;
	setAttr ".tgi[0].ni[102].x" -205.71427917480469;
	setAttr ".tgi[0].ni[102].y" 10461.4287109375;
	setAttr ".tgi[0].ni[102].nvs" 1922;
	setAttr ".tgi[0].ni[103].x" -191.42857360839844;
	setAttr ".tgi[0].ni[103].y" -2241.428466796875;
	setAttr ".tgi[0].ni[103].nvs" 2162;
	setAttr ".tgi[0].ni[104].x" -205.71427917480469;
	setAttr ".tgi[0].ni[104].y" 6662.85693359375;
	setAttr ".tgi[0].ni[104].nvs" 1922;
	setAttr ".tgi[0].ni[105].x" -191.42857360839844;
	setAttr ".tgi[0].ni[105].y" -1801.4285888671875;
	setAttr ".tgi[0].ni[105].nvs" 2162;
	setAttr ".tgi[0].ni[106].x" -205.71427917480469;
	setAttr ".tgi[0].ni[106].y" 10962.857421875;
	setAttr ".tgi[0].ni[106].nvs" 1922;
	setAttr ".tgi[0].ni[107].x" -205.71427917480469;
	setAttr ".tgi[0].ni[107].y" 5388.5712890625;
	setAttr ".tgi[0].ni[107].nvs" 1922;
	setAttr ".tgi[0].ni[108].x" -205.71427917480469;
	setAttr ".tgi[0].ni[108].y" 16095.7138671875;
	setAttr ".tgi[0].ni[108].nvs" 1922;
	setAttr ".tgi[0].ni[109].x" -205.71427917480469;
	setAttr ".tgi[0].ni[109].y" 1235.7142333984375;
	setAttr ".tgi[0].ni[109].nvs" 1922;
	setAttr ".tgi[0].ni[110].x" -205.71427917480469;
	setAttr ".tgi[0].ni[110].y" 16505.71484375;
	setAttr ".tgi[0].ni[110].nvs" 1922;
	setAttr ".tgi[0].ni[111].x" -205.71427917480469;
	setAttr ".tgi[0].ni[111].y" 13755.7138671875;
	setAttr ".tgi[0].ni[111].nvs" 1922;
	setAttr ".tgi[0].ni[112].x" -205.71427917480469;
	setAttr ".tgi[0].ni[112].y" 11990;
	setAttr ".tgi[0].ni[112].nvs" 1922;
	setAttr ".tgi[0].ni[113].x" -205.71427917480469;
	setAttr ".tgi[0].ni[113].y" 6080;
	setAttr ".tgi[0].ni[113].nvs" 1922;
	setAttr ".tgi[0].ni[114].x" -205.71427917480469;
	setAttr ".tgi[0].ni[114].y" 27857.142578125;
	setAttr ".tgi[0].ni[114].nvs" 1922;
	setAttr ".tgi[0].ni[115].x" -191.42857360839844;
	setAttr ".tgi[0].ni[115].y" 24158.572265625;
	setAttr ".tgi[0].ni[115].nvs" 2162;
	setAttr ".tgi[0].ni[116].x" -205.71427917480469;
	setAttr ".tgi[0].ni[116].y" -4010;
	setAttr ".tgi[0].ni[116].nvs" 1922;
	setAttr ".tgi[0].ni[117].x" -205.71427917480469;
	setAttr ".tgi[0].ni[117].y" -512.85711669921875;
	setAttr ".tgi[0].ni[117].nvs" 1922;
	setAttr ".tgi[0].ni[118].x" -205.71427917480469;
	setAttr ".tgi[0].ni[118].y" 6481.4287109375;
	setAttr ".tgi[0].ni[118].nvs" 1922;
	setAttr ".tgi[0].ni[119].x" -205.71427917480469;
	setAttr ".tgi[0].ni[119].y" 27857.142578125;
	setAttr ".tgi[0].ni[119].nvs" 1922;
	setAttr ".tgi[0].ni[120].x" -205.71427917480469;
	setAttr ".tgi[0].ni[120].y" 18254.28515625;
	setAttr ".tgi[0].ni[120].nvs" 1922;
	setAttr ".tgi[0].ni[121].x" -205.71427917480469;
	setAttr ".tgi[0].ni[121].y" 15322.857421875;
	setAttr ".tgi[0].ni[121].nvs" 1922;
	setAttr ".tgi[0].ni[122].x" -205.71427917480469;
	setAttr ".tgi[0].ni[122].y" 17671.427734375;
	setAttr ".tgi[0].ni[122].nvs" 1922;
	setAttr ".tgi[0].ni[123].x" -205.71427917480469;
	setAttr ".tgi[0].ni[123].y" 25675.71484375;
	setAttr ".tgi[0].ni[123].nvs" 1922;
	setAttr ".tgi[0].ni[124].x" -205.71427917480469;
	setAttr ".tgi[0].ni[124].y" 18837.142578125;
	setAttr ".tgi[0].ni[124].nvs" 1922;
	setAttr ".tgi[0].ni[125].x" -191.42857360839844;
	setAttr ".tgi[0].ni[125].y" 25038.572265625;
	setAttr ".tgi[0].ni[125].nvs" 2162;
	setAttr ".tgi[0].ni[126].x" -191.42857360839844;
	setAttr ".tgi[0].ni[126].y" 21078.572265625;
	setAttr ".tgi[0].ni[126].nvs" 2162;
	setAttr ".tgi[0].ni[127].x" -205.71427917480469;
	setAttr ".tgi[0].ni[127].y" -331.42855834960938;
	setAttr ".tgi[0].ni[127].nvs" 1922;
	setAttr ".tgi[0].ni[128].x" -205.71427917480469;
	setAttr ".tgi[0].ni[128].y" 10642.857421875;
	setAttr ".tgi[0].ni[128].nvs" 1922;
	setAttr ".tgi[0].ni[129].x" -205.71427917480469;
	setAttr ".tgi[0].ni[129].y" 19420;
	setAttr ".tgi[0].ni[129].nvs" 1922;
	setAttr ".tgi[0].ni[130].x" -191.42857360839844;
	setAttr ".tgi[0].ni[130].y" -921.4285888671875;
	setAttr ".tgi[0].ni[130].nvs" 2162;
	setAttr ".tgi[0].ni[131].x" -205.71427917480469;
	setAttr ".tgi[0].ni[131].y" 21587.142578125;
	setAttr ".tgi[0].ni[131].nvs" 1922;
	setAttr ".tgi[0].ni[132].x" -205.71427917480469;
	setAttr ".tgi[0].ni[132].y" 12581.4287109375;
	setAttr ".tgi[0].ni[132].nvs" 1922;
	setAttr ".tgi[0].ni[133].x" -205.71427917480469;
	setAttr ".tgi[0].ni[133].y" 27818.572265625;
	setAttr ".tgi[0].ni[133].nvs" 1922;
	setAttr ".tgi[0].ni[134].x" -170;
	setAttr ".tgi[0].ni[134].y" -29058.572265625;
	setAttr ".tgi[0].ni[134].nvs" 1922;
	setAttr ".tgi[0].ni[135].x" -205.71427917480469;
	setAttr ".tgi[0].ni[135].y" 5898.5712890625;
	setAttr ".tgi[0].ni[135].nvs" 1922;
	setAttr ".tgi[0].ni[136].x" -205.71427917480469;
	setAttr ".tgi[0].ni[136].y" 24501.427734375;
	setAttr ".tgi[0].ni[136].nvs" 1922;
	setAttr ".tgi[0].ni[137].x" -205.71427917480469;
	setAttr ".tgi[0].ni[137].y" -3427.142822265625;
	setAttr ".tgi[0].ni[137].nvs" 1922;
	setAttr ".tgi[0].ni[138].x" -205.71427917480469;
	setAttr ".tgi[0].ni[138].y" 27818.572265625;
	setAttr ".tgi[0].ni[138].nvs" 1922;
	setAttr ".tgi[0].ni[139].x" -205.71427917480469;
	setAttr ".tgi[0].ni[139].y" 16687.142578125;
	setAttr ".tgi[0].ni[139].nvs" 1922;
	setAttr ".tgi[0].ni[140].x" -205.71427917480469;
	setAttr ".tgi[0].ni[140].y" 27857.142578125;
	setAttr ".tgi[0].ni[140].nvs" 1922;
	setAttr ".tgi[0].ni[141].x" -205.71427917480469;
	setAttr ".tgi[0].ni[141].y" 14558.5712890625;
	setAttr ".tgi[0].ni[141].nvs" 1922;
	setAttr ".tgi[0].ni[142].x" -205.71427917480469;
	setAttr ".tgi[0].ni[142].y" 21367.142578125;
	setAttr ".tgi[0].ni[142].nvs" 1922;
	setAttr ".tgi[0].ni[143].x" -205.71427917480469;
	setAttr ".tgi[0].ni[143].y" 7137.14306640625;
	setAttr ".tgi[0].ni[143].nvs" 1922;
	setAttr ".tgi[0].ni[144].x" -170;
	setAttr ".tgi[0].ni[144].y" -29507.142578125;
	setAttr ".tgi[0].ni[144].nvs" 1922;
	setAttr ".tgi[0].ni[145].x" -205.71427917480469;
	setAttr ".tgi[0].ni[145].y" 9978.5712890625;
	setAttr ".tgi[0].ni[145].nvs" 1922;
	setAttr ".tgi[0].ni[146].x" -205.71427917480469;
	setAttr ".tgi[0].ni[146].y" 7728.5712890625;
	setAttr ".tgi[0].ni[146].nvs" 1922;
	setAttr ".tgi[0].ni[147].x" -205.71427917480469;
	setAttr ".tgi[0].ni[147].y" -1497.142822265625;
	setAttr ".tgi[0].ni[147].nvs" 1922;
	setAttr ".tgi[0].ni[148].x" -205.71427917480469;
	setAttr ".tgi[0].ni[148].y" 27818.572265625;
	setAttr ".tgi[0].ni[148].nvs" 1922;
	setAttr ".tgi[0].ni[149].x" -205.71427917480469;
	setAttr ".tgi[0].ni[149].y" 27818.572265625;
	setAttr ".tgi[0].ni[149].nvs" 1922;
	setAttr ".tgi[0].ni[150].x" -205.71427917480469;
	setAttr ".tgi[0].ni[150].y" 11044.2861328125;
	setAttr ".tgi[0].ni[150].nvs" 1922;
	setAttr ".tgi[0].ni[151].x" -205.71427917480469;
	setAttr ".tgi[0].ni[151].y" 4732.85693359375;
	setAttr ".tgi[0].ni[151].nvs" 1922;
	setAttr ".tgi[0].ni[152].x" -205.71427917480469;
	setAttr ".tgi[0].ni[152].y" 22532.857421875;
	setAttr ".tgi[0].ni[152].nvs" 1922;
	setAttr ".tgi[0].ni[153].x" -205.71427917480469;
	setAttr ".tgi[0].ni[153].y" 22170;
	setAttr ".tgi[0].ni[153].nvs" 1922;
	setAttr ".tgi[0].ni[154].x" -205.71427917480469;
	setAttr ".tgi[0].ni[154].y" 27857.142578125;
	setAttr ".tgi[0].ni[154].nvs" 1922;
	setAttr ".tgi[0].ni[155].x" -205.71427917480469;
	setAttr ".tgi[0].ni[155].y" 2401.428466796875;
	setAttr ".tgi[0].ni[155].nvs" 1922;
	setAttr ".tgi[0].ni[156].x" -191.42857360839844;
	setAttr ".tgi[0].ni[156].y" 23718.572265625;
	setAttr ".tgi[0].ni[156].nvs" 2162;
	setAttr ".tgi[0].ni[157].x" -191.42857360839844;
	setAttr ".tgi[0].ni[157].y" -481.42855834960938;
	setAttr ".tgi[0].ni[157].nvs" 2162;
	setAttr ".tgi[0].ni[158].x" -205.71427917480469;
	setAttr ".tgi[0].ni[158].y" 7064.28564453125;
	setAttr ".tgi[0].ni[158].nvs" 1922;
	setAttr ".tgi[0].ni[159].x" -205.71427917480469;
	setAttr ".tgi[0].ni[159].y" 6372.85693359375;
	setAttr ".tgi[0].ni[159].nvs" 1922;
	setAttr ".tgi[0].ni[160].x" -205.71427917480469;
	setAttr ".tgi[0].ni[160].y" -1860;
	setAttr ".tgi[0].ni[160].nvs" 1922;
	setAttr ".tgi[0].ni[161].x" -191.42857360839844;
	setAttr ".tgi[0].ni[161].y" -3121.428466796875;
	setAttr ".tgi[0].ni[161].nvs" 2162;
	setAttr ".tgi[0].ni[162].x" -205.71427917480469;
	setAttr ".tgi[0].ni[162].y" 2220;
	setAttr ".tgi[0].ni[162].nvs" 1922;
	setAttr ".tgi[0].ni[163].x" -205.71427917480469;
	setAttr ".tgi[0].ni[163].y" 11407.142578125;
	setAttr ".tgi[0].ni[163].nvs" 1922;
	setAttr ".tgi[0].ni[164].x" -205.71427917480469;
	setAttr ".tgi[0].ni[164].y" 27857.142578125;
	setAttr ".tgi[0].ni[164].nvs" 1922;
	setAttr ".tgi[0].ni[165].x" -205.71427917480469;
	setAttr ".tgi[0].ni[165].y" 27818.572265625;
	setAttr ".tgi[0].ni[165].nvs" 1922;
	setAttr ".tgi[0].ni[166].x" -205.71427917480469;
	setAttr ".tgi[0].ni[166].y" 11225.7138671875;
	setAttr ".tgi[0].ni[166].nvs" 1922;
	setAttr ".tgi[0].ni[167].x" -205.71427917480469;
	setAttr ".tgi[0].ni[167].y" -1277.142822265625;
	setAttr ".tgi[0].ni[167].nvs" 1922;
	setAttr ".tgi[0].ni[168].x" -205.71427917480469;
	setAttr ".tgi[0].ni[168].y" 834.28570556640625;
	setAttr ".tgi[0].ni[168].nvs" 1922;
	setAttr ".tgi[0].ni[169].x" -205.71427917480469;
	setAttr ".tgi[0].ni[169].y" 4914.28564453125;
	setAttr ".tgi[0].ni[169].nvs" 1922;
	setAttr ".tgi[0].ni[170].x" -205.71427917480469;
	setAttr ".tgi[0].ni[170].y" 20412.857421875;
	setAttr ".tgi[0].ni[170].nvs" 1922;
	setAttr ".tgi[0].ni[171].x" -205.71427917480469;
	setAttr ".tgi[0].ni[171].y" 10160;
	setAttr ".tgi[0].ni[171].nvs" 1922;
	setAttr ".tgi[0].ni[172].x" -205.71427917480469;
	setAttr ".tgi[0].ni[172].y" 20784.28515625;
	setAttr ".tgi[0].ni[172].nvs" 1922;
	setAttr ".tgi[0].ni[173].x" -205.71427917480469;
	setAttr ".tgi[0].ni[173].y" 6882.85693359375;
	setAttr ".tgi[0].ni[173].nvs" 1922;
	setAttr ".tgi[0].ni[174].x" -191.42857360839844;
	setAttr ".tgi[0].ni[174].y" 20638.572265625;
	setAttr ".tgi[0].ni[174].nvs" 2162;
	setAttr ".tgi[0].ni[175].x" -205.71427917480469;
	setAttr ".tgi[0].ni[175].y" 5315.71435546875;
	setAttr ".tgi[0].ni[175].nvs" 1922;
	setAttr ".tgi[0].ni[176].x" -205.71427917480469;
	setAttr ".tgi[0].ni[176].y" 27818.572265625;
	setAttr ".tgi[0].ni[176].nvs" 1922;
	setAttr ".tgi[0].ni[177].x" -205.71427917480469;
	setAttr ".tgi[0].ni[177].y" 27857.142578125;
	setAttr ".tgi[0].ni[177].nvs" 1922;
	setAttr ".tgi[0].ni[178].x" -205.71427917480469;
	setAttr ".tgi[0].ni[178].y" -4191.4287109375;
	setAttr ".tgi[0].ni[178].nvs" 1922;
	setAttr ".tgi[0].ni[179].x" -205.71427917480469;
	setAttr ".tgi[0].ni[179].y" 14921.4287109375;
	setAttr ".tgi[0].ni[179].nvs" 1922;
	setAttr ".tgi[0].ni[180].x" -205.71427917480469;
	setAttr ".tgi[0].ni[180].y" 4331.4287109375;
	setAttr ".tgi[0].ni[180].nvs" 1922;
	setAttr ".tgi[0].ni[181].x" -205.71427917480469;
	setAttr ".tgi[0].ni[181].y" 12801.4287109375;
	setAttr ".tgi[0].ni[181].nvs" 1922;
	setAttr ".tgi[0].ni[182].x" -205.71427917480469;
	setAttr ".tgi[0].ni[182].y" 10824.2861328125;
	setAttr ".tgi[0].ni[182].nvs" 1922;
	setAttr ".tgi[0].ni[183].x" -205.71427917480469;
	setAttr ".tgi[0].ni[183].y" 12218.5712890625;
	setAttr ".tgi[0].ni[183].nvs" 1922;
	setAttr ".tgi[0].ni[184].x" -205.71427917480469;
	setAttr ".tgi[0].ni[184].y" 19018.572265625;
	setAttr ".tgi[0].ni[184].nvs" 1922;
	setAttr ".tgi[0].ni[185].x" -205.71427917480469;
	setAttr ".tgi[0].ni[185].y" 652.85711669921875;
	setAttr ".tgi[0].ni[185].nvs" 1922;
	setAttr ".tgi[0].ni[186].x" -205.71427917480469;
	setAttr ".tgi[0].ni[186].y" 24100;
	setAttr ".tgi[0].ni[186].nvs" 1922;
	setAttr ".tgi[0].ni[187].x" -191.42857360839844;
	setAttr ".tgi[0].ni[187].y" 398.57144165039062;
	setAttr ".tgi[0].ni[187].nvs" 2162;
	setAttr ".tgi[0].ni[188].x" -205.71427917480469;
	setAttr ".tgi[0].ni[188].y" -1095.7142333984375;
	setAttr ".tgi[0].ni[188].nvs" 1922;
	setAttr ".tgi[0].ni[189].x" -205.71427917480469;
	setAttr ".tgi[0].ni[189].y" 5971.4287109375;
	setAttr ".tgi[0].ni[189].nvs" 1922;
	setAttr ".tgi[0].ni[190].x" -191.42857360839844;
	setAttr ".tgi[0].ni[190].y" 20198.572265625;
	setAttr ".tgi[0].ni[190].nvs" 2162;
	setAttr ".tgi[0].ni[191].x" -205.71427917480469;
	setAttr ".tgi[0].ni[191].y" 11144.2861328125;
	setAttr ".tgi[0].ni[191].nvs" 1922;
	setAttr ".tgi[0].ni[192].x" -205.71427917480469;
	setAttr ".tgi[0].ni[192].y" 17088.572265625;
	setAttr ".tgi[0].ni[192].nvs" 1922;
	setAttr ".tgi[0].ni[193].x" -205.71427917480469;
	setAttr ".tgi[0].ni[193].y" 7538.5712890625;
	setAttr ".tgi[0].ni[193].nvs" 1922;
	setAttr ".tgi[0].ni[194].x" -205.71427917480469;
	setAttr ".tgi[0].ni[194].y" 11627.142578125;
	setAttr ".tgi[0].ni[194].nvs" 1922;
	setAttr ".tgi[0].ni[195].x" -205.71427917480469;
	setAttr ".tgi[0].ni[195].y" 7910;
	setAttr ".tgi[0].ni[195].nvs" 1922;
	setAttr ".tgi[0].ni[196].x" -205.71427917480469;
	setAttr ".tgi[0].ni[196].y" 27835.71484375;
	setAttr ".tgi[0].ni[196].nvs" 1922;
	setAttr ".tgi[0].ni[197].x" -205.71427917480469;
	setAttr ".tgi[0].ni[197].y" 21950;
	setAttr ".tgi[0].ni[197].nvs" 1922;
	setAttr ".tgi[0].ni[198].x" -205.71427917480469;
	setAttr ".tgi[0].ni[198].y" 25092.857421875;
	setAttr ".tgi[0].ni[198].nvs" 1922;
	setAttr ".tgi[0].ni[199].x" -205.71427917480469;
	setAttr ".tgi[0].ni[199].y" 5497.14306640625;
	setAttr ".tgi[0].ni[199].nvs" 1922;
	setAttr ".tgi[0].ni[200].x" -205.71427917480469;
	setAttr ".tgi[0].ni[200].y" 9395.7138671875;
	setAttr ".tgi[0].ni[200].nvs" 1922;
	setAttr ".tgi[0].ni[201].x" -205.71427917480469;
	setAttr ".tgi[0].ni[201].y" -694.28570556640625;
	setAttr ".tgi[0].ni[201].nvs" 1922;
	setAttr ".tgi[0].ni[202].x" -205.71427917480469;
	setAttr ".tgi[0].ni[202].y" -3608.571533203125;
	setAttr ".tgi[0].ni[202].nvs" 1922;
	setAttr ".tgi[0].ni[203].x" -205.71427917480469;
	setAttr ".tgi[0].ni[203].y" 24682.857421875;
	setAttr ".tgi[0].ni[203].nvs" 1922;
	setAttr ".tgi[0].ni[204].x" -205.71427917480469;
	setAttr ".tgi[0].ni[204].y" 24872.857421875;
	setAttr ".tgi[0].ni[204].nvs" 1922;
	setAttr ".tgi[0].ni[205].x" -205.71427917480469;
	setAttr ".tgi[0].ni[205].y" 27857.142578125;
	setAttr ".tgi[0].ni[205].nvs" 1922;
	setAttr ".tgi[0].ni[206].x" -205.71427917480469;
	setAttr ".tgi[0].ni[206].y" 8812.857421875;
	setAttr ".tgi[0].ni[206].nvs" 1922;
	setAttr ".tgi[0].ni[207].x" -205.71427917480469;
	setAttr ".tgi[0].ni[207].y" 6300;
	setAttr ".tgi[0].ni[207].nvs" 1922;
	setAttr ".tgi[0].ni[208].x" -191.42857360839844;
	setAttr ".tgi[0].ni[208].y" 9638.5712890625;
	setAttr ".tgi[0].ni[208].nvs" 2162;
	setAttr ".tgi[0].ni[209].x" -205.71427917480469;
	setAttr ".tgi[0].ni[209].y" 1417.142822265625;
	setAttr ".tgi[0].ni[209].nvs" 1922;
	setAttr ".tgi[0].ni[210].x" -205.71427917480469;
	setAttr ".tgi[0].ni[210].y" -1678.5714111328125;
	setAttr ".tgi[0].ni[210].nvs" 1922;
	setAttr ".tgi[0].ni[211].x" -205.71427917480469;
	setAttr ".tgi[0].ni[211].y" 27835.71484375;
	setAttr ".tgi[0].ni[211].nvs" 1922;
	setAttr ".tgi[0].ni[212].x" -205.71427917480469;
	setAttr ".tgi[0].ni[212].y" 8230;
	setAttr ".tgi[0].ni[212].nvs" 1922;
	setAttr ".tgi[0].ni[213].x" -205.71427917480469;
	setAttr ".tgi[0].ni[213].y" 10742.857421875;
	setAttr ".tgi[0].ni[213].nvs" 1922;
	setAttr ".tgi[0].ni[214].x" -205.71427917480469;
	setAttr ".tgi[0].ni[214].y" 6735.71435546875;
	setAttr ".tgi[0].ni[214].nvs" 1922;
	setAttr ".tgi[0].ni[215].x" -205.71427917480469;
	setAttr ".tgi[0].ni[215].y" 5134.28564453125;
	setAttr ".tgi[0].ni[215].nvs" 1922;
	setAttr ".tgi[0].ni[216].x" -205.71427917480469;
	setAttr ".tgi[0].ni[216].y" 27818.572265625;
	setAttr ".tgi[0].ni[216].nvs" 1922;
	setAttr ".tgi[0].ni[217].x" -205.71427917480469;
	setAttr ".tgi[0].ni[217].y" 27818.572265625;
	setAttr ".tgi[0].ni[217].nvs" 1922;
	setAttr ".tgi[0].ni[218].x" -205.71427917480469;
	setAttr ".tgi[0].ni[218].y" 8631.4287109375;
	setAttr ".tgi[0].ni[218].nvs" 1922;
	setAttr ".tgi[0].ni[219].x" -205.71427917480469;
	setAttr ".tgi[0].ni[219].y" 12982.857421875;
	setAttr ".tgi[0].ni[219].nvs" 1922;
	setAttr ".tgi[0].ni[220].x" -205.71427917480469;
	setAttr ".tgi[0].ni[220].y" 8130;
	setAttr ".tgi[0].ni[220].nvs" 1922;
	setAttr ".tgi[0].ni[221].x" 98.571426391601562;
	setAttr ".tgi[0].ni[221].y" 28845.71484375;
	setAttr ".tgi[0].ni[221].nvs" 2354;
	setAttr ".tgi[0].ni[222].x" -205.71427917480469;
	setAttr ".tgi[0].ni[222].y" 27827.142578125;
	setAttr ".tgi[0].ni[222].nvs" 1922;
	setAttr ".tgi[0].ni[223].x" -205.71427917480469;
	setAttr ".tgi[0].ni[223].y" 70;
	setAttr ".tgi[0].ni[223].nvs" 1922;
	setAttr ".tgi[0].ni[224].x" -191.42857360839844;
	setAttr ".tgi[0].ni[224].y" 21518.572265625;
	setAttr ".tgi[0].ni[224].nvs" 2162;
	setAttr ".tgi[0].ni[225].x" -205.71427917480469;
	setAttr ".tgi[0].ni[225].y" -111.42857360839844;
	setAttr ".tgi[0].ni[225].nvs" 1922;
	setAttr ".tgi[0].ni[226].x" -205.71427917480469;
	setAttr ".tgi[0].ni[226].y" 27818.572265625;
	setAttr ".tgi[0].ni[226].nvs" 1922;
	setAttr ".tgi[0].ni[227].x" -208.57142639160156;
	setAttr ".tgi[0].ni[227].y" 28494.28515625;
	setAttr ".tgi[0].ni[227].nvs" 1922;
	setAttr ".tgi[0].ni[228].x" -205.71427917480469;
	setAttr ".tgi[0].ni[228].y" 17270;
	setAttr ".tgi[0].ni[228].nvs" 1922;
	setAttr ".tgi[0].ni[229].x" -205.71427917480469;
	setAttr ".tgi[0].ni[229].y" 21004.28515625;
	setAttr ".tgi[0].ni[229].nvs" 1922;
	setAttr ".tgi[0].ni[230].x" -205.71427917480469;
	setAttr ".tgi[0].ni[230].y" -2442.857177734375;
	setAttr ".tgi[0].ni[230].nvs" 1922;
	setAttr ".tgi[0].ni[231].x" -205.71427917480469;
	setAttr ".tgi[0].ni[231].y" 11808.5712890625;
	setAttr ".tgi[0].ni[231].nvs" 1922;
	setAttr ".tgi[0].ni[232].x" -205.71427917480469;
	setAttr ".tgi[0].ni[232].y" 13975.7138671875;
	setAttr ".tgi[0].ni[232].nvs" 1922;
	setAttr ".tgi[0].ni[233].x" -205.71427917480469;
	setAttr ".tgi[0].ni[233].y" 23335.71484375;
	setAttr ".tgi[0].ni[233].nvs" 1922;
	setAttr ".tgi[0].ni[234].x" -205.71427917480469;
	setAttr ".tgi[0].ni[234].y" 1637.142822265625;
	setAttr ".tgi[0].ni[234].nvs" 1922;
	setAttr ".tgi[0].ni[235].x" -205.71427917480469;
	setAttr ".tgi[0].ni[235].y" 9577.142578125;
	setAttr ".tgi[0].ni[235].nvs" 1922;
	setAttr ".tgi[0].ni[236].x" -205.71427917480469;
	setAttr ".tgi[0].ni[236].y" 27818.572265625;
	setAttr ".tgi[0].ni[236].nvs" 1922;
	setAttr ".tgi[0].ni[237].x" -205.71427917480469;
	setAttr ".tgi[0].ni[237].y" 5207.14306640625;
	setAttr ".tgi[0].ni[237].nvs" 1922;
	setAttr ".tgi[0].ni[238].x" -45.714286804199219;
	setAttr ".tgi[0].ni[238].y" 167.14285278320312;
	setAttr ".tgi[0].ni[238].nvs" 2162;
	setAttr ".tgi[0].ni[239].x" -191.42857360839844;
	setAttr ".tgi[0].ni[239].y" -41.428569793701172;
	setAttr ".tgi[0].ni[239].nvs" 2162;
	setAttr ".tgi[0].ni[240].x" -205.71427917480469;
	setAttr ".tgi[0].ni[240].y" 5570;
	setAttr ".tgi[0].ni[240].nvs" 1922;
	setAttr ".tgi[0].ni[241].x" -205.71427917480469;
	setAttr ".tgi[0].ni[241].y" 1818.5714111328125;
	setAttr ".tgi[0].ni[241].nvs" 1922;
	setAttr ".tgi[0].ni[242].x" -205.71427917480469;
	setAttr ".tgi[0].ni[242].y" 13384.2861328125;
	setAttr ".tgi[0].ni[242].nvs" 1922;
	setAttr ".tgi[0].ni[243].x" -205.71427917480469;
	setAttr ".tgi[0].ni[243].y" 22752.857421875;
	setAttr ".tgi[0].ni[243].nvs" 1922;
	setAttr ".tgi[0].ni[244].x" -205.71427917480469;
	setAttr ".tgi[0].ni[244].y" 7245.71435546875;
	setAttr ".tgi[0].ni[244].nvs" 1922;
	setAttr ".tgi[0].ni[245].x" -205.71427917480469;
	setAttr ".tgi[0].ni[245].y" 27827.142578125;
	setAttr ".tgi[0].ni[245].nvs" 1922;
	setAttr ".tgi[0].ni[246].x" -191.42857360839844;
	setAttr ".tgi[0].ni[246].y" 21958.572265625;
	setAttr ".tgi[0].ni[246].nvs" 2162;
	setAttr ".tgi[0].ni[247].x" -205.71427917480469;
	setAttr ".tgi[0].ni[247].y" 9797.142578125;
	setAttr ".tgi[0].ni[247].nvs" 1922;
	setAttr ".tgi[0].ni[248].x" -208.57142639160156;
	setAttr ".tgi[0].ni[248].y" 28092.857421875;
	setAttr ".tgi[0].ni[248].nvs" 1922;
	setAttr ".tgi[0].ni[249].x" -205.71427917480469;
	setAttr ".tgi[0].ni[249].y" 13164.2861328125;
	setAttr ".tgi[0].ni[249].nvs" 1922;
	setAttr ".tgi[0].ni[250].x" -205.71427917480469;
	setAttr ".tgi[0].ni[250].y" 23918.572265625;
	setAttr ".tgi[0].ni[250].nvs" 1922;
	setAttr ".tgi[0].ni[251].x" -205.71427917480469;
	setAttr ".tgi[0].ni[251].y" 8994.2861328125;
	setAttr ".tgi[0].ni[251].nvs" 1922;
	setAttr ".tgi[0].ni[252].x" -205.71427917480469;
	setAttr ".tgi[0].ni[252].y" 25455.71484375;
	setAttr ".tgi[0].ni[252].nvs" 1922;
	setAttr ".tgi[0].ni[253].x" -205.71427917480469;
	setAttr ".tgi[0].ni[253].y" 23517.142578125;
	setAttr ".tgi[0].ni[253].nvs" 1922;
	setAttr ".tgi[0].ni[254].x" -205.71427917480469;
	setAttr ".tgi[0].ni[254].y" 23115.71484375;
	setAttr ".tgi[0].ni[254].nvs" 1922;
	setAttr ".tgi[0].ni[255].x" -205.71427917480469;
	setAttr ".tgi[0].ni[255].y" 9878.5712890625;
	setAttr ".tgi[0].ni[255].nvs" 1922;
	setAttr ".tgi[0].ni[256].x" -205.71427917480469;
	setAttr ".tgi[0].ni[256].y" 15141.4287109375;
	setAttr ".tgi[0].ni[256].nvs" 1922;
	setAttr ".tgi[0].ni[257].x" -205.71427917480469;
	setAttr ".tgi[0].ni[257].y" 6955.71435546875;
	setAttr ".tgi[0].ni[257].nvs" 1922;
	setAttr ".tgi[0].ni[258].x" -205.71427917480469;
	setAttr ".tgi[0].ni[258].y" 5717.14306640625;
	setAttr ".tgi[0].ni[258].nvs" 1922;
	setAttr ".tgi[0].ni[259].x" -205.71427917480469;
	setAttr ".tgi[0].ni[259].y" 18072.857421875;
	setAttr ".tgi[0].ni[259].nvs" 1922;
	setAttr ".tgi[0].ni[260].x" -205.71427917480469;
	setAttr ".tgi[0].ni[260].y" 24281.427734375;
	setAttr ".tgi[0].ni[260].nvs" 1922;
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
select -ne :ikSystem;
	setAttr -s 4 ".sol";
connectAttr "SkelChar_translateX.o" "SkelChar.tx";
connectAttr "SkelChar_translateY.o" "SkelChar.ty";
connectAttr "SkelChar_translateZ.o" "SkelChar.tz";
connectAttr "SkelChar_rotateX.o" "SkelChar.rx";
connectAttr "SkelChar_rotateY.o" "SkelChar.ry";
connectAttr "SkelChar_rotateZ.o" "SkelChar.rz";
connectAttr "Hips_translateX.o" "Hips.tx";
connectAttr "Hips_translateY.o" "Hips.ty";
connectAttr "Hips_translateZ.o" "Hips.tz";
connectAttr "Hips_rotateX.o" "Hips.rx";
connectAttr "Hips_rotateY.o" "Hips.ry";
connectAttr "Hips_rotateZ.o" "Hips.rz";
connectAttr "Hips_scaleX.o" "Hips.sx";
connectAttr "Hips_scaleY.o" "Hips.sy";
connectAttr "Hips_scaleZ.o" "Hips.sz";
connectAttr "Torso_translateX.o" "Torso.tx";
connectAttr "Torso_translateY.o" "Torso.ty";
connectAttr "Torso_translateZ.o" "Torso.tz";
connectAttr "Torso_rotateX.o" "Torso.rx";
connectAttr "Torso_rotateY.o" "Torso.ry";
connectAttr "Torso_rotateZ.o" "Torso.rz";
connectAttr "Torso_scaleX.o" "Torso.sx";
connectAttr "Torso_scaleY.o" "Torso.sy";
connectAttr "Torso_scaleZ.o" "Torso.sz";
connectAttr "Chest_translateX.o" "Chest.tx";
connectAttr "Chest_translateY.o" "Chest.ty";
connectAttr "Chest_translateZ.o" "Chest.tz";
connectAttr "Chest_rotateX.o" "Chest.rx";
connectAttr "Chest_rotateY.o" "Chest.ry";
connectAttr "Chest_rotateZ.o" "Chest.rz";
connectAttr "Chest_scaleX.o" "Chest.sx";
connectAttr "Chest_scaleY.o" "Chest.sy";
connectAttr "Chest_scaleZ.o" "Chest.sz";
connectAttr "UpChest_translateX.o" "UpChest.tx";
connectAttr "UpChest_translateY.o" "UpChest.ty";
connectAttr "UpChest_translateZ.o" "UpChest.tz";
connectAttr "UpChest_rotateX.o" "UpChest.rx";
connectAttr "UpChest_rotateY.o" "UpChest.ry";
connectAttr "UpChest_rotateZ.o" "UpChest.rz";
connectAttr "UpChest_scaleX.o" "UpChest.sx";
connectAttr "UpChest_scaleY.o" "UpChest.sy";
connectAttr "UpChest_scaleZ.o" "UpChest.sz";
connectAttr "Neck_translateX.o" "Neck.tx";
connectAttr "Neck_translateY.o" "Neck.ty";
connectAttr "Neck_translateZ.o" "Neck.tz";
connectAttr "Neck_rotateX.o" "Neck.rx";
connectAttr "Neck_rotateY.o" "Neck.ry";
connectAttr "Neck_rotateZ.o" "Neck.rz";
connectAttr "Neck_scaleX.o" "Neck.sx";
connectAttr "Neck_scaleY.o" "Neck.sy";
connectAttr "Neck_scaleZ.o" "Neck.sz";
connectAttr "Head_translateX.o" "Head.tx";
connectAttr "Head_translateY.o" "Head.ty";
connectAttr "Head_translateZ.o" "Head.tz";
connectAttr "Head_rotateX.o" "Head.rx";
connectAttr "Head_rotateY.o" "Head.ry";
connectAttr "Head_rotateZ.o" "Head.rz";
connectAttr "Head_scaleX.o" "Head.sx";
connectAttr "Head_scaleY.o" "Head.sy";
connectAttr "Head_scaleZ.o" "Head.sz";
connectAttr "LEye_translateX.o" "LEye.tx";
connectAttr "LEye_translateY.o" "LEye.ty";
connectAttr "LEye_translateZ.o" "LEye.tz";
connectAttr "LEye_rotateX.o" "LEye.rx";
connectAttr "LEye_rotateY.o" "LEye.ry";
connectAttr "LEye_rotateZ.o" "LEye.rz";
connectAttr "LEye_scaleX.o" "LEye.sx";
connectAttr "LEye_scaleY.o" "LEye.sy";
connectAttr "LEye_scaleZ.o" "LEye.sz";
connectAttr "REye_translateX.o" "REye.tx";
connectAttr "REye_translateY.o" "REye.ty";
connectAttr "REye_translateZ.o" "REye.tz";
connectAttr "REye_rotateX.o" "REye.rx";
connectAttr "REye_rotateY.o" "REye.ry";
connectAttr "REye_rotateZ.o" "REye.rz";
connectAttr "REye_scaleX.o" "REye.sx";
connectAttr "REye_scaleY.o" "REye.sy";
connectAttr "REye_scaleZ.o" "REye.sz";
connectAttr "LShldr_translateX.o" "LShldr.tx";
connectAttr "LShldr_translateY.o" "LShldr.ty";
connectAttr "LShldr_translateZ.o" "LShldr.tz";
connectAttr "LShldr_rotateX.o" "LShldr.rx";
connectAttr "LShldr_rotateY.o" "LShldr.ry";
connectAttr "LShldr_rotateZ.o" "LShldr.rz";
connectAttr "LShldr_scaleX.o" "LShldr.sx";
connectAttr "LShldr_scaleY.o" "LShldr.sy";
connectAttr "LShldr_scaleZ.o" "LShldr.sz";
connectAttr "LArm_translateX.o" "LArm.tx";
connectAttr "LArm_translateY.o" "LArm.ty";
connectAttr "LArm_translateZ.o" "LArm.tz";
connectAttr "LArm_rotateX.o" "LArm.rx";
connectAttr "LArm_rotateY.o" "LArm.ry";
connectAttr "LArm_rotateZ.o" "LArm.rz";
connectAttr "LArm_scaleX.o" "LArm.sx";
connectAttr "LArm_scaleY.o" "LArm.sy";
connectAttr "LArm_scaleZ.o" "LArm.sz";
connectAttr "LElbow_translateX.o" "LElbow.tx";
connectAttr "LElbow_translateY.o" "LElbow.ty";
connectAttr "LElbow_translateZ.o" "LElbow.tz";
connectAttr "LElbow_rotateX.o" "LElbow.rx";
connectAttr "LElbow_rotateY.o" "LElbow.ry";
connectAttr "LElbow_rotateZ.o" "LElbow.rz";
connectAttr "LElbow_scaleX.o" "LElbow.sx";
connectAttr "LElbow_scaleY.o" "LElbow.sy";
connectAttr "LElbow_scaleZ.o" "LElbow.sz";
connectAttr "LHand_translateX.o" "LHand.tx";
connectAttr "LHand_translateY.o" "LHand.ty";
connectAttr "LHand_translateZ.o" "LHand.tz";
connectAttr "LHand_rotateX.o" "LHand.rx";
connectAttr "LHand_rotateY.o" "LHand.ry";
connectAttr "LHand_rotateZ.o" "LHand.rz";
connectAttr "LHand_scaleX.o" "LHand.sx";
connectAttr "LHand_scaleY.o" "LHand.sy";
connectAttr "LHand_scaleZ.o" "LHand.sz";
connectAttr "RShldr_translateX.o" "RShldr.tx";
connectAttr "RShldr_translateY.o" "RShldr.ty";
connectAttr "RShldr_translateZ.o" "RShldr.tz";
connectAttr "RShldr_rotateX.o" "RShldr.rx";
connectAttr "RShldr_rotateY.o" "RShldr.ry";
connectAttr "RShldr_rotateZ.o" "RShldr.rz";
connectAttr "RShldr_scaleX.o" "RShldr.sx";
connectAttr "RShldr_scaleY.o" "RShldr.sy";
connectAttr "RShldr_scaleZ.o" "RShldr.sz";
connectAttr "RArm_translateX.o" "RArm.tx";
connectAttr "RArm_translateY.o" "RArm.ty";
connectAttr "RArm_translateZ.o" "RArm.tz";
connectAttr "RArm_rotateX.o" "RArm.rx";
connectAttr "RArm_rotateY.o" "RArm.ry";
connectAttr "RArm_rotateZ.o" "RArm.rz";
connectAttr "RArm_scaleX.o" "RArm.sx";
connectAttr "RArm_scaleY.o" "RArm.sy";
connectAttr "RArm_scaleZ.o" "RArm.sz";
connectAttr "RElbow_translateX.o" "RElbow.tx";
connectAttr "RElbow_translateY.o" "RElbow.ty";
connectAttr "RElbow_translateZ.o" "RElbow.tz";
connectAttr "RElbow_rotateX.o" "RElbow.rx";
connectAttr "RElbow_rotateY.o" "RElbow.ry";
connectAttr "RElbow_rotateZ.o" "RElbow.rz";
connectAttr "RElbow_scaleX.o" "RElbow.sx";
connectAttr "RElbow_scaleY.o" "RElbow.sy";
connectAttr "RElbow_scaleZ.o" "RElbow.sz";
connectAttr "RHand_translateX.o" "RHand.tx";
connectAttr "RHand_translateY.o" "RHand.ty";
connectAttr "RHand_translateZ.o" "RHand.tz";
connectAttr "RHand_rotateX.o" "RHand.rx";
connectAttr "RHand_rotateY.o" "RHand.ry";
connectAttr "RHand_rotateZ.o" "RHand.rz";
connectAttr "RHand_scaleX.o" "RHand.sx";
connectAttr "RHand_scaleY.o" "RHand.sy";
connectAttr "RHand_scaleZ.o" "RHand.sz";
connectAttr "LLeg_translateX.o" "LLeg.tx";
connectAttr "LLeg_translateY.o" "LLeg.ty";
connectAttr "LLeg_translateZ.o" "LLeg.tz";
connectAttr "LLeg_rotateX.o" "LLeg.rx";
connectAttr "LLeg_rotateY.o" "LLeg.ry";
connectAttr "LLeg_rotateZ.o" "LLeg.rz";
connectAttr "LLeg_scaleX.o" "LLeg.sx";
connectAttr "LLeg_scaleY.o" "LLeg.sy";
connectAttr "LLeg_scaleZ.o" "LLeg.sz";
connectAttr "LKnee_translateX.o" "LKnee.tx";
connectAttr "LKnee_translateY.o" "LKnee.ty";
connectAttr "LKnee_translateZ.o" "LKnee.tz";
connectAttr "LKnee_rotateX.o" "LKnee.rx";
connectAttr "LKnee_rotateY.o" "LKnee.ry";
connectAttr "LKnee_rotateZ.o" "LKnee.rz";
connectAttr "LKnee_scaleX.o" "LKnee.sx";
connectAttr "LKnee_scaleY.o" "LKnee.sy";
connectAttr "LKnee_scaleZ.o" "LKnee.sz";
connectAttr "LFoot_translateX.o" "LFoot.tx";
connectAttr "LFoot_translateY.o" "LFoot.ty";
connectAttr "LFoot_translateZ.o" "LFoot.tz";
connectAttr "LFoot_rotateX.o" "LFoot.rx";
connectAttr "LFoot_rotateY.o" "LFoot.ry";
connectAttr "LFoot_rotateZ.o" "LFoot.rz";
connectAttr "LFoot_scaleX.o" "LFoot.sx";
connectAttr "LFoot_scaleY.o" "LFoot.sy";
connectAttr "LFoot_scaleZ.o" "LFoot.sz";
connectAttr "LToes_translateX.o" "LToes.tx";
connectAttr "LToes_translateY.o" "LToes.ty";
connectAttr "LToes_translateZ.o" "LToes.tz";
connectAttr "LToes_rotateX.o" "LToes.rx";
connectAttr "LToes_rotateY.o" "LToes.ry";
connectAttr "LToes_rotateZ.o" "LToes.rz";
connectAttr "LToes_scaleX.o" "LToes.sx";
connectAttr "LToes_scaleY.o" "LToes.sy";
connectAttr "LToes_scaleZ.o" "LToes.sz";
connectAttr "RLeg_translateX.o" "RLeg.tx";
connectAttr "RLeg_translateY.o" "RLeg.ty";
connectAttr "RLeg_translateZ.o" "RLeg.tz";
connectAttr "RLeg_rotateX.o" "RLeg.rx";
connectAttr "RLeg_rotateY.o" "RLeg.ry";
connectAttr "RLeg_rotateZ.o" "RLeg.rz";
connectAttr "RLeg_scaleX.o" "RLeg.sx";
connectAttr "RLeg_scaleY.o" "RLeg.sy";
connectAttr "RLeg_scaleZ.o" "RLeg.sz";
connectAttr "RKnee_translateX.o" "RKnee.tx";
connectAttr "RKnee_translateY.o" "RKnee.ty";
connectAttr "RKnee_translateZ.o" "RKnee.tz";
connectAttr "RKnee_rotateX.o" "RKnee.rx";
connectAttr "RKnee_rotateY.o" "RKnee.ry";
connectAttr "RKnee_rotateZ.o" "RKnee.rz";
connectAttr "RKnee_scaleX.o" "RKnee.sx";
connectAttr "RKnee_scaleY.o" "RKnee.sy";
connectAttr "RKnee_scaleZ.o" "RKnee.sz";
connectAttr "RFoot_translateX.o" "RFoot.tx";
connectAttr "RFoot_translateY.o" "RFoot.ty";
connectAttr "RFoot_translateZ.o" "RFoot.tz";
connectAttr "RFoot_rotateX.o" "RFoot.rx";
connectAttr "RFoot_rotateY.o" "RFoot.ry";
connectAttr "RFoot_rotateZ.o" "RFoot.rz";
connectAttr "RFoot_scaleX.o" "RFoot.sx";
connectAttr "RFoot_scaleY.o" "RFoot.sy";
connectAttr "RFoot_scaleZ.o" "RFoot.sz";
connectAttr "RToes_translateX.o" "RToes.tx";
connectAttr "RToes_translateY.o" "RToes.ty";
connectAttr "RToes_translateZ.o" "RToes.tz";
connectAttr "RToes_rotateX.o" "RToes.rx";
connectAttr "RToes_rotateY.o" "RToes.ry";
connectAttr "RToes_rotateZ.o" "RToes.rz";
connectAttr "RToes_scaleX.o" "RToes.sx";
connectAttr "RToes_scaleY.o" "RToes.sy";
connectAttr "RToes_scaleZ.o" "RToes.sz";
relationship "link" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
connectAttr "layerManager.dli[0]" "defaultLayer.id";
connectAttr "renderLayerManager.rlmi[0]" "defaultRenderLayer.rlid";
connectAttr "Hips.msg" "bindPose.m[0]";
connectAttr "Torso.msg" "bindPose.m[1]";
connectAttr "Chest.msg" "bindPose.m[2]";
connectAttr "UpChest.msg" "bindPose.m[3]";
connectAttr "Neck.msg" "bindPose.m[4]";
connectAttr "Head.msg" "bindPose.m[5]";
connectAttr "LEye.msg" "bindPose.m[6]";
connectAttr "REye.msg" "bindPose.m[7]";
connectAttr "LShldr.msg" "bindPose.m[8]";
connectAttr "LArm.msg" "bindPose.m[9]";
connectAttr "LElbow.msg" "bindPose.m[10]";
connectAttr "LHand.msg" "bindPose.m[11]";
connectAttr "RShldr.msg" "bindPose.m[32]";
connectAttr "RArm.msg" "bindPose.m[33]";
connectAttr "RElbow.msg" "bindPose.m[34]";
connectAttr "RHand.msg" "bindPose.m[35]";
connectAttr "LLeg.msg" "bindPose.m[56]";
connectAttr "LKnee.msg" "bindPose.m[57]";
connectAttr "LFoot.msg" "bindPose.m[58]";
connectAttr "LToes.msg" "bindPose.m[59]";
connectAttr "RLeg.msg" "bindPose.m[61]";
connectAttr "RKnee.msg" "bindPose.m[62]";
connectAttr "RFoot.msg" "bindPose.m[63]";
connectAttr "RToes.msg" "bindPose.m[64]";
connectAttr "Hips.bps" "bindPose.wm[0]";
connectAttr "Torso.bps" "bindPose.wm[1]";
connectAttr "Chest.bps" "bindPose.wm[2]";
connectAttr "UpChest.bps" "bindPose.wm[3]";
connectAttr "Neck.bps" "bindPose.wm[4]";
connectAttr "Head.bps" "bindPose.wm[5]";
connectAttr "LEye.bps" "bindPose.wm[6]";
connectAttr "REye.bps" "bindPose.wm[7]";
connectAttr "LShldr.bps" "bindPose.wm[8]";
connectAttr "LArm.bps" "bindPose.wm[9]";
connectAttr "LElbow.bps" "bindPose.wm[10]";
connectAttr "LHand.bps" "bindPose.wm[11]";
connectAttr "RShldr.bps" "bindPose.wm[32]";
connectAttr "RArm.bps" "bindPose.wm[33]";
connectAttr "RElbow.bps" "bindPose.wm[34]";
connectAttr "RHand.bps" "bindPose.wm[35]";
connectAttr "LLeg.bps" "bindPose.wm[56]";
connectAttr "LKnee.bps" "bindPose.wm[57]";
connectAttr "LFoot.bps" "bindPose.wm[58]";
connectAttr "LToes.bps" "bindPose.wm[59]";
connectAttr "RLeg.bps" "bindPose.wm[61]";
connectAttr "RKnee.bps" "bindPose.wm[62]";
connectAttr "RFoot.bps" "bindPose.wm[63]";
connectAttr "RToes.bps" "bindPose.wm[64]";
connectAttr "bindPose.w" "bindPose.p[0]";
connectAttr "bindPose.m[0]" "bindPose.p[1]";
connectAttr "bindPose.m[1]" "bindPose.p[2]";
connectAttr "bindPose.m[2]" "bindPose.p[3]";
connectAttr "bindPose.m[3]" "bindPose.p[4]";
connectAttr "bindPose.m[4]" "bindPose.p[5]";
connectAttr "bindPose.m[5]" "bindPose.p[6]";
connectAttr "bindPose.m[5]" "bindPose.p[7]";
connectAttr "bindPose.m[3]" "bindPose.p[8]";
connectAttr "bindPose.m[8]" "bindPose.p[9]";
connectAttr "bindPose.m[9]" "bindPose.p[10]";
connectAttr "bindPose.m[10]" "bindPose.p[11]";
connectAttr "bindPose.m[11]" "bindPose.p[12]";
connectAttr "bindPose.m[12]" "bindPose.p[13]";
connectAttr "bindPose.m[13]" "bindPose.p[14]";
connectAttr "bindPose.m[14]" "bindPose.p[15]";
connectAttr "bindPose.m[11]" "bindPose.p[16]";
connectAttr "bindPose.m[16]" "bindPose.p[17]";
connectAttr "bindPose.m[17]" "bindPose.p[18]";
connectAttr "bindPose.m[18]" "bindPose.p[19]";
connectAttr "bindPose.m[11]" "bindPose.p[20]";
connectAttr "bindPose.m[20]" "bindPose.p[21]";
connectAttr "bindPose.m[21]" "bindPose.p[22]";
connectAttr "bindPose.m[22]" "bindPose.p[23]";
connectAttr "bindPose.m[11]" "bindPose.p[24]";
connectAttr "bindPose.m[24]" "bindPose.p[25]";
connectAttr "bindPose.m[25]" "bindPose.p[26]";
connectAttr "bindPose.m[26]" "bindPose.p[27]";
connectAttr "bindPose.m[11]" "bindPose.p[28]";
connectAttr "bindPose.m[28]" "bindPose.p[29]";
connectAttr "bindPose.m[29]" "bindPose.p[30]";
connectAttr "bindPose.m[30]" "bindPose.p[31]";
connectAttr "bindPose.m[3]" "bindPose.p[32]";
connectAttr "bindPose.m[32]" "bindPose.p[33]";
connectAttr "bindPose.m[33]" "bindPose.p[34]";
connectAttr "bindPose.m[34]" "bindPose.p[35]";
connectAttr "bindPose.m[35]" "bindPose.p[36]";
connectAttr "bindPose.m[36]" "bindPose.p[37]";
connectAttr "bindPose.m[37]" "bindPose.p[38]";
connectAttr "bindPose.m[38]" "bindPose.p[39]";
connectAttr "bindPose.m[35]" "bindPose.p[40]";
connectAttr "bindPose.m[40]" "bindPose.p[41]";
connectAttr "bindPose.m[41]" "bindPose.p[42]";
connectAttr "bindPose.m[42]" "bindPose.p[43]";
connectAttr "bindPose.m[35]" "bindPose.p[44]";
connectAttr "bindPose.m[44]" "bindPose.p[45]";
connectAttr "bindPose.m[45]" "bindPose.p[46]";
connectAttr "bindPose.m[46]" "bindPose.p[47]";
connectAttr "bindPose.m[35]" "bindPose.p[48]";
connectAttr "bindPose.m[48]" "bindPose.p[49]";
connectAttr "bindPose.m[49]" "bindPose.p[50]";
connectAttr "bindPose.m[50]" "bindPose.p[51]";
connectAttr "bindPose.m[35]" "bindPose.p[52]";
connectAttr "bindPose.m[52]" "bindPose.p[53]";
connectAttr "bindPose.m[53]" "bindPose.p[54]";
connectAttr "bindPose.m[54]" "bindPose.p[55]";
connectAttr "bindPose.m[0]" "bindPose.p[56]";
connectAttr "bindPose.m[56]" "bindPose.p[57]";
connectAttr "bindPose.m[57]" "bindPose.p[58]";
connectAttr "bindPose.m[58]" "bindPose.p[59]";
connectAttr "bindPose.m[59]" "bindPose.p[60]";
connectAttr "bindPose.m[0]" "bindPose.p[61]";
connectAttr "bindPose.m[61]" "bindPose.p[62]";
connectAttr "bindPose.m[62]" "bindPose.p[63]";
connectAttr "bindPose.m[63]" "bindPose.p[64]";
connectAttr "bindPose.m[64]" "bindPose.p[65]";
connectAttr "LArm_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[0].dn"
		;
connectAttr "RShldr_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[1].dn"
		;
connectAttr "RLeg_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[2].dn"
		;
connectAttr "Torso_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[3].dn"
		;
connectAttr "LHand_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[4].dn"
		;
connectAttr "RElbow_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[5].dn"
		;
connectAttr "RKnee_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[6].dn"
		;
connectAttr "RArm.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[7].dn"
		;
connectAttr "Hips_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[9].dn"
		;
connectAttr "RLeg_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[10].dn"
		;
connectAttr "Torso.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[11].dn"
		;
connectAttr "bindPose.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[12].dn"
		;
connectAttr "RShldr_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[13].dn"
		;
connectAttr "LArm_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[14].dn"
		;
connectAttr "ExtraJoints.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[15].dn"
		;
connectAttr "RElbow_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[16].dn"
		;
connectAttr "UpChest_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[17].dn"
		;
connectAttr "Chest_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[18].dn"
		;
connectAttr "Head_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[19].dn"
		;
connectAttr "RToes_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[20].dn"
		;
connectAttr "UpChest_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[21].dn"
		;
connectAttr "REye_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[22].dn"
		;
connectAttr "Neck_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[23].dn"
		;
connectAttr "LShldr_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[24].dn"
		;
connectAttr "LLeg_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[25].dn"
		;
connectAttr "Neck_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[26].dn"
		;
connectAttr "Hips_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[27].dn"
		;
connectAttr "RToes_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[28].dn"
		;
connectAttr "RShldr.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[29].dn"
		;
connectAttr "LLeg_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[30].dn"
		;
connectAttr "LEye.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[31].dn"
		;
connectAttr "RArm_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[32].dn"
		;
connectAttr "RElbow_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[33].dn"
		;
connectAttr "Head.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[34].dn"
		;
connectAttr "Neck_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[35].dn"
		;
connectAttr "SkelChar_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[36].dn"
		;
connectAttr "Head_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[37].dn"
		;
connectAttr "Head_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[38].dn"
		;
connectAttr "LArm_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[39].dn"
		;
connectAttr "LToes_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[40].dn"
		;
connectAttr "UpChest_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[41].dn"
		;
connectAttr "RToes_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[42].dn"
		;
connectAttr "Chest_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[43].dn"
		;
connectAttr "LKnee_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[45].dn"
		;
connectAttr "RToes_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[46].dn"
		;
connectAttr "LElbow_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[47].dn"
		;
connectAttr "LEye_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[48].dn"
		;
connectAttr ":px_renderGlobals1.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[49].dn"
		;
connectAttr "LFoot_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[50].dn"
		;
connectAttr "RHand_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[51].dn"
		;
connectAttr "RElbow.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[52].dn"
		;
connectAttr "RFoot_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[53].dn"
		;
connectAttr "RElbow_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[54].dn"
		;
connectAttr "LArm_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[55].dn"
		;
connectAttr "LArm_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[56].dn"
		;
connectAttr "LHand_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[57].dn"
		;
connectAttr "LElbow_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[58].dn"
		;
connectAttr "Chest_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[59].dn"
		;
connectAttr "RElbow_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[60].dn"
		;
connectAttr "RArm_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[61].dn"
		;
connectAttr "Head_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[62].dn"
		;
connectAttr "Neck.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[63].dn"
		;
connectAttr "SkelChar_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[64].dn"
		;
connectAttr "LKnee_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[65].dn"
		;
connectAttr "RHand_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[66].dn"
		;
connectAttr "LHand_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[67].dn"
		;
connectAttr "UpChest_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[68].dn"
		;
connectAttr "RKnee_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[69].dn"
		;
connectAttr "RFoot_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[71].dn"
		;
connectAttr "RHand_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[72].dn"
		;
connectAttr "RFoot_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[73].dn"
		;
connectAttr "SkelChar_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[74].dn"
		;
connectAttr "SkelChar_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[76].dn"
		;
connectAttr "LArm_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[77].dn"
		;
connectAttr "RElbow_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[78].dn"
		;
connectAttr "RKnee_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[79].dn"
		;
connectAttr "LArm_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[80].dn"
		;
connectAttr "Neck_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[81].dn"
		;
connectAttr "LKnee_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[82].dn"
		;
connectAttr "RArm_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[83].dn"
		;
connectAttr "UpChest_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[84].dn"
		;
connectAttr "REye_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[85].dn"
		;
connectAttr "LHand_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[86].dn"
		;
connectAttr "LEye_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[87].dn"
		;
connectAttr "LEye_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[88].dn"
		;
connectAttr "UpChest_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[90].dn"
		;
connectAttr "LFoot_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[91].dn"
		;
connectAttr "LArm_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[92].dn"
		;
connectAttr "Torso_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[94].dn"
		;
connectAttr "LKnee_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[95].dn"
		;
connectAttr "RToes_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[97].dn"
		;
connectAttr "SkelChar.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[98].dn"
		;
connectAttr "RShldr_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[99].dn"
		;
connectAttr "RFoot.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[100].dn"
		;
connectAttr "Head_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[101].dn"
		;
connectAttr "LShldr_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[102].dn"
		;
connectAttr "RKnee.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[103].dn"
		;
connectAttr "LFoot_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[104].dn"
		;
connectAttr "RLeg.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[105].dn"
		;
connectAttr "LLeg_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[106].dn"
		;
connectAttr "LHand_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[107].dn"
		;
connectAttr "Head_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[108].dn"
		;
connectAttr "RLeg_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[109].dn"
		;
connectAttr "Head_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[110].dn"
		;
connectAttr "LEye_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[111].dn"
		;
connectAttr "REye_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[112].dn"
		;
connectAttr "LFoot_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[113].dn"
		;
connectAttr "RArm_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[114].dn"
		;
connectAttr "Chest.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[115].dn"
		;
connectAttr "RToes_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[116].dn"
		;
connectAttr "RKnee_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[117].dn"
		;
connectAttr "LFoot_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[118].dn"
		;
connectAttr "RArm_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[119].dn"
		;
connectAttr "Neck_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[120].dn"
		;
connectAttr "LEye_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[121].dn"
		;
connectAttr "Neck_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[122].dn"
		;
connectAttr "Hips_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[123].dn"
		;
connectAttr "Neck_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[124].dn"
		;
connectAttr "Hips.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[125].dn"
		;
connectAttr "LArm.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[126].dn"
		;
connectAttr "RKnee_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[127].dn"
		;
connectAttr "LShldr_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[128].dn"
		;
connectAttr "UpChest_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[129].dn"
		;
connectAttr "LToes.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[130].dn"
		;
connectAttr "Chest_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[131].dn"
		;
connectAttr "REye_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[132].dn"
		;
connectAttr "RElbow_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[133].dn"
		;
connectAttr "SkelChar_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[134].dn"
		;
connectAttr "LToes_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[135].dn"
		;
connectAttr "Hips_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[136].dn"
		;
connectAttr "RToes_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[137].dn"
		;
connectAttr "RElbow_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[138].dn"
		;
connectAttr "Head_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[139].dn"
		;
connectAttr "RHand_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[140].dn"
		;
connectAttr "LEye_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[141].dn"
		;
connectAttr "Chest_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[142].dn"
		;
connectAttr "LElbow_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[143].dn"
		;
connectAttr "SkelChar_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[144].dn"
		;
connectAttr "LLeg_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[145].dn"
		;
connectAttr "LElbow_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[146].dn"
		;
connectAttr "RFoot_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[147].dn"
		;
connectAttr "RArm_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[148].dn"
		;
connectAttr "RShldr_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[149].dn"
		;
connectAttr "LShldr_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[150].dn"
		;
connectAttr "LToes_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[151].dn"
		;
connectAttr "Torso_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[152].dn"
		;
connectAttr "Chest_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[153].dn"
		;
connectAttr "RElbow_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[154].dn"
		;
connectAttr "RLeg_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[155].dn"
		;
connectAttr "UpChest.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[156].dn"
		;
connectAttr "LFoot.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[157].dn"
		;
connectAttr "LFoot_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[158].dn"
		;
connectAttr "LHand_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[159].dn"
		;
connectAttr "RFoot_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[160].dn"
		;
connectAttr "RToes.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[161].dn"
		;
connectAttr "RLeg_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[162].dn"
		;
connectAttr "LShldr_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[163].dn"
		;
connectAttr "RShldr_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[164].dn"
		;
connectAttr "RArm_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[165].dn"
		;
connectAttr "LShldr_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[166].dn"
		;
connectAttr "RFoot_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[167].dn"
		;
connectAttr "RLeg_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[168].dn"
		;
connectAttr "LToes_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[169].dn"
		;
connectAttr "UpChest_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[170].dn"
		;
connectAttr "LLeg_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[171].dn"
		;
connectAttr "Chest_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[172].dn"
		;
connectAttr "LFoot_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[173].dn"
		;
connectAttr "LElbow.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[174].dn"
		;
connectAttr "LToes_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[175].dn"
		;
connectAttr "RArm_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[176].dn"
		;
connectAttr "RArm_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[177].dn"
		;
connectAttr "RToes_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[178].dn"
		;
connectAttr "LEye_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[179].dn"
		;
connectAttr "LToes_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[180].dn"
		;
connectAttr "REye_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[181].dn"
		;
connectAttr "LShldr_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[182].dn"
		;
connectAttr "REye_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[183].dn"
		;
connectAttr "UpChest_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[184].dn"
		;
connectAttr "RKnee_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[185].dn"
		;
connectAttr "Torso_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[186].dn"
		;
connectAttr "LLeg.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[187].dn"
		;
connectAttr "RFoot_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[188].dn"
		;
connectAttr "LHand_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[189].dn"
		;
connectAttr "LHand.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[190].dn"
		;
connectAttr "LLeg_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[191].dn"
		;
connectAttr "Head_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[192].dn"
		;
connectAttr "LElbow_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[193].dn"
		;
connectAttr "LShldr_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[194].dn"
		;
connectAttr "LElbow_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[195].dn"
		;
connectAttr "RHand_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[196].dn"
		;
connectAttr "Chest_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[197].dn"
		;
connectAttr "Hips_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[198].dn"
		;
connectAttr "LToes_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[199].dn"
		;
connectAttr "LKnee_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[200].dn"
		;
connectAttr "RKnee_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[201].dn"
		;
connectAttr "RToes_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[202].dn"
		;
connectAttr "Hips_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[203].dn"
		;
connectAttr "Hips_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[204].dn"
		;
connectAttr "RShldr_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[205].dn"
		;
connectAttr "LKnee_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[206].dn"
		;
connectAttr "LFoot_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[207].dn"
		;
connectAttr "RHand.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[208].dn"
		;
connectAttr "RLeg_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[209].dn"
		;
connectAttr "RFoot_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[210].dn"
		;
connectAttr "RHand_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[211].dn"
		;
connectAttr "LKnee_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[212].dn"
		;
connectAttr "LLeg_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[213].dn"
		;
connectAttr "LElbow_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[214].dn"
		;
connectAttr "LToes_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[215].dn"
		;
connectAttr "RHand_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[216].dn"
		;
connectAttr "RShldr_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[217].dn"
		;
connectAttr "LKnee_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[218].dn"
		;
connectAttr "REye_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[219].dn"
		;
connectAttr "LElbow_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[220].dn"
		;
connectAttr "RHand_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[222].dn"
		;
connectAttr "RKnee_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[223].dn"
		;
connectAttr "LShldr.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[224].dn"
		;
connectAttr "RKnee_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[225].dn"
		;
connectAttr "RShldr_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[226].dn"
		;
connectAttr "Neck_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[228].dn"
		;
connectAttr "Chest_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[229].dn"
		;
connectAttr "RFoot_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[230].dn"
		;
connectAttr "LShldr_translateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[231].dn"
		;
connectAttr "LEye_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[232].dn"
		;
connectAttr "Torso_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[233].dn"
		;
connectAttr "RLeg_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[234].dn"
		;
connectAttr "LLeg_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[235].dn"
		;
connectAttr "RShldr_translateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[236].dn"
		;
connectAttr "LHand_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[237].dn"
		;
connectAttr "RHandPropAttach.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[238].dn"
		;
connectAttr "LKnee.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[239].dn"
		;
connectAttr "LHand_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[240].dn"
		;
connectAttr "RLeg_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[241].dn"
		;
connectAttr "REye_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[242].dn"
		;
connectAttr "Torso_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[243].dn"
		;
connectAttr "LFoot_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[244].dn"
		;
connectAttr "RHand_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[245].dn"
		;
connectAttr "REye.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[246].dn"
		;
connectAttr "LLeg_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[247].dn"
		;
connectAttr "REye_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[249].dn"
		;
connectAttr "Torso_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[250].dn"
		;
connectAttr "LKnee_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[251].dn"
		;
connectAttr "Hips_scaleX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[252].dn"
		;
connectAttr "Torso_translateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[253].dn"
		;
connectAttr "Torso_scaleY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[254].dn"
		;
connectAttr "LArm_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[255].dn"
		;
connectAttr "LEye_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[256].dn"
		;
connectAttr "LElbow_rotateZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[257].dn"
		;
connectAttr "LToes_rotateX.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[258].dn"
		;
connectAttr "Neck_rotateY.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[259].dn"
		;
connectAttr "Hips_scaleZ.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[260].dn"
		;
connectAttr "defaultRenderLayer.msg" ":defaultRenderingList1.r" -na;
// End of UsdExportSkeleton.ma
