ARG CI_IMAGE_NAME

FROM $CI_IMAGE_NAME

WORKDIR /public

ARG PUBLIC_REPO
ADD --keep-git-dir $PUBLIC_REPO .

RUN ./common/tools/build_all_tasks.sh
