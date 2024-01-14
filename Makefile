# Makefile
#
# build
#

-include    *.mk

BLD_ARG  ?= --build-arg DIST=alpine --build-arg REL=3.19 --build-arg PHP_VER=php82
BLD_REPO ?= mlan/asterisk
BLD_VER  ?= latest
BLD_TGT  ?= full
BLD_TGTS ?= mini base full xtra
BLD_CMT  ?= HEAD
BLD_CVER ?= ast160
BLD_DNLD ?= curl -o
BLD_KIT  ?= DOCKER_BUILDKIT=1

TST_REPO ?= $(BLD_REPO)
TST_VER  ?= $(BLD_VER)
TST_ENV  ?= -C test
TST_TGTE ?= $(addprefix test-,all diff down env htop logs sh sv up)
TST_INDX ?= 1 2 3 4
TST_TGTI ?= $(addprefix test_,$(TST_INDX)) $(addprefix test-up_,$(TST_INDX))

export TST_REPO TST_VER

push:
	#
	# PLEASE REVIEW THESE IMAGES WHICH ARE ABOUT TO BE PUSHED TO THE REGISTRY
	#
	@docker image ls $(BLD_REPO)
	#
	# ARE YOU SURE YOU WANT TO PUSH THESE IMAGES TO THE REGISTRY? [yN]
	@read input; [ "$${input}" = "y" ]
	docker push --all-tags $(BLD_REPO)

build-all: $(addprefix build_,$(BLD_TGTS))

build: build_$(BLD_TGT)

build_%: pre_build
	$(BLD_KIT) docker build $(BLD_ARG) --target $* \
	$(addprefix --tag $(BLD_REPO):,$(call bld_tags,$*,$(BLD_VER))) .

pre_build: Dockerfile pre_autoban pre_codecs
	

pre_autoban: sub/autoban/php/ami.class.inc
	

pre_codecs: codec_g723.so codec_g729.so
	

sub/autoban/php/ami.class.inc: submodule
	mkdir -p $(@D)
	ln -f sub/module/phpami/src/Ami.php $@

submodule:
	git submodule update --init --recursive

codec_%.so: sub/codecs/download/codec_%-$(BLD_CVER).so
	mkdir -p sub/codecs/module
	ln -f $< sub/codecs/module/$@

.PRECIOUS: sub/codecs/download/%.so

sub/codecs/download/%.so:
	mkdir -p $(@D)
	$(BLD_DNLD) $@ http://asterisk.hosting.lv/bin/$*-gcc4-glibc-x86_64-core2.so
	chmod 0755 $@

variables:
	make -pn | grep -A1 "^# makefile"| grep -v "^#\|^--" | sort | uniq

ps:
	docker ps -a

prune:
	docker image prune -f

clean:
	docker images | grep $(BLD_REPO) | awk '{print $$1 ":" $$2}' | uniq | xargs docker rmi || true

$(TST_TGTE):
	${MAKE} $(TST_ENV) $@

$(TST_TGTI):
	${MAKE} $(TST_ENV) $@
