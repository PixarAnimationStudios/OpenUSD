# The manylinux image is parameterized so the same Dockerfile serves every
# architecture the PyPI workflow builds for. Callers pass --build-arg ARCH=...;
# the default keeps existing x86_64 invocations working unchanged.
ARG ARCH=x86_64
ARG MANYLINUX_TAG=2025.12.07-1

FROM quay.io/pypa/manylinux_2_28_${ARCH}:${MANYLINUX_TAG}

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en

WORKDIR /opt/USD

CMD bash
