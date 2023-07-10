FROM ubuntu:20.04
SHELL ["/bin/bash", "-c"]

ARG CMAKE_VERSION=3.20.3
ARG EMSCRIPTEN_VERSION=2.0.24

RUN apt-get -y update && apt-get install -y\
        python-setuptools \
        libglew-dev \
        libxrandr-dev \
        libxcursor-dev \
        libxinerama-dev \
        libxi-dev \        
        python3-pip \
        git \
        curl

RUN curl -fsSL https://deb.nodesource.com/setup_current.x | bash - && DEBIAN_FRONTEND=noninteractive apt-get install -y nodejs libglib2.0-0

RUN pip install --upgrade pip
RUN pip install PySide2 PyOpenGL Jinja2

# Updating cmake version
RUN apt remove cmake
RUN pip install cmake==${CMAKE_VERSION}

RUN mkdir -p tmp && cd tmp && git clone --recursive https://github.com/emscripten-core/emsdk
RUN cd /tmp/emsdk && ./emsdk install ${EMSCRIPTEN_VERSION}
RUN cd /tmp/emsdk && ./emsdk activate ${EMSCRIPTEN_VERSION} --permanent

COPY . /src/
RUN source /tmp/emsdk/emsdk_env.sh && cd /src && python3 ./build_scripts/build_usd.py --emscripten USD_emscripten
