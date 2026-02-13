# WasmFetchResolver Example

This example showcases a simple resolver utilizing the
[Emscripten Fetch API](https://emscripten.org/docs/api_reference/fetch.html) to
load files from a webserver.  In order to facilitate file loading, a bare bones
[Express.js](https://expressjs.com/) webserver is included in `server.js`.
This server is configured to serve static files and showcase the resolver's
functionality.

### Configuring the Example

Make sure that examples are being built along with your Wasm build.
Once the main build has been completed, navigate to the example directory
and install the Node.js dependencies:
```sh
cd $INST_DIR/share/usd/examples/bin/wasmFetchResolver
npm install
```
### Running the Server

After dependencies have been installed, start the server:
```sh
npm run server
```
After the server has started, open a browser and navigate to
`http://localhost:8080/wasmFetchResolver.html`. The stage dropdown menu is
pre-populated with a sample model.  Click one of the three buttons below to view
the output.

### Testing Additional Assets

In order to test with different assets, copy them into the `public` directory
within the example's root directory.  Next, modify the `stages.js` to include
an entry for your new model. You can then either reload the page or click the
refresh button to view the updated list of models from the dropdown menu.