FROM rustlang/rust:nightly

RUN apt-get update && apt-get install -y \
    cmake clang-19 g++

RUN rustup component add clippy
