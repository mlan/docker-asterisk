-include    *.mk

BLD_ARG  ?= --build-arg DIST=alpine --build-arg REL=3.9
BLD_REPO ?= mlan/asterisk
BLD_VER  ?= latest
BLD_TGT  ?= full

IMG_REPO ?= $(BLD_REPO)
IMG_VER  ?= $(BLD_VER)
_version  = $(if $(findstring $(BLD_TGT),$(1)),$(2),$(if $(findstring latest,$(2)),$(1),$(1)-$(2)))
_ip       = $(shell docker inspect -f \
	'{{range .NetworkSettings.Networks}}{{println .IPAddress}}{{end}}' \
	$(1) | head -n1)
IMG_CMD  ?= /bin/bash

CNT_NAME ?= test-pbx
CNT_DOM  ?= example.com
CNT_HOST ?= pbx.$(CNT_DOM)
TST_SIPP ?= 5060
CNT_RTPP ?= 10000-10099
TST_SMSP ?= 8080
TST_SMSU ?= 127.0.0.1:$(TST_SMSP)/
TST_PORT ?= -p $(TST_SIPP):$(TST_SIPP)/udp \
	-p $(CNT_RTPP):$(CNT_RTPP)/udp \
	-p $(TST_SMSP):80
TST_XTRA ?= --cap-add SYS_PTRACE \
	--cap-add=NET_ADMIN \
	--cap-add=NET_RAW \
	-e ASTERISK_SMSD_DEBUG=true -e SYSLOG_LEVEL=8
CNT_ENV  ?= --hostname $(CNT_HOST) $(TST_PORT) $(TST_XTRA)
CNT_VOL  ?=
CNT_CMD  ?= asterisk -pf -vvvddd
CNT_CLI  ?= asterisk -r -vvvddd
CNT_DRV  ?=
CNT_TZ   ?= UTC
CNT_IP    = $(shell docker inspect -f \
	'{{range .NetworkSettings.Networks}}{{println .IPAddress}}{{end}}' \
	$(1) | head -n1)

SMS_FROM ?= +15017122661
SMS_TO   ?= +15558675310
SMS_ACCT ?= 10T9M37yvSc
SMS_BODY ?= This is the ship that made the Kessel Run in fourteen parsecs?

TST_W8S1 ?= 1
TST_W8S2 ?= 40
TST_W8L1 ?= 20
TST_W8L2 ?= 120

.PHONY:

build-all: build_mini build_base build_full build_xtra

build: Dockerfile dep/autoban/php/ami.class.inc
	docker build $(BLD_ARG) --target $(BLD_TGT) -t $(BLD_REPO):$(BLD_VER) .

build_%: Dockerfile dep/autoban/php/ami.class.inc
	docker build $(BLD_ARG) --target $* -t $(BLD_REPO):$(call _version,$*,$(BLD_VER)) .

dep/autoban/php/ami.class.inc:
	mkdir -p dep/autoban/php
	wget -O dep/autoban/php/ami.class.inc https://raw.githubusercontent.com/ofbeaton/phpami/master/src/Ami.php

variables:
	make -pn | grep -A1 "^# makefile"| grep -v "^#\|^--" | sort | uniq

ps:
	docker ps -a

pw:
	dd if=/dev/random count=1 bs=8 2>/dev/null | base64 | sed -e 's/=*$$//'
	#od -vAn -N4 -tu4 < /dev/urandom

prune:
	docker image prune
	docker container prune
	docker volume prune
	docker network prune

test-up: test-up_0
	

test-up_0:
	#
	# test (0) run with defaults
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

test-upgrade:
#	docker cp src/*/bin/. $(CNT_NAME):/usr/local/bin
#	docker cp src/*/entrypoint.d/. $(CNT_NAME):/etc/entrypoint.d
	docker cp src/asterisk/php/. $(CNT_NAME):/usr/share/php7
	docker cp src/websms/php/. $(CNT_NAME):/usr/share/php7
#	docker cp dep/*/php/. $(CNT_NAME):/usr/share/php7
#	docker cp src/*/config/. $(CNT_NAME):/etc/asterisk
#	docker cp src/*/nft/. $(CNT_NAME):/var/lib/nftables

test-smsd1:
	curl -i $(TST_SMSU) -X POST \
	--data-urlencode "caller_did=$(SMS_FROM)" \
	--data-urlencode "caller_id=$(SMS_TO)" \
	--data-urlencode "text=$(SMS_BODY)" \
	--data-urlencode "account_sid=$(SMS_ACCT)"

test-smsd2:
	curl -i $(TST_SMSU) -X POST \
	--data-urlencode "To=$(SMS_FROM)" \
	--data-urlencode "From=$(SMS_TO)" \
	--data-urlencode "Body=$(SMS_BODY)"

test-smsd3:
	curl -i $(TST_SMSU) -G \
	--data-urlencode "zd_echo=$(shell date)"

test-down:
	docker stop $(CNT_NAME) 2>/dev/null || true
	docker rm $(CNT_NAME) 2>/dev/null || true

test-start:
	docker start $(CNT_NAME)

test-logs:
	docker container logs $(CNT_NAME)

test-sh:
	docker exec -it $(CNT_NAME) bash

test-cli:
	docker exec -it $(CNT_NAME) $(CNT_CLI)

test-diff:
	docker container diff $(CNT_NAME)

test-top:
	docker container top $(CNT_NAME)

test-nft:
	docker exec -it $(CNT_NAME) nft list ruleset

test-nft_watch:
	docker exec -it $(CNT_NAME) nft list set inet autoban watch

test-htop: test-debugtools
	docker exec -it $(CNT_NAME) htop

test-debugtools:
	docker exec -it $(CNT_NAME) apk --no-cache --update add \
	nano less lsof htop openldap-clients bind-tools iputils strace

test-tz:
	docker cp /usr/share/zoneinfo/$(CNT_TZ) $(CNT_NAME):/etc/localtime
	docker exec -it $(CNT_NAME) sh -c 'echo $(CNT_TZ) > /etc/timezone'
