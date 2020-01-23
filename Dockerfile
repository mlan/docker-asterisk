ARG	DIST=alpine
ARG	REL=latest


#
#
# target: mini
#
# asterisk, minimal
#
#

FROM	$DIST:$REL AS mini
LABEL	maintainer=mlan

ENV	DOCKER_RUNSV_DIR=/etc/service \
	DOCKER_PERSIST_DIR=/srv \
	DOCKER_BIN_DIR=/usr/local/bin \
	DOCKER_ENTRY_DIR=/etc/entrypoint.d \
	DOCKER_EXIT_DIR=/etc/exitpoint.d \
	DOCKER_PHP_DIR=/usr/share/php7 \
	DOCKER_SPOOL_DIR=/var/spool/asterisk \
	DOCKER_CONF_DIR=/etc/asterisk \
	DOCKER_LOG_DIR=/var/log/asterisk \
	DOCKER_LIB_DIR=/var/lib/asterisk \
	DOCKER_NFT_DIR=/var/lib/nftables \
	DOCKER_SEED_CONF_DIR=/usr/share/asterisk/config \
	DOCKER_SEED_NFT_DIR=/etc/nftables \
	DOCKER_SSL_DIR=/etc/ssl \
	SYSLOG_LEVEL=4 \
	SYSLOG_OPTIONS='-S -D'
ENV	DOCKER_MOH_DIR=${DOCKER_LIB_DIR}/moh \
	DOCKER_ACME_SSL_DIR=${DOCKER_SSL_DIR}/acme \
	DOCKER_AST_SSL_DIR=${DOCKER_SSL_DIR}/asterisk

#
# Copy utility scripts including entrypoint.sh to image
#

COPY	src/*/bin $DOCKER_BIN_DIR/
COPY	src/*/entrypoint.d $DOCKER_ENTRY_DIR/
COPY	src/*/exitpoint.d $DOCKER_EXIT_DIR/
COPY	src/*/php $DOCKER_PHP_DIR/
COPY	dep/*/php $DOCKER_PHP_DIR/
COPY	src/*/config $DOCKER_SEED_CONF_DIR/
COPY	src/*/nft $DOCKER_SEED_NFT_DIR/

#
# Install
#
#
#

RUN	mkdir -p ${DOCKER_PERSIST_DIR}${DOCKER_SPOOL_DIR} \
	${DOCKER_PERSIST_DIR}${DOCKER_CONF_DIR} \
	${DOCKER_PERSIST_DIR}${DOCKER_LOG_DIR} \
	${DOCKER_PERSIST_DIR}${DOCKER_MOH_DIR} \
	${DOCKER_PERSIST_DIR}${DOCKER_NFT_DIR} \
	${DOCKER_PERSIST_DIR}${DOCKER_ACME_SSL_DIR} \
	${DOCKER_PERSIST_DIR}${DOCKER_AST_SSL_DIR} \
	${DOCKER_LIB_DIR} \
	${DOCKER_SSL_DIR} \
	&& ln -sf ${DOCKER_PERSIST_DIR}${DOCKER_SPOOL_DIR} $DOCKER_SPOOL_DIR \
	&& ln -sf ${DOCKER_PERSIST_DIR}${DOCKER_CONF_DIR} $DOCKER_CONF_DIR \
	&& ln -sf ${DOCKER_PERSIST_DIR}${DOCKER_LOG_DIR} $DOCKER_LOG_DIR \
	&& ln -sf ${DOCKER_PERSIST_DIR}${DOCKER_MOH_DIR} $DOCKER_MOH_DIR \
	&& ln -sf ${DOCKER_PERSIST_DIR}${DOCKER_NFT_DIR} $DOCKER_NFT_DIR \
	&& ln -sf ${DOCKER_PERSIST_DIR}${DOCKER_ACME_SSL_DIR} $DOCKER_ACME_SSL_DIR \
	&& ln -sf ${DOCKER_PERSIST_DIR}${DOCKER_AST_SSL_DIR} $DOCKER_AST_SSL_DIR \
	&& apk --no-cache --update add \
	asterisk

#
# Entrypoint, how container is run
#

ENTRYPOINT ["entrypoint.sh"]
CMD	["asterisk", "-fp"]


#
#
# target: base
#
# Configure Runit, a process manager
#
#
#

FROM	mini AS base

#
# Install
#

RUN	apk --no-cache --update add \
	asterisk-curl \
	asterisk-speex \
	asterisk-srtp \
	sox \
	openssl \
	curl \
	php7 \
	php7-curl \
	php7-json \
	runit \
	bash \
	nftables \
	jq \
	&& setup-runit.sh \
	"syslogd -n -O - -l $SYSLOG_LEVEL $SYSLOG_OPTIONS" \
	"crond -f -c /etc/crontabs" \
	"-q asterisk -pf" \
	"-n websmsd php -S 0.0.0.0:80 -t $DOCKER_PHP_DIR websmsd.php" \
	"$DOCKER_PHP_DIR/autoband.php" \
	&& mkdir -p /var/spool/asterisk/staging

#
# Have runit's runsvdir start all services
#

CMD	runsvdir -P ${DOCKER_RUNSV_DIR}

#
# Check if all services are running
#

HEALTHCHECK CMD sv status ${DOCKER_RUNSV_DIR}/*


#
#
# target: full
#
# Add sounds and configure ALSA pluging to PulseAudio
#
#

FROM	base AS full

#
# Install

RUN	apk --no-cache --update add \
	asterisk-alsa \
	alsa-plugins-pulse \
	asterisk-sounds-en

#
#
# target: extra
#
#
#
#

FROM	full AS xtra

#
# Install

RUN	apk --no-cache --update add \
	asterisk-cdr-mysql \
	asterisk-dahdi \
	asterisk-doc \
	asterisk-fax \
	asterisk-mobile \
	asterisk-odbc \
	asterisk-pgsql \
	asterisk-tds \
	asterisk-dbg \
	asterisk-dev \
	asterisk-sounds-moh \
	man



