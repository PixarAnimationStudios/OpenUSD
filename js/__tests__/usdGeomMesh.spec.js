const Module = require("../bindings/Release/jsBindings");

let Usd;
let mesh;

describe('USD Mesh', () => {
  beforeEach(async () => {
      Usd = await Module();
      const fileName = "HelloWorld.usda";
      let stage = Usd.UsdStage.CreateNew(fileName);
      stage.DefinePrim("/hello", "Xform");
      mesh = Usd.UsdGeomMesh.Define(stage, "/hello/mesh");
    }, 500000);

  afterEach(() => {
    Usd.PThread.runningWorkers.forEach(x => x.onmessage = function() {});
    Usd.PThread.terminateAllThreads();
    Usd = null;
    mesh = null;
    process.removeAllListeners('unhandledRejection')
    process.removeAllListeners('uncaughtException')
  });

  it("Define Mesh", () => {
    expect(mesh).not.toBeUndefined();
  });

  it("GetPointsAttr", () => {
    expect(mesh.GetPointsAttr()).not.toBeUndefined();
    expect(mesh.GetPointsAttr().Get()).toBeUndefined();
  });

  it("CreatePointsAttr", () => {
    const testData = [[-430, -145, 0], [430, -145, 0], [430, 145, 0], [-430, 145, 0]];
    const pointsAttr = mesh.CreatePointsAttr(testData);
    expect(pointsAttr).not.toBeUndefined();
    expect(pointsAttr).toEqual(mesh.GetPointsAttr());
    expect(pointsAttr.Get()).toEqual(testData);
  });

  it("GetFaceVertexCountsAttr", () => {
    expect(mesh.GetFaceVertexCountsAttr()).not.toBeUndefined();
    expect(mesh.GetFaceVertexCountsAttr().Get()).toBeUndefined();
  });

  it("CreateFaceVertexCountsAttr", () => {
    const testData = [4];
    const faceVertexCountsAttr = mesh.CreateFaceVertexCountsAttr(testData);
    expect(faceVertexCountsAttr).not.toBeUndefined();
    expect(faceVertexCountsAttr).toEqual(mesh.GetFaceVertexCountsAttr());
    expect(faceVertexCountsAttr.Get()).toEqual(testData);
  });

  it("GetFaceVertexIndicesAttr", () => {
    expect(mesh.GetFaceVertexIndicesAttr()).not.toBeUndefined();
    expect(mesh.GetFaceVertexIndicesAttr().Get()).toBeUndefined();
  });

  it("CreateFaceVertexIndicesAttr", () => {
    const testData = [0, 1, 2, 3];
    const faceVertexIndicesAttr = mesh.CreateFaceVertexIndicesAttr(testData);
    expect(faceVertexIndicesAttr).not.toBeUndefined();
    expect(faceVertexIndicesAttr).toEqual(mesh.GetFaceVertexIndicesAttr());
    expect(faceVertexIndicesAttr.Get()).toEqual(testData);
  });

  it("GetExtentAttr", () => {
    expect(mesh.GetExtentAttr()).not.toBeUndefined();
    expect(mesh.GetExtentAttr().Get()).toBeUndefined();
  });

  it("CreateExtentAttr", () => {
    const testData = [[-430, -145, 0], [430, 145, 0]];
    const extentAttr = mesh.CreateExtentAttr(testData);
    expect(extentAttr).not.toBeUndefined();
    expect(extentAttr).toEqual(mesh.GetExtentAttr());
    expect(extentAttr.Get()).toEqual(testData);
  });
});
