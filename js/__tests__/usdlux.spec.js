const Module = require("../bindings/Release/jsBindings");

let Usd;
let stage;

describe('UsdLux', () => {
  beforeEach(async () => {
      Usd = await Module();
      const fileName = "lights.usda";
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

  it("Add UsdLuxDistantLight to a stage", () => {
    Usd.UsdLuxDistantLight.Define(stage, '/distantLight');
    const data = stage.ExportToString();
    expect(data.indexOf('DistantLight')).not.toBe(-1);;
  });
});
