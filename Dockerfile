FROM domwst/caos-private-ci

WORKDIR /public

ADD --keep-git-dir https://gitlab.com/oleg-shatov/hse-caos-public.git .

RUN ./common/tools/build_all_tasks.sh
