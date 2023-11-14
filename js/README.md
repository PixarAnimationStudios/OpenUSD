Additional Requirements
-----------------------

Node version 18.0.0^
Emscripten version 3.1.47^

Setup
-----

Build USD with Emscripten for NodeJS (in the root of this repository):

locally
```sh
python build_scripts/build_usd.py --emscriptenNode <build_folder>
```
or in a Docker container
```sh
docker build --build-arg BUILD_TARGET=--emscriptenNode -t <CONTAINER_TAG> .
docker run -it -w //src <CONTAINER_TAG>
```

and run

```sh
cd js
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

NPM package consumption
------------------------

Currently, we only support consumption of the bindings via Script tags in the browser or via Node.js

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
