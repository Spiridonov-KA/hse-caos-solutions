FROM rustlang/rust:nightly-slim

WORKDIR /public

RUN apt-get update && apt-get install -y cmake git

RUN git clone https://gitlab.com/oleg-shatov/hse-caos-public.git .

RUN ./common/tools/build_all_tasks.sh
