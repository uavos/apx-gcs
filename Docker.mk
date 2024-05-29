DOCKER_IMAGE := apx-gcs-linux
DOCKER_PROJECT_DIR := /gcs

DOCKER_FUSE := --cap-add SYS_ADMIN --cap-add MKNOD --device /dev/fuse:mrw --privileged

docker-image:
	@docker stop $(DOCKER_IMAGE); docker rm -f $(DOCKER_IMAGE); echo ""
	docker build -t uavos/$(DOCKER_IMAGE) - < Dockerfile

docker-images:
	@docker stop $(DOCKER_IMAGE); docker rm -f $(DOCKER_IMAGE); echo ""
	docker buildx build --push --platform linux/amd64,linux/arm64 -t uavos/$(DOCKER_IMAGE) .


docker-run:
	@docker run -it -v $(realpath $(CURDIR))/:$(DOCKER_PROJECT_DIR) -w $(DOCKER_PROJECT_DIR) $(DOCKER_FUSE) uavos/$(DOCKER_IMAGE) bash

docker-push:
	@docker push uavos/$(DOCKER_IMAGE)

docker-commit:
	@docker commit $(DOCKER_IMAGE) uavos/$(DOCKER_IMAGE)

docker-%:
	@docker run -it -v $(realpath $(CURDIR))/:$(DOCKER_PROJECT_DIR) -w $(DOCKER_PROJECT_DIR) $(DOCKER_FUSE) uavos/$(DOCKER_IMAGE) make $*
