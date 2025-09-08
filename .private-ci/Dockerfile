FROM domwst/nsjail AS nsjail

FROM rustlang/rust:nightly

COPY --from="nsjail" /usr/bin/nsjail /usr/bin/nsjail

# https://github.com/google/nsjail/blob/master/Dockerfile#L3
RUN apt-get update && apt-get install -y \
    libc6 \
    libstdc++6 \
    libprotobuf32 \
    libnl-route-3-200

RUN apt-get install -y \
    cmake clang-19 clang-format g++
RUN apt-get clean && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/cc cc $(which gcc) 30
RUN update-alternatives --install /usr/bin/c++ c++ $(which g++) 30

RUN rustup component add clippy rustfmt
