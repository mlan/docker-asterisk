-include    *.mk

BLD_ARG  ?= --build-arg DIST=alpine --build-arg REL=3.12
BLD_REPO ?= mlan/asterisk
BLD_VER  ?= latest
BLD_TGT  ?= full

IMG_REPO ?= $(BLD_REPO)
IMG_VER  ?= $(BLD_VER)
_version  = $(if $(findstring $(BLD_TGT),$(1)),$(2),$(if $(findstring latest,$(2)),$(1),$(1)-$(2)))
_ip       = $(shell docker inspect -f \
	'{{range .NetworkSettings.Networks}}{{println .IPAddress}}{{end}}' \
	$(1) | head -n1)

CNT_NAME ?= test-pbx
CNT_DOM  ?= example.com
CNT_HOST ?= pbx.$(CNT_DOM)
TST_NET  ?= test-net
TST_SIPP ?= 5060
TST_SIPS ?= 5061
CNT_RTPP ?= 10000-10099
TST_SMSP ?= 8080
TST_PORT ?= -p $(TST_SIPP):$(TST_SIPP)/udp \
	-p $(TST_SIPP):$(TST_SIPP) \
	-p $(TST_SIPS):$(TST_SIPS) \
	-p $(CNT_RTPP):$(CNT_RTPP)/udp \
	-p $(TST_SMSP):80
TST_XTRA ?= --cap-add SYS_PTRACE \
	--cap-add=NET_ADMIN \
	--cap-add=NET_RAW \
	-e SYSLOG_LEVEL=8
TST_CUST ?=
TST_W8   ?= 1
CNT_ENV  ?= --hostname $(CNT_HOST) $(TST_PORT) $(TST_XTRA) $(TST_CUST)
CNT_VOL  ?=
CNT_CMD  ?= asterisk -pf -vvvddd

.PHONY:

build-all: build_mini build_base build_full build_xtra

build: depends
	docker build $(BLD_ARG) --target $(BLD_TGT) -t $(BLD_REPO):$(BLD_VER) .

build_%: depends
	docker build $(BLD_ARG) --target $* -t $(BLD_REPO):$(call _version,$*,$(BLD_VER)) .

depends: Dockerfile sub/autoban/php/ami.class.inc
	

submodule:
	git submodule update --init --recursive

sub/autoban/php/ami.class.inc: submodule
	mkdir -p sub/autoban/php
	ln -f sub/module/phpami/src/Ami.php sub/autoban/php/ami.class.inc

variables:
	make -pn | grep -A1 "^# makefile"| grep -v "^#\|^--" | sort | uniq

ps:
	docker ps -a

prune:
	docker image prune -f

prune-all:
	docker image prune
	docker container prune
	docker volume prune
	docker network prune

test-all: test_0
	

test_%: test-up_% test-wait_% test-logs_% test-ping_% test-down_%
	

test-up_0:
	#
	# test (0) run with defaults (is there smoke?)
	#
	docker run -d --name $(CNT_NAME) $(CNT_ENV) \
		$(IMG_REPO):$(call _version,full,$(IMG_VER))

test-up_1:
	#
	# test (1) run with $(CNT_CMD)
	#
	docker run -d --name $(CNT_NAME) $(CNT_ENV) \
		$(IMG_REPO):$(call _version,full,$(IMG_VER)) $(CNT_CMD)

test-up_2:
	#
	# test (2) run using srv vol
	#
	docker run -d --name $(CNT_NAME) $(CNT_ENV) $(CNT_VOL) \
		$(IMG_REPO):$(call _version,full,$(IMG_VER))

test-up_3: test-up-net
	#
	# test (3) run using srv vol and test net
	#
	docker run -d --name $(CNT_NAME) $(CNT_ENV) $(CNT_VOL) \
		--network $(TST_NET) \
		$(IMG_REPO):$(call _version,$(BLD_TGT),$(IMG_VER))

test-ping_%: test-version_%
	#
	# test ($*) successful
	#

test-version_%:
	docker exec -it $(CNT_NAME) asterisk -x "pjsip show version" | grep PJPROJECT

test-logs_%:
	docker container logs $(CNT_NAME) | grep 'docker-entrypoint.sh' || true

test-wait_%:
	sleep $(TST_W8)

test-up-net:
	docker network create $(TST_NET) 2>/dev/null || true
	
test-down_%:
	docker rm -fv $(CNT_NAME) 2>/dev/null || true

test-down: test-down_%
	docker network rm $(TST_NET) 2>/dev/null || true
