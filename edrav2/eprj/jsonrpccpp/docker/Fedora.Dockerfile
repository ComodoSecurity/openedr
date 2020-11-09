FROM fedora:latest
MAINTAINER Peter Spiess-Knafl <dev@spiessknafl.at>
ENV OS=fedora
RUN dnf -y install \
    gcc-c++ \
    jsoncpp-devel \
    libcurl-devel \
    libmicrohttpd-devel \
    catch-devel \
    git \
    cmake \
    make \
    argtable-devel \
    hiredis-devel \
    redis

RUN mkdir /app
COPY docker/build_test_install.sh /app
COPY . /app
RUN chmod a+x /app/build_test_install.sh
RUN cd /app && ./build_test_install.sh
