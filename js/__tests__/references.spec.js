const Module = require("../bindings/Release/jsBindings");

let Usd;
let stage;

describe('USD References', () => {
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

  it("AddReference", () => {
    const xform = stage.DefinePrim("/hello", "Xform");
    stage.SetDefaultPrim(xform);

    let refStage = Usd.UsdStage.CreateNew('RefExample.usda');
    refSphere = refStage.OverridePrim('/refSphere');
    refSphere.GetReferences().AddReference('./HelloWorld.usda');

    expect(refStage.GetRootLayer().ExportToString().indexOf('prepend references = @./HelloWorld.usda@')).not.toBe(-1);
  });
});
