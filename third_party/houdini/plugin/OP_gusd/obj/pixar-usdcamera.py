
# This script will be run every time a usd camera node is created.

node = kwargs['node']
if node:

    hou.hscript( 'obj/cam.cmd ' + node.path())
    node.setName( 'main_cam', unique_name = True )

    frustum = node.createNode( 'pixar::camerafrustum', 'frustum' )
    visibility = node.createNode( 'visibility', 'visibility' )
    merge = node.createNode( 'merge', 'merge' )

    xform = node.node( 'xform1' )
    merge.insertInput( 0, xform )
    merge.insertInput( 1, visibility )
    visibility.insertInput( 0, frustum )

    node.layoutChildren()

    visibility.parm( 'action' ).setExpression( "ch('../displayFrustum' )",
                                                hou.exprLanguage.Hscript )

    merge.setDisplayFlag(True)
    xform.setRenderFlag(True)

    # Lock all params in the Transform folder
    for parm in node.parmsInFolder(["Transform"]):
        parm.lock(True)

    # If a site specific customization file exists, run it. 
    # A Pixar we use this to set default values to point to a 
    # shot specific USD file.
    try:
        f = hou.findFile( 'scripts/obj/pixar-usdcamera-site.py' )
        if f:
            execfile( f )

    except:
        pass
