Module = Module || {};
Module.uploadJS = async (filePath, fileName) => {
  let isNode = false;
  if (typeof module !== 'undefined' && module.exports) {
      isNode = true;
  }
  let usdFile;
  if (isNode) {
      const fs = require('fs');
      usdFile = fs.readFileSync(filePath);
  } else {
      usdFile = await new Promise( (resolve, reject) => {
      let req = new XMLHttpRequest();
      req.open("GET", filePath, true);
      req.responseType = "arraybuffer";
      req.onload = function (oEvent) {
          let arrayBuffer = req.response;
          if (arrayBuffer) {
          resolve(arrayBuffer);
          } else {
          reject();
          }
      };
      req.send(null);
      });
  }

  Module.FS.createDataFile('/', fileName, new Uint8Array(usdFile), true, true, true);
}
