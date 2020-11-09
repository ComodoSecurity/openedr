FROM base/archlinux:latest
MAINTAINER Peter Spiess-Knafl <dev@spiessknafl.at>
ENV OS=arch
RUN pacman -Sy --noconfirm \
    sudo \
    sed \
    grep \
    awk \
    fakeroot \
    wget \
    cmake \
    make \
    gcc \
    git \
    jsoncpp \
    libmicrohttpd \
    curl \
    hiredis \
    redis

RUN sudo -u nobody mkdir /tmp/argtable && cd /tmp && git clone https://aur.archlinux.org/argtable.git && cd argtable && sudo -u nobody makepkg
RUN pacman -U --noconfirm /tmp/argtable/*.pkg.tar.xz
RUN mkdir /app
COPY docker/build_test_install.sh /app
COPY . /app
RUN chmod a+x /app/build_test_install.sh
RUN cd /app && ./build_test_install.sh
