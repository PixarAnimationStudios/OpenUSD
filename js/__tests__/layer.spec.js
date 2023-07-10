const Module = require("../bindings/Release/jsBindings");

let Usd;
let sphere;

describe('USD Attribute', () => {
  beforeEach(async () => {
    Usd = await Module();
    const fileName = "HelloWorld.usda";
  }, 500000);

  afterEach(() => {
    Usd.PThread.runningWorkers.forEach(x => x.onmessage = function() {});
    Usd.PThread.terminateAllThreads();
    Usd = null;
    process.removeAllListeners('unhandledRejection')
    process.removeAllListeners('uncaughtException')
  });

  it("Create anonymous layer with two arguments", () => {
    let layer = Usd.SdfLayer.CreateAnonymous('usda', {});
    expect(layer).not.toBe(undefined);
  });

  it("Create anonymous layer with one argument", () => {
    let layer = Usd.SdfLayer.CreateAnonymous('usda');
    expect(layer).not.toBe(undefined);
  });

  it("SDF Find with one argument", () => {
    let layer = Usd.SdfLayer.CreateAnonymous('usda');
    let foundLayer = Usd.SdfLayer.Find('test')
    expect(foundLayer).not.toBe(undefined);

    layer.identifier = 'test';

    foundLayer = Usd.SdfLayer.Find('test');
    expect(foundLayer).not.toBe(layer);
  });

  it("Create a Prim in a layer", () => {
    let layer = Usd.SdfLayer.CreateAnonymous('usda');
    let prim = Usd.SdfCreatePrimInLayer(layer, '/test');
    expect(prim).not.toBe(undefined);

    let prim_r = layer.GetPrimAtPath('/test');
    expect(prim_r).not.toBe(undefined);
    expect(prim_r).not.toBe('/test');
    // prim !== prim2. It seems smart pointers don't support testing for equality in embind

    let prim2 = Usd.SdfCreatePrimInLayer(layer, '/test/test2');
    let prim2_r = layer.GetPrimAtPath('/test/test2');
    expect(prim2_r).not.toBe(undefined);

    expect(prim2).not.toBe('/test/test2');
    expect(prim2_r).not.toBe('/test/test2');

    prim.SetInfo("specifier", Usd.SdfSpecifier.SdfSpecifierDef);
  });

  it("Create an AttributeSpec in a prim", () => {
    let layer = Usd.SdfLayer.CreateAnonymous('usda');
    let prim  = Usd.SdfCreatePrimInLayer(layer, '/test');
    prim.typeName = 'XForm'

    let attribute = new Usd.SdfAttributeSpec(prim, "visibility", Usd.ValueTypeNames.Find("token"));
    let attribute2 = layer.GetPropertyAtPath('/test.visibility');
    expect(attribute2.path).toBe(attribute.path);

    attribute.default = 'inherit';
    expect(attribute.default).toEqual('inherit');

    let xformAttribute = new Usd.SdfAttributeSpec(prim, "xformOp:rotateXYZ", Usd.ValueTypeNames.Find("float3"));
    xformAttribute.default = [-30,0,0];
    expect(xformAttribute.default).toEqual([-30,0,0]);

    xformAttribute.default = undefined;
    expect(xformAttribute.default).toEqual(undefined);

    let xformOpOrderAttribute = new Usd.SdfAttributeSpec(prim, "xformOpOrder", Usd.ValueTypeNames.Find("token[]"));
    xformOpOrderAttribute.default = ["xformOp:tranlate", "xformOp:tranlate:piv"];
    expect(xformOpOrderAttribute.default).toEqual(["xformOp:tranlate", "xformOp:tranlate:piv"]);

    let prim2  = Usd.SdfCreatePrimInLayer(layer, '/test2');
    prim.typeName = 'Mesh';
    let extentAttribute = new Usd.SdfAttributeSpec(prim2, "extent", Usd.ValueTypeNames.Find("float3[]"));
    extentAttribute.default = [ 0, 0, 0, 1, 1, 1];
    expect(extentAttribute.default).toEqual([[0, 0, 0], [1, 1, 1]]);

    extentAttribute.SetInfo('custom', true);
    extentAttribute.SetInfo('typeName', 'token');
  });

  it("Create an RelationshipSpec in a prim", () => {
    let layer = Usd.SdfLayer.CreateAnonymous('usda');
    let prim  = Usd.SdfCreatePrimInLayer(layer, '/test');
    prim.typeName = 'XForm'
    let relationship = new Usd.SdfRelationshipSpec(prim, "relationship");
    let relationship2 = layer.GetPropertyAtPath('/test.relationship');
    expect(relationship.path).toBe(relationship2.path);

    for (const member of ["addedItems", "explicitItems", "prependedItems",
                          "appendedItems", "deletedItems", "orderedItems"]) {
      relationship.targetPathList[member] = ['/a', '/b'];
      expect(relationship.targetPathList[member]).toEqual(['/a', '/b']);
    }
  });
});
