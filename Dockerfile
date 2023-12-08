ARG 	DIST=alpine
ARG 	REL=latest


#
#
# target: mini
#
# asterisk, minimal
#
#

FROM	$DIST:$REL AS mini
ARG 	PHP_VER=php82
LABEL	maintainer=mlan

ENV	PHP_VER=$PHP_VER \
	SVDIR=/etc/service \
	DOCKER_PERSIST_DIR=/srv \
	DOCKER_BIN_DIR=/usr/local/bin \
	DOCKER_ENTRY_DIR=/etc/docker/entry.d \
	DOCKER_EXIT_DIR=/etc/docker/exit.d \
	DOCKER_PHP_DIR=/usr/share/$PHP_VER \
	DOCKER_SPOOL_DIR=/var/spool/asterisk \
	DOCKER_CONF_DIR=/etc/asterisk \
	DOCKER_LOG_DIR=/var/log/asterisk \
	DOCKER_LIB_DIR=/var/lib/asterisk \
	DOCKER_DL_DIR=/usr/lib/asterisk/modules \
	DOCKER_NFT_DIR=/etc/nftables.d \
	DOCKER_SEED_CONF_DIR=/usr/share/asterisk/config \
	DOCKER_SEED_NFT_DIR=/usr/share/nftables \
	DOCKER_SSL_DIR=/etc/ssl \
	ACME_POSTHOOK="sv restart asterisk" \
	SYSLOG_LEVEL=4 \
	SYSLOG_OPTIONS=-SDt \
	WEBSMSD_PORT=80
ENV	DOCKER_MOH_DIR=$DOCKER_LIB_DIR/moh \
	DOCKER_ACME_SSL_DIR=$DOCKER_SSL_DIR/acme \
	DOCKER_APPL_SSL_DIR=$DOCKER_SSL_DIR/asterisk

#
# Copy utility scripts including docker-entrypoint.sh to image
#

COPY	src/*/bin $DOCKER_BIN_DIR/
COPY	src/*/entry.d $DOCKER_ENTRY_DIR/
COPY	src/*/exit.d $DOCKER_EXIT_DIR/
COPY	src/*/php $DOCKER_PHP_DIR/
COPY	sub/*/php $DOCKER_PHP_DIR/
COPY	src/*/config $DOCKER_SEED_CONF_DIR/
COPY	src/*/nft $DOCKER_SEED_NFT_DIR/

#
# Facilitate persistent storage and install asterisk
#

RUN	source docker-common.sh \
	&& source docker-config.sh \
	&& dc_persist_dirs \
	$DOCKER_APPL_SSL_DIR \
	$DOCKER_CONF_DIR \
	$DOCKER_LOG_DIR \
	$DOCKER_MOH_DIR \
	$DOCKER_NFT_DIR \
	$DOCKER_SPOOL_DIR \
	&& mkdir -p $DOCKER_ACME_SSL_DIR \
	&& ln -sf $DOCKER_PHP_DIR/autoban.php $DOCKER_BIN_DIR/autoban \
	&& ln -sf $DOCKER_PHP_DIR/websms.php $DOCKER_BIN_DIR/websms \
	&& apk --no-cache --update add \
	asterisk

#
# Entrypoint, how container is run
#

ENTRYPOINT ["docker-entrypoint.sh"]
CMD	["asterisk", "-fp"]


#
#
# target: base
#
# asterisk add-ons: WebSMS and AutoBan
#
#

FROM	mini AS base

#
# Install packages used by the add-ons and register services
#

RUN	apk --no-cache --update add \
	asterisk-curl \
	asterisk-speex \
	asterisk-srtp \
	openssl \
	curl \
	$PHP_VER \
	$PHP_VER-curl \
	$PHP_VER-json \
	runit \
	bash \
	nftables \
	jq \
	&& ln -sf /usr/bin/$PHP_VER /usr/bin/php \
	&& docker-service.sh \
	"syslogd -nO- -l$SYSLOG_LEVEL $SYSLOG_OPTIONS" \
	"crond -f -c /etc/crontabs" \
	"-q asterisk -pf" \
	"-n websmsd php -S 0.0.0.0:$WEBSMSD_PORT -t $DOCKER_PHP_DIR websmsd.php" \
	"$DOCKER_PHP_DIR/autoband.php" \
	&& mkdir -p /var/spool/asterisk/staging

#
# Have runit's runsvdir start all services
#

CMD	runsvdir -P ${SVDIR}

#
# Check if all services are running
#

HEALTHCHECK CMD sv status ${SVDIR}/*


#
#
# target: full
#
# Add sounds and configure ALSA pluging to PulseAudio
#
#

FROM	base AS full

#
# Copy patent-encumbered codecs to image
#

COPY	sub/*/module $DOCKER_DL_DIR/

#
# Install packages supporting audio
#

RUN	apk --no-cache --update add \
	asterisk-alsa \
	alsa-plugins-pulse \
	asterisk-sounds-en \
	sox

#
#
# target: extra
#
# all asterisk packages
#
#

FROM	full AS xtra

#
# Install all asterisk packages
#

RUN	apk --no-cache --update add \
	asterisk-doc \
	asterisk-fax \
	asterisk-mobile \
	asterisk-odbc \
	asterisk-pgsql \
	asterisk-tds \
	asterisk-dbg \
	asterisk-dev \
	asterisk-sounds-moh \
	man-pages
