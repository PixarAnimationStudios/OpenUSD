const Module = require("../bindings/Release/jsBindings");
const fs = require("fs");
const path = require("path");

let Usd;

describe('USD Tutorials', () => {
  beforeEach(async () => {
      Usd = await Module();
    }, 500000);

  afterEach(() => {
    Usd.PThread.runningWorkers.forEach(x => x.onmessage = function() {});
    Usd.PThread.terminateAllThreads();
    Usd = null;
    process.removeAllListeners('unhandledRejection')
    process.removeAllListeners('uncaughtException')
  });

  it("Referencing Layers", () => {
    const usdFilename = 'HelloWorld.usda';
    const stage = Usd.UsdStage.CreateNew(usdFilename);
    const xform = stage.DefinePrim('/hello', 'Xform');
    const sphere = stage.DefinePrim('/hello/world', 'Sphere');

    stage.SetDefaultPrim(xform);
    const xformable = new Usd.UsdGeomXformable(xform);
    const translateOp = xformable.AddTranslateOp();
    translateOp.Set([4, 5, 6]);

    const radiusAttr = sphere.GetAttribute('radius');
    radiusAttr.Set(2);

    const color = sphere.GetAttribute("primvars:displayColor");
    color.Set([[0,0,1]]);

    // First Reference
    const refStage = Usd.UsdStage.CreateNew('RefExample.usda')

    refSphere = refStage.OverridePrim('/refSphere')
    refSphere.GetReferences().AddReference('./HelloWorld.usda')
    refXform = new Usd.UsdGeomXformable(refSphere);
    refXform.SetXformOpOrder([]);

    // Second Reference
    const refSphere2 = refStage.OverridePrim('/refSphere2')
    refSphere2.GetReferences().AddReference('./HelloWorld.usda')
    const overSphere = refStage.GetPrimAtPath('/refSphere2/world');
    overSphere.GetAttribute("primvars:displayColor").Set( [[1, 0, 0]] );

    expect(refStage.ExportToString()).toMatchSnapshot();
    expect(refStage.GetRootLayer().ExportToString()).toMatchSnapshot();
  });

  it("Simple Shading in USD", () => {
    const fileName = "SimpleShading.usda";

    const data = fs.readFileSync(path.resolve(__dirname, './test-data/simpleshading.usda'));

    Usd.FS.createDataFile('/', fileName, data, true, true, true);
    const stage = Usd.UsdStage.Open(fileName);

    const stReader = Usd.UsdShadeShader.Define(stage, '/TexModel/boardMat/stReader');
    stReader.CreateIdAttr('UsdPrimvarReader_float2');
    stReader.CreateInput('varname', Usd.ValueTypeNames.Token).ConnectToSourcePath("/TexModel/boardMat.inputs:frame:stPrimvarName");

    const sampler = Usd.UsdShadeShader.Define(stage, "/TexModel/boardMat/texture");
    sampler.CreateIdAttr("UsdUVTexture");
    const fileInput = sampler.CreateInput("file", Usd.ValueTypeNames.Asset);
    fileInput.Set("USDLogoHeader.png");
    const st = sampler.CreateInput("st", Usd.ValueTypeNames.Float2);
    st.ConnectToSource(stReader, 'result')
    sampler.CreateOutput("rgb", Usd.ValueTypeNames.Float3);

    const pbrShader = Usd.UsdShadeShader.Define(stage, '/TexModel/boardMat/PBRShader')
    pbrShader.CreateIdAttr("UsdPreviewSurface");
    pbrShader.CreateInput("roughness", Usd.ValueTypeNames.Float).Set(0.4);
    pbrShader.CreateInput("metallic", Usd.ValueTypeNames.Float).Set(0.0);
    pbrShader.CreateInput("diffuseColor", Usd.ValueTypeNames.Color3f).ConnectToSource(sampler, "rgb");
    pbrShader.CreateOutput("surface", Usd.ValueTypeNames.Token);

    expect(stage.ExportToString()).toMatchSnapshot();
  });
});
