GIT_BRANCH = $(shell git rev-parse --abbrev-ref HEAD)

GIT_REF_RELEASE := release

GIT_VERSION_CMD = git describe --tags --match="v*.*" 2> /dev/null |sed 's/^v//;s/\-/\./;s/\(.*\)-\(.*\)/\1/'
GIT_VERSION = $(shell $(GIT_VERSION_CMD))
GIT_BRANCH = $(shell git rev-parse --abbrev-ref HEAD)

version: 
	@echo "Current version: $(shell $(GIT_VERSION_CMD))"

branch: 
	@echo "Current branch: $(GIT_BRANCH)"

release: release-tags release-push

release-tags:
	@echo "Requesting release..."
	@if ! git diff-index --quiet HEAD ; then echo "Commit changes first"; exit 1; fi
	@git fetch --tags
	@if [[ $(shell git rev-list HEAD ^$(GIT_REF_RELEASE) --count) -lt 1 ]]; then echo "** NO CHANGES **" && exit 1; fi
	
	@echo "Updating tags..."
	@mkdir -p docs/releases
	@apx-changelog --ref $(GIT_REF_RELEASE) --title "APX Ground Control" \
		--log CHANGELOG.md --out docs/releases/release-$(GIT_VERSION).md
	@git add CHANGELOG.md docs/releases/release-$(GIT_VERSION).md
	@git commit -am "Release $(GIT_VERSION)"
	@git tag release-$(GIT_VERSION)
	

release-push:
	@echo "Release push..."
	git push origin $(GIT_BRANCH)
	git push origin $(GIT_BRANCH) --tags
	git fetch . $(GIT_BRANCH):$(GIT_REF_RELEASE) -f
	git push origin $(GIT_REF_RELEASE) -f
