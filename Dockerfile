FROM rustlang/rust:nightly-slim

WORKDIR /public

RUN apt-get update && apt-get install -y cmake git clang-19

RUN update-alternatives --install /usr/bin/cc cc $(which clang-19) 30
RUN update-alternatives --install /usr/bin/c++ c++ $(which clang++-19) 30

ADD --keep-git-dir https://gitlab.com/oleg-shatov/hse-caos-public.git .

RUN ./common/tools/build_all_tasks.sh
