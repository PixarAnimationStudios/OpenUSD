const Module = require("../bindings/Release/jsBindings");

let stage;
let xform;
let Usd;

describe('UsdGeomXformable', () => {
  beforeEach(async () => {
    Usd = await Module();
    const fileName = "HelloWorld.usda";
    stage = Usd.UsdStage.CreateNew(fileName);
    xform = Usd.UsdGeomXform.Define(stage, "/test");
  }, 500000);

  afterEach(() => {
    Usd.PThread.runningWorkers.forEach(x => x.onmessage = function() {});
    Usd.PThread.terminateAllThreads();
    Usd = null;
    stage = null;
    xform = null;
    process.removeAllListeners('unhandledRejection')
    process.removeAllListeners('uncaughtException')
  });

  it("AddScaleOp constructor should modify the stage", () => {
    const scaleOp = xform.AddScaleOp();
    scaleOp.Set([1, 2, 3]);
    let data = stage.ExportToString();
    expect(data).toContain("xformOp:scale");
    expect(data).toContain("(1, 2, 3)");
  });

  it("AddTranslateOp constructor should modify the stage", () => {
    const scaleOp = xform.AddTranslateOp();
    scaleOp.Set([1, 2, 3]);
    let data = stage.ExportToString();
    expect(data).toContain("xformOp:translate");
    expect(data).toContain("(1, 2, 3)");
  });

  it("SetXformOpOrder should modify the stage", () => {
    xform.SetXformOpOrder([]);
    let data = stage.ExportToString();
    expect(data).toContain("uniform token[] xformOpOrder = []");
  });
});
