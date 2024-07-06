# Use AlmaLinux as the base image
FROM almalinux:8.10

# Install basic tools and dependencies
RUN dnf update -y && dnf install -y \
    git \
    cmake \
    make \
    python3-devel \
    python3-libs \
    boost-devel \
    tbb-devel \
    openssl-devel \
    libzstd-devel \
    libX11-devel \
    libXext-devel \
    libXt-devel \
    libXrandr-devel \
    libXinerama-devel \
    libXcursor-devel \
    libXi-devel \
    libXfixes-devel \
    libXrender-devel \
    libXdamage-devel \
    libXcomposite-devel \
    libxkbcommon-devel \
    libdrm-devel \
    mesa-libGL-devel \
    mesa-libGLU-devel \
    zlib-devel \
    wget \
    autoconf \
    automake \
    libtool \
    pkgconfig \
    zip \
    which \
    java-21-openjdk-devel \
    && dnf clean all

# Remove system GCC
RUN dnf remove -y gcc gcc-c++

# Install GCC 9
RUN dnf install -y dnf-plugins-core && \
    dnf config-manager --set-enabled powertools && \
    dnf install -y gcc-toolset-9-gcc gcc-toolset-9-gcc-c++ gcc-toolset-9-binutils

# Create symlinks to make GCC 9 the default
RUN ln -sf /opt/rh/gcc-toolset-9/root/usr/bin/gcc /usr/bin/gcc && \
    ln -sf /opt/rh/gcc-toolset-9/root/usr/bin/g++ /usr/bin/g++ && \
    ln -sf /opt/rh/gcc-toolset-9/root/usr/bin/c++ /usr/bin/c++ && \
    ln -sf /opt/rh/gcc-toolset-9/root/usr/bin/cc  /usr/bin/cc

# Set environment variables
ENV PATH="/opt/rh/gcc-toolset-9/root/usr/bin:${PATH}" \
    LD_LIBRARY_PATH="/opt/rh/gcc-toolset-9/root/usr/lib64:/opt/rh/gcc-toolset-9/root/usr/lib:${LD_LIBRARY_PATH}"

# Verify GCC version
RUN gcc --version

# Install Python packages
RUN pip3 install --upgrade pip && \
    pip3 install PySide2 PySide6 PyOpenGL Jinja2

# Download and build USD dependencies
WORKDIR /tmp

# Build Boost
RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.zip && \
    unzip boost_1_76_0.zip && \
    cd boost_1_76_0 && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 -j$(nproc) install && \
    cd .. && rm -rf boost_1_76_0*

# Install xorg-macros from source
RUN wget https://www.x.org/releases/individual/util/util-macros-1.19.3.tar.gz && \
    tar xf util-macros-1.19.3.tar.gz && \
    cd util-macros-1.19.3 && \
    ./configure --prefix=/usr/local && \
    make && \
    make install && \
    cd .. && rm -rf util-macros-1.19.3 util-macros-1.19.3.tar.gz

# Install libXxf86vm from source
RUN wget https://www.x.org/releases/individual/lib/libXxf86vm-1.1.5.tar.gz && \
    tar xf libXxf86vm-1.1.5.tar.gz && \
    cd libXxf86vm-1.1.5 && \
    ./configure --prefix=/usr/local && \
    make && \
    make install && \
    cd .. && rm -rf libXxf86vm-1.1.5 libXxf86vm-1.1.5.tar.gz

# Build and install OpenSubdiv
RUN wget https://github.com/PixarAnimationStudios/OpenSubdiv/archive/v3_5_0.tar.gz && \
    tar xf v3_5_0.tar.gz && \
    cd OpenSubdiv-3_5_0 && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DNO_OPENCL=ON -DNO_CUDA=ON -DNO_PTEX=ON -DNO_DOC=ON -DNO_TUTORIALS=ON -DNO_REGRESSION=ON && \
    make -j$(nproc) && \
    make install && \
    cd ../.. && rm -rf OpenSubdiv-3_5_0 v3_5_0.tar.gz

# Build and install OpenEXR
RUN wget https://github.com/AcademySoftwareFoundation/openexr/archive/refs/tags/v3.1.11.zip && \
    unzip v3.1.11.zip && \
    cd openexr-3.1.11 && \
    mkdir buildExr && cd buildExr && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DOPENEXR_INSTALL_TOOLS=OFF -DOPENEXR_INSTALL_EXAMPLES=OFF -DBUILD_TESTING=OFF && \
    make -j$(nproc) install && \
    cd ../.. && rm -rf openexr-3.1.11*

# Build and install HDF5
RUN wget https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.2/src/hdf5-1.12.2.tar.gz && \
    tar xf hdf5-1.12.2.tar.gz && \
    cd hdf5-1.12.2 && \
    ./configure --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    cd .. && rm -rf hdf5-1.12.2 hdf5-1.12.2.tar.gz

# Build and install Alembic
RUN wget https://github.com/alembic/alembic/archive/1.8.3.tar.gz && \
    tar xf 1.8.3.tar.gz && \
    cd alembic-1.8.3 && \
    mkdir buildAlembic && cd buildAlembic && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DUSE_HDF5=ON && \
    make -j$(nproc) && \
    make install && \
    cd ../.. && rm -rf alembic-1.8.3 1.8.3.tar.gz

# Set up environment variables
ENV HOME=/root
ENV PYTHONPATH=/usr/lib64/python3.6/:$PYTHONPATH
ENV PATH=/usr/local/bin:$PATH
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
RUN PATH="/usr/lib/jvm/java-21-openjdk/bin:$PATH"

# Install Bazel
RUN wget https://github.com/bazelbuild/bazel/releases/download/7.2.1/bazel_nojdk-7.2.1-linux-x86_64 -O /usr/local/bin/bazel && \
    chmod +x /usr/local/bin/bazel

# Verify Bazel installation
RUN bazel --version

# Set working directory
WORKDIR /usd

COPY . /usd

# Set the default command
CMD ["/usr/bin/bash"]
