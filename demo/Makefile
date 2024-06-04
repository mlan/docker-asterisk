-include    *.mk .env .init.env

SRV_NAME ?= tele

CNT_CLI  ?= asterisk -rvvvddd

SIPS_URL ?= $(call dkr_srv_ip,$(SRV_NAME)):5061

SMS_HOST ?= 127.0.0.1:8080
SMS_URL1 ?= /hook/1
SMS_URL2 ?= /hook/2
SMS_FROM ?= +15017122661
SMS_TO   ?= +12025550160
SMS_BODY ?= This is a test message, sent $$(date), with special chars: ☺ [$$\"\\];; and 4 additional lines:%0A1%0A2%0A3%0A4.


variables:
	make -pn | grep -A1 "^# makefile"| grep -v "^#\|^--" | sort | uniq

init: up

ps:
	docker compose ps

up:
	docker compose up -d

start:
	docker compose start

stop:
	docker compose stop

down:
	docker compose down

destroy:
	docker compose down -v

config:
	docker compose config

wait_%:
	sleep 10

pw:
	dd if=/dev/random count=1 bs=8 2>/dev/null | base64 | sed -e 's/=*$$//'
	#od -vAn -N4 -tu4 < /dev/urandom

sms0:
	curl -i $(SMS_HOST)$(SMS_URL1) -G \
	--data-urlencode "zd_echo=$(shell date)"
	@echo

sms1:
	curl -i $(SMS_HOST)$(SMS_URL1) -X POST \
	--data-urlencode "To=$(SMS_TO)" \
	--data-urlencode "From=$(SMS_FROM)" \
	--data "Body=$(SMS_BODY)"

sms2:
	curl -i $(SMS_HOST)$(SMS_URL2) -X POST \
	--data-urlencode "To=$(SMS_TO)" \
	--data-urlencode "From=$(SMS_FROM)" \
	--data "Body=$(SMS_BODY)"

logs:
	docker compose logs $(SRV_NAME)

sh:
	docker compose exec $(SRV_NAME) bash

cli:
	docker compose exec $(SRV_NAME) $(CNT_CLI)

diff:
	docker container diff $(call dkr_srv_cnt,$(SRV_NAME))

top:
	docker compose top $(SRV_NAME)

env:
	docker compose exec $(SRV_NAME) env

sv:
	docker compose exec $(SRV_NAME) sh -c 'sv status $$SVDIR/*'

nft:
	docker compose exec $(SRV_NAME) nft list ruleset

autoban:
	docker compose exec $(SRV_NAME) autoban show

htop: tools_install
	docker compose exec $(SRV_NAME) htop

ipcs:
	docker compose exec $(SRV_NAME) ipcs

logger:
	docker compose exec $(SRV_NAME) asterisk -rx \
	'logger add channel console notice,warning,error,verbose'

restart_%:
	docker compose exec $(SRV_NAME) sv restart $*

apk_list:
	docker compose exec $(SRV_NAME) /bin/sh -c 'apk info -sq $$(apk info -q) | sed -r "N;N;s/([^ ]+) installed size:\n([^ ]+) (.).*/\2\3\t\1/" | sort -h'

apk_add_%:
	docker compose exec $(SRV_NAME) apk -q --no-cache --update add $*

tools_install:
	docker compose exec $(SRV_NAME) apk --no-cache --update add \
	nano lsof htop openldap-clients bind-tools iputils strace util-linux \
	pulseaudio-utils alsa-utils

xdebug_install:
	$(eval PHP_VER=$(shell docker compose exec $(SRV_NAME) sh -c 'echo $$PHP_VER'))
	docker compose exec $(SRV_NAME) apk --no-cache --update add \
	$(PHP_VER)-pecl-xdebug
	docker compose exec $(SRV_NAME) sed -i '1 a xdebug.profiler_enable = 1' /etc/$(PHP_VER)/php.ini
	docker compose exec $(SRV_NAME) sed -i '2 a zend_extension=xdebug.so' /etc/$(PHP_VER)/conf.d/50_xdebug.ini
	docker compose exec $(SRV_NAME) sv restart websmsd autoband

xdebug_getdata:
	docker cp $(call dkr_srv_cnt,$(SRV_NAME)):tmp/. ../local/xdebug

sips_test:
	docker run --rm -it --network bridge drwetter/testssl.sh $(SIPS_URL) || true

sound_enable: pulse/socket
	cp -f $${PULSE_COOKIE-$$HOME/.config/pulse/cookie} pulse/cookie
	sudo mount --bind $$(pactl info | sed '1!d;s/.*:\s*//g') pulse/socket
	docker compose exec $(SRV_NAME) asterisk -rx 'module load chan_alsa.so' || true

sound_disable:
	rm pulse/cookie
	sudo umount pulse/socket

pulse/socket:
	mkdir -p pulse
	touch pulse/socket

sound_1:
	docker compose exec $(SRV_NAME) play -nq -t alsa synth 1 sine 300

sound_2:
	docker compose exec $(SRV_NAME) play -q /var/lib/asterisk/sounds/en/hello-world.gsm

sound_3: apk_add_alsa-utils
	docker compose exec $(SRV_NAME) aplay -Dpulse /usr/share/sounds/alsa/Front_Right.wav

sound_4: apk_add_pulseaudio-utils
	docker compose exec $(SRV_NAME) paplay /usr/share/sounds/alsa/Front_Center.wav

sound_5:
	docker compose exec $(SRV_NAME) asterisk -rx \
	'channel originate console/+12025550160@dp_entry_call_inout application playback hello-world'

sound_6:
	docker compose exec $(SRV_NAME) asterisk -rx 'console dial t@dp_ivr_recgreet'

