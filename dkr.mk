# dkr.mk
#
# Container   make-functions
#

#
# $(call dkr_srv_cnt,app) -> d03dda046e0b90c...
#
dkr_srv_cnt = $(shell docker compose ps -q $(1) | head -n1)
#
# $(call dkr_cnt_ip,demo-app-1) -> 172.28.0.3
#
dkr_cnt_ip   = $(shell docker inspect -f \
	'{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' \
	$(1) | head -n1)
#
# $(call dkr_srv_ip,app) -> 172.28.0.3
#
dkr_srv_ip   = $(shell docker inspect -f \
	'{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' \
	$$(docker compose ps -q $(1)) | head -n1)
#
# $(call dkr_cnt_pid,demo-app-1) -> 9755
#
dkr_cnt_pid  = $(shell docker inspect --format '{{.State.Pid}}' $(1))
#
#cnt_ip_old = $(shell docker inspect -f \
#	'{{range .NetworkSettings.Networks}}{{println .IPAddress}}{{end}}' \
#	$(1) | head -n1)

#
# $(call dkr_img_env,image,envvar) -> value
#
dkr_img_env  = $(shell docker inspect -f \
	'{{range .Config.Env}}{{println .}}{{end}}' $(1) | grep -P "^$(2)=" | sed 's/[^=]*=//'

#
# $(call dkr_cnt_state,demo-app-1) -> docker inspect -f '{{.State.Status}}' demo-app-1
#
dkr_cnt_state = docker inspect -f '{{.State.Status}}' $(1)

#
# $(call dkr_cnt_wait_run,test-db,180) -> i=0; time while ! [ "$(docker inspect -f '{{.State.Status}}'  test-db)" = "running" ]; do sleep 1; i=$((i+1)); if [[ $i > 180 ]]; then echo test-db timeout with state: $(docker inspect -f '{{.State.Status}}'  test-db); break; fi; done
#
dkr_cnt_wait_run = i=0; time while ! [ "$$($(call dkr_cnt_state, $(1)))" = "running" ]; do sleep 1; i=$$((i+1)); if [[ $$i > $(2) ]]; then echo $(1) timeout with state: $$($(call dkr_cnt_state, $(1))); break; fi; done

#
# $(call dkr_srv_wait_run,180,app) -> wait up to 180s for app to enter state running
#
dkr_srv_wait_run = $(call dkr_cnt_wait_run,$(call dkr_srv_cnt $(1)),$(2))

#
# $(call dkr_cnt_wait_log,app,ready for connections) -> time docker logs -f app | sed -n '/ready for connections/{p;q}'
#
dkr_cnt_wait_log = time docker logs -f $(1) 2>&1 | sed -n '/$(2)/{p;q}'

#
# $(call dkr_pull_missing,mariadb:latest) -> if ! docker image inspect mariadb:latest &>/dev/null; then docker pull mariadb:latest; fi
#
dkr_pull_missing = if ! docker image inspect $(1) &>/dev/null; then docker pull $(1); fi

#
# List IPs of containers
#
ip-list:
	@for srv in $$(docker ps --format "{{.Names}}"); do \
	echo $$srv $$(docker inspect -f \
	'{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $$srv); \
	done | column -t
