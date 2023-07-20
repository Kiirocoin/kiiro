# This is a Dockerfile for kiirocoind.
FROM debian:bullseye as build-image

# Install required system packages
RUN apt-get update && apt-get install -y \
    automake \
    bsdmainutils \
    curl \
    g++ \
    libtool \
    make \
    pkg-config \
    patch 

# Build Kiirocoin
COPY . /tmp/kiirocoin/

WORKDIR /tmp/kiirocoin

RUN cd depends && \
    NO_QT=true make HOST=$(uname -m)-linux-gnu -j$(nproc)

RUN ./autogen.sh && \
    ./configure --without-gui --enable-tests --prefix=/tmp/kiirocoin/depends/$(uname -m)-linux-gnu && \
    make -j$(nproc) && \
    make check && \
    make install

# extract shared dependencies of kiirocoind and kiirocoin-cli
# copy relevant binaries to /usr/bin, the COPY --from cannot use $(uname -m) variable in argument
RUN mkdir /tmp/ldd && \
    ./depends/ldd_copy.sh -b "./depends/$(uname -m)-linux-gnu/bin/kiirocoind" -t "/tmp/ldd" && \
    ./depends/ldd_copy.sh -b "./depends/$(uname -m)-linux-gnu/bin/kiirocoin-cli" -t "/tmp/ldd" && \
    cp ./depends/$(uname -m)-linux-gnu/bin/* /usr/bin/

FROM debian:bullseye-slim

COPY --from=build-image /usr/bin/kiirocoind /usr/bin/kiirocoind
COPY --from=build-image /usr/bin/kiirocoin-cli /usr/bin/kiirocoin-cli
COPY --from=build-image /tmp/ldd /tmp/ldd

# restore ldd files in correct paths
RUN cp --verbose -RT /tmp/ldd / && \
    rm -rf /tmp/ldd && \
    ldd /usr/bin/kiirocoind

# Create user to run daemon
RUN useradd -m -U kiirocoind
USER kiirocoind

RUN mkdir /home/kiirocoind/.kiirocoin
VOLUME [ "/home/kiirocoind/.kiirocoin" ]

# Main network ports
EXPOSE 8200
EXPOSE 8201

# Test network ports
EXPOSE 18200
EXPOSE 18201

# Regression test network ports
EXPOSE 18444
EXPOSE 28201

ENTRYPOINT ["/usr/bin/kiirocoind", "-printtoconsole"]

