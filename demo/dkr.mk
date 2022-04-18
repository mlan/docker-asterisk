# dkr.mk
#
# Container   make-functions
#

#
# $(call dkr_srv_cnt,app) -> d03dda046e0b90c...
#
dkr_srv_cnt = $(shell docker-compose ps -q $(1) | head -n1)
#
# $(call dkr_cnt_ip,demo_app_1) -> 172.28.0.3
#
dkr_cnt_ip   = $(shell docker inspect -f \
	'{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' \
	$(1) | head -n1)
#
# $(call dkr_srv_ip,app) -> 172.28.0.3
#
dkr_srv_ip   = $(shell docker inspect -f \
	'{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' \
	$$(docker-compose ps -q $(1)) | head -n1)
#
#cnt_ip_old = $(shell docker inspect -f \
#	'{{range .NetworkSettings.Networks}}{{println .IPAddress}}{{end}}' \
#	$(1) | head -n1)

#
# List IPs of containers
#
ip-list:
	@for srv in $$(docker ps --format "{{.Names}}"); do \
	echo $$srv $$(docker inspect -f \
	'{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $$srv); \
	done | column -t
