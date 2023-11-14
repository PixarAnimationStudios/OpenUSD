FROM ubuntu:23.04
SHELL ["/bin/bash", "-c"]
ARG BUILD_TARGET=--emscripten
ARG EMSCRIPTEN_VERSION=3.1.47
RUN apt-get -y update && apt-get install -y\
        software-properties-common \
        cmake \
        g++ \
        lbzip2 \
        git \
        npm=9.2.*

RUN alias python='python3'
RUN mkdir -p tmp && cd tmp && git clone --recursive https://github.com/emscripten-core/emsdk
RUN cd /tmp/emsdk && ./emsdk install ${EMSCRIPTEN_VERSION}
RUN cd /tmp/emsdk && ./emsdk activate ${EMSCRIPTEN_VERSION} --permanent

COPY . /src/
RUN source /tmp/emsdk/emsdk_env.sh && cd /src && python3 ./build_scripts/build_usd.py -v ${BUILD_TARGET} USD_emscripten
