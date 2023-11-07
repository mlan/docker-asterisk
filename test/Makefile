# Makefile
#
# test
#

-include    *.mk

TST_REPO ?= mlan/asterisk
TST_VER  ?= latest
TST_NAME ?= test

NET_NAME ?= $(TST_NAME)-net
NET_ENV  ?= --network $(NET_NAME)

CNT_LIST ?= pbx

PBX_NAME ?= $(TST_NAME)-pbx
PBX_DOM  ?= example.com
PBX_FQDN ?= pbx.$(PBX_DOM)
PBX_CMD  ?= asterisk -r
PBX_SIPP ?= 5060
PBX_SIPS ?= 5061
PBX_RTPP ?= 10000-10099
PBX_SMSP ?= 8080
EXP_ENV  ?= \
-p $(PBX_SIPP):$(PBX_SIPP)/udp \
-p $(PBX_SIPP):$(PBX_SIPP) \
-p $(PBX_SIPS):$(PBX_SIPS) \
-p $(PBX_RTPP):$(PBX_RTPP)/udp \
-p $(PBX_SMSP):80
CAP_ENV  ?= \
--cap-add SYS_PTRACE \
--cap-add=NET_ADMIN \
--cap-add=NET_RAW
PBX_ENV  ?= $(NET_ENV) $(EXP_ENV) $(CAP_ENV) $(LOC_ENV) \
--name $(PBX_NAME) \
--hostname $(PBX_FQDN) \
-e SYSLOG_LEVEL=8
PBXV_ENV ?= -v $(PBX_NAME):/srv

TST_W8S  ?= 5
TST_W8L  ?= 10

NET_SYSP := $(shell command -v nft)

ifdef NET_SYSP
	NET_RULE := nft list ruleset
else
	NET_RULE := iptables -t nat -nvL
endif

variables:
	make -pn | grep -A1 "^# makefile"| grep -v "^#\|^--" | sort | uniq

ps:
	docker ps -a

test-all: $(addprefix test_,1 2 3 4)
	

test_%: test-up_% test-wait_% test-logs_% test-autoban_% test-ping_% test-down_%
	

test-up_1: test-up-net
	#
	#
	#
	# test (1) run mini with defaults (is there smoke?)
	#
	#
	docker run -d $(PBX_ENV) $(TST_REPO):$(call bld_tag,mini,$(TST_VER))

test-up_2: test-up-net
	#
	#
	#
	# test (2) run base
	#
	#
	docker run -d $(PBX_ENV) $(TST_REPO):$(call bld_tag,base,$(TST_VER))

test-up_3: test-up-net
	#
	#
	#
	# test (3) run full
	#
	#
	docker run -d $(PBX_ENV) $(TST_REPO):$(call bld_tag,full,$(TST_VER))

test-up_4: test-up-net
	#
	#
	#
	# test (4) run xtra
	#
	#
	docker run -d $(PBX_ENV) $(TST_REPO):$(call bld_tag,xtra,$(TST_VER))

test-ping_%: test-version_%
	#
	#
	# test ($*) success ☺
	#
	#
	#

test-version_%:
	docker exec -it $(PBX_NAME) asterisk -x "pjsip show version" | grep PJPROJECT

test-autoban_%:
	case $* in [2-3]) docker exec -it $(PBX_NAME) autoban;; esac

test-logs_%:
	docker container logs $(PBX_NAME) | grep 'docker-entrypoint.sh' || true

test-wait_%:
	case $* in [1-2]) sleep $(TST_W8S);; [3-9]) sleep $(TST_W8L);; esac

test-up-net:
	docker network create $(NET_NAME) 2>/dev/null || true

test-down: test-down_0 test-down-net test-down-vol acme-destroy

test-down_%:
	docker rm -fv $(PBX_NAME) 2>/dev/null || true

test-down-net:
	docker network rm $(NET_NAME) 2>/dev/null || true

test-down-vol:
	docker volume rm $(PBX_NAME) 2>/dev/null || true


$(addprefix test-,diff env htop imap logs pop3 sh sv):
	${MAKE} $(patsubst test-%,pbx-%,$@)

$(addsuffix -sh,$(CNT_LIST)):
	docker exec -it $(patsubst %-sh,$(TST_NAME)-%,$@) sh -c 'exec $$(getent passwd root | sed "s/.*://g")'

$(addsuffix -cli,$(CNT_LIST)):
	docker exec -it $(patsubst %-cli,$(TST_NAME)-%,$@) $(PBX_CMD)

$(addsuffix -env,$(CNT_LIST)):
	docker exec -it $(patsubst %-env,$(TST_NAME)-%,$@) env

$(addsuffix -logs,$(CNT_LIST)):
	docker container logs $(patsubst %-logs,$(TST_NAME)-%,$@)

$(addsuffix -diff,$(CNT_LIST)):
	docker container diff $(patsubst %-diff,$(TST_NAME)-%,$@)

$(addsuffix -tools,$(CNT_LIST)):
	docker exec -it $(patsubst %-tools,$(TST_NAME)-%,$@) \
	apk --no-cache --update add \
	nano less lsof htop openldap-clients bind-tools iputils strace util-linux \
	pulseaudio-utils alsa-utils mariadb-client

$(addsuffix -htop,$(CNT_LIST)):
	docker exec -it $(patsubst %-htop,$(TST_NAME)-%,$@) htop

$(addsuffix -sv,$(CNT_LIST)):
	docker exec -it $(patsubst %-sv,$(TST_NAME)-%,$@) sh -c 'sv status $$SVDIR/*'

$(addsuffix -nsrule,$(CNT_LIST)):
	sudo nsenter -n -t $(call dkr_cnt_pid,$(patsubst %-nsrule,$(TST_NAME)-%,$@)) $(NET_RULE)

$(addsuffix -nsss,$(CNT_LIST)):
	sudo nsenter -n -t $(call dkr_cnt_pid,$(patsubst %-nsss,$(TST_NAME)-%,$@)) ss -utnlp

acme-destroy: ssl-destroy
	rm -f acme/*

acme/acme.json: $(SRV_CERT)
	bin/gen-acme-json.sh $(AD_USR_CN)@$(AD_DOM) $(SRV_FQDN) $(SRV_KEY) $(SRV_CERT) > $@
