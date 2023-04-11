FROM quay.io/pypa/manylinux2014_x86_64:2021-06-07-00faba2

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en

WORKDIR /opt/USD

# XXX:
# The above manylinux2014 image contains CMake 3.20, but we require
# 3.24+ for Python 3.10 support. Newer images include later cmake
# versions but for some reason (possibly the use of gcc 10?) wheels
# created from these images crash in TBB. So for now, we use this
# older image but install a newer CMake.
RUN pipx install --force cmake==3.24.3

CMD bash
