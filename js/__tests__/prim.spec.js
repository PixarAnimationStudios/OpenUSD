const Module = require("../bindings/Release/jsBindings");

let Usd;
let sphere;

describe('USD Prim', () => {
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

  it("GetAttribute", () => {
    expect(sphere.GetAttribute('radius')).not.toBeUndefined();
  });

  it("GetAttributes", () => {
    const attributes = sphere.GetAttributes();
    expect(attributes.size()).toBe(9);
  });

  it("GetTypeName", () => {
    const name = sphere.GetTypeName();
    expect(name).toBe("Sphere");
  });

  it("GetPropertyNames", () => {
    const propNames = sphere.GetPropertyNames();
    expect(propNames).toEqual(["doubleSided", "extent", "orientation", "primvars:displayColor", "primvars:displayOpacity", "proxyPrim", "purpose", "radius", "visibility", "xformOpOrder"]);
  });

  it("GetReferences", () => {
    const references = sphere.GetReferences();
    expect(references).not.toBeUndefined();
  });
});
