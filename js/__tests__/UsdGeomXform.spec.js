const Module = require("../bindings/Release/jsBindings");

let stage;
let Usd;

describe('UsdGeomXform', () => {
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
    process.removeAllListeners('unhandledRejection')
    process.removeAllListeners('uncaughtException')
  });

  it("Define constructor should modify the stage", () => {
    Usd.UsdGeomXform.Define(stage, "/test")
    let data = stage.ExportToString();
    expect(data).toContain("Xform");
    expect(data).toContain("test")
  });
});
