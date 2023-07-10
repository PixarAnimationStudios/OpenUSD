const Module = require("../bindings/Release/jsBindings");

let Usd;
let stage;
describe("UsdShade", () => {
  beforeEach(async () => {
      Usd = await Module();
      const fileName = "UsdShade.usda";
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

  describe("UsdShadeShader", () => {
    it("Define", () => {
      Usd.UsdShadeShader.Define(stage, "/shader");
      expect(stage.GetPrimAtPath("/shader").GetTypeName()).toBe("Shader");
    });

    it("CreateIdAttr", () => {
      const shader = Usd.UsdShadeShader.Define(stage, "/shader");
      shader.CreateIdAttr("idattr");
      expect(shader.GetIdAttr().GetTypeName()).toBe("TfToken");
    });

    it("CreateInput", () => {
      const shader = Usd.UsdShadeShader.Define(stage, "/shader");
      shader.CreateInput("input", Usd.ValueTypeNames.Float);
      expect(shader.GetInput("input")).not.toBeUndefined();
    });

    it("CreateOutput", () => {
      const shader = Usd.UsdShadeShader.Define(stage, "/shader");
      shader.CreateOutput("output", Usd.ValueTypeNames.Float);
      expect(shader.GetOutput("output")).not.toBeUndefined();
    });
  });

  describe("UsdShadeInput", () => {
    it("ConnectToSource (Shader)", () => {
      const shader = Usd.UsdShadeShader.Define(stage, "/shader");
      const otherShader = Usd.UsdShadeShader.Define(stage, "/otherShader");
      const input = shader.CreateInput("input", Usd.ValueTypeNames.Float);
      expect(input.ConnectToSource(otherShader, "connection")).toEqual(true);
    });

    it("ConnectToSource (Input)", () => {
      const shader = Usd.UsdShadeShader.Define(stage, "/shader");
      const otherShader = Usd.UsdShadeShader.Define(stage, "/otherShader");
      const input = shader.CreateInput("input", Usd.ValueTypeNames.Token);
      const otherInput = otherShader.CreateInput("otherInput", Usd.ValueTypeNames.Token);
      expect(input.ConnectToSourceInput(otherInput)).toEqual(true);;
    });

    it("ConnectToSource (Path)", () => {
      const shader = Usd.UsdShadeShader.Define(stage, "/shader");
      const otherShader = Usd.UsdShadeShader.Define(stage, "/otherShader");
      const input = shader.CreateInput("input", Usd.ValueTypeNames.Float);
      otherShader.CreateInput("otherInput", Usd.ValueTypeNames.Float);
      expect(input.ConnectToSourcePath("/otherShader.inputs:float:otherInput")).toEqual(true);
    });
  });
});
