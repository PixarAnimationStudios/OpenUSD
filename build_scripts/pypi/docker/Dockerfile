FROM quay.io/pypa/manylinux2014_x86_64:2022-05-14-b55b680

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en

WORKDIR /opt/USD

# XXX:
# The above manylinux2014 image contains CMake 3.22, but we require
# 3.27+ for Python 3.11 support. So we install 3.27 manually here.
RUN pipx install --force cmake==3.27.9

CMD bash
