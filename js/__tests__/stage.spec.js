const Module = require("../bindings/Release/jsBindings");
const fs = require("fs");
const path = require("path");

let Usd;
let stage;

describe('USD Stage', () => {
beforeEach(async () => {
    Usd = await Module();
  const fileName = "HelloWorld.usda";
  stage = Usd.UsdStage.CreateNew(fileName);
  }, 500000);

  afterEach(() => {
  Usd.PThread.runningWorkers.forEach(x => x.onmessage = function() {});
  Usd.PThread.terminateAllThreads();
  Usd = null;
  stage = null;
    process.removeAllListeners('unhandledRejection');
  process.removeAllListeners('uncaughtException')
});

  it("CreateNew", () => {
    expect(stage).not.toBeUndefined();
  });

  it("DefinePrim", () => {
    let sphere = stage.DefinePrim("/hello/world", "Sphere");
    expect(sphere).not.toBeUndefined();
  });

  it("ExportToString", () => {
    stage.DefinePrim("/hello", "Xform");
    stage.DefinePrim("/hello/world", "Sphere");
    const data = stage.ExportToString();
    expect(data.indexOf('Sphere')).not.toBe(-1);
    expect(data.indexOf('Xform')).not.toBe(-1);
  });

  it("GetPrimAtPath", () => {
    stage.DefinePrim("/hello/world", "Sphere");
    expect(stage.GetPrimAtPath("/hello/world").GetTypeName()).toBe("Sphere");
  });

  it("SetDefaultPrim", () => {
    const xform = stage.DefinePrim('/hello', 'Xform');
    stage.SetDefaultPrim(xform);
    expect(stage.ExportToString().indexOf('defaultPrim = "hello"')).not.toBe(-1);
  });

  it("OverridePrim", () => {
    stage.OverridePrim('/refSphere');
    expect(stage.ExportToString().indexOf('over "refSphere"')).not.toBe(-1);
  });

  it("GetRootLayer", () => {
    expect(stage.GetRootLayer()).not.toBeUndefined();
  });

  it("Open", async () => {
    await Usd.uploadJS(path.resolve(__dirname, "./test-data/sphere.usda"), "sphere.usda");
    const stage = Usd.UsdStage.Open("sphere.usda");
    const data = stage.ExportToString();
    expect(data.indexOf('def Sphere "sphere"')).not.toBe(-1);;
  });

  it("GetLayerStack", () => {
    let layers = stage.GetLayerStack(true);
    expect(layers).not.toBeUndefined();
    expect(layers.size()).toBe(2);
  });
});
