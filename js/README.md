Additional Requirements
-----------------------

Node version 16.0.0^
Emscripten version 2.0.24^

Setup
-----

Build USD with Emscripten for NodeJS (in the root of this repository):

locally
```sh
python build_scripts/build_usd.py --emscripten <build_folder>
```
or in a Docker container
```sh
docker build -t <CONTAINER_TAG> .
docker run -it -w //src -p 6931:6931 <CONTAINER_TAG>
```

and run

```sh
npm install
```

in this `js` folder.

Tests
------

Run

```sh
npm run test
```

or in watch mode

```sh
npm run test --  --watch
```

After installation you have a *bin* subfolder under your *build* folder. This contains 
a *test.html* file you can open in a browser. USD-for-Web uses the SharedArrayBuffer
feature - this requires certain security headers (CORS). 
If you don't want to worry about this for testing purposes you can run the Chrome browser
with --enable-features=SharedArrayBuffer as a command line argument.

NPM package consumption
------------------------

Currently we only support consumption of the bindings via Script tags in the browser or via Node.js

If you are using Webpack you can add the bindings to your application with

```
  externals: {
    "usd": 'usd', // indicates global variable
  },
  plugins: [
    new CopyPlugin({
      patterns: [
        { from: "<PATH TO BINDINGS>" },
      ],
    }),
  ],
```

and after adding `<script src="jsBindings.js"></script>` to your HTML page use it in your code with

```
    <script src="jsBindings.js"></script>
    <script type="module">
      const Usd = await usdModule();
      const UsdStage = Usd.UsdStage;
      let stage = UsdStage.CreateNew('HelloWorld.usda');
    </script>
```

In Node.Js you can load it via 
```
const usdModule = require("usd");
const Usd = await usdModule();
const UsdStage = Usd.UsdStage;

let stage = UsdStage.CreateNew('HelloWorld.usda');
...
```
