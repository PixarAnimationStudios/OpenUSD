const Module = require("../bindings/Release/jsBindings");

let Usd;
let sphere;

describe('USD Attribute', () => {
  beforeEach(async () => {
    Usd = await Module();
    const fileName = "HelloWorld.usda";
    let stage = Usd.UsdStage.CreateNew(fileName);
    stage.DefinePrim("/hello", "Xform");
    sphere = stage.DefinePrim("/hello/world", "Sphere");
  }, 500000);

  afterEach(() => {
    Usd.PThread.runningWorkers.forEach(x => x.onmessage = function() {});
    Usd.PThread.terminateAllThreads();
    Usd = null;
    sphere = null;
    process.removeAllListeners('unhandledRejection')
    process.removeAllListeners('uncaughtException')
  });

  it("Get - VtValue", () => {
    expect(sphere.GetAttribute('radius').Get()).toBe(1);
  });

  it("Get - VtArray", () => {
    const extent = sphere.GetAttribute('extent').Get();
    expect(extent).toEqual([[-1, -1, -1], [1, 1, 1]]);
  });

  it("Set - VtValue", () => {
    let radiusAttr = sphere.GetAttribute('radius');
    radiusAttr.Set(2);
    expect(radiusAttr.Get()).toEqual(2);
  });

  it("Set - VtArray", () => {
    let extentAttr = sphere.GetAttribute('extent');
    resultExtent = [-1, -1, -1, 2, 2, 2];
    extentAttr.Set(resultExtent);
    expect(extentAttr.Get()).toEqual([[-1, -1, -1], [2, 2, 2]]);

    resultExtent = [[-1, -1, -1], [3, 3, 3]];
    extentAttr.Set(resultExtent);
    expect(extentAttr.Get()).toEqual([[-1, -1, -1], [3, 3, 3]]);
  });

  it("GetTypeName - VtValue", () => {
    expect(sphere.GetAttribute('radius').GetTypeName()).toEqual("double");
  });

  it("GetTypeName - VtArray", () => {
    expect(sphere.GetAttribute('extent').GetTypeName()).toEqual("VtArray<GfVec3f>");
  });
});
