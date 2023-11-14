Setup
-----

Build USD with Emscripten (in the root of this repository):

locally
```sh
python build_scripts/build_usd.py --emscripten <build_folder>
```
or in a Docker container
```sh
docker build --build-arg -t <CONTAINER_TAG> .
docker run -it -w //src -p 6931:6931 <CONTAINER_TAG>
```

After installation, you have a *bin* subfolder under your *build* folder. This contains
a *hdEmscripten.html.html* file you can open in a browser. USD-for-Web uses the SharedArrayBuffer
feature - this requires certain security headers (CORS).
If you don't want to worry about this for testing purposes you can run the Chrome browser
with --enable-features=SharedArrayBuffer as a command line argument.

Inside the docker container you can run the server with

```sh
cd /src/USD_emscripten/bin
python3 wasm-server.py --port 6931
```

Now you can go your browser and open http://localhost:6931/hdEmscripten.html