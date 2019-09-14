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

ENV	DOCKER_RUNIT_DIR=/etc/sv \
	DOCKER_PERSIST_DIR=/srv \
	DOCKER_BIN_DIR=/usr/local/bin \
	DOCKER_ENTRY_DIR=/etc/entrypoint.d \
	DOCKER_PHP_DIR=/usr/share/php7 \
	DOCKER_SPOOL_DIR=/var/spool/asterisk \
	DOCKER_CONF_DIR=/etc/asterisk \
	DOCKER_LOG_DIR=/var/log/asterisk \
	DOCKER_LIB_DIR=/var/log/asterisk \
	DOCKER_MOH_DIR=$DOCKER_LIB_DIR/moh \
	DOCKER_SEED_CONF_DIR=/usr/share/asterisk/config \
	SYSLOG_LEVEL=8 \
	SYSLOG_OPTIONS='-S -D'
#
# Copy utility scripts including entrypoint.sh to image
#

COPY	src/bin $DOCKER_BIN_DIR/
COPY	src/entrypoint.d $DOCKER_ENTRY_DIR/
COPY	src/php $DOCKER_PHP_DIR/
COPY	src/asterisk/config $DOCKER_SEED_CONF_DIR/

#
# Install
#
#
#

RUN	mkdir -p ${DOCKER_PERSIST_DIR}${DOCKER_SPOOL_DIR} \
	${DOCKER_PERSIST_DIR}${DOCKER_CONF_DIR} \
	${DOCKER_PERSIST_DIR}${DOCKER_LOG_DIR} \
	${DOCKER_PERSIST_DIR}${DOCKER_MOH_DIR} \
	${DOCKER_LIB_DIR} \
	&& ln -sf ${DOCKER_PERSIST_DIR}${DOCKER_SPOOL_DIR} $DOCKER_SPOOL_DIR \
	&& ln -sf ${DOCKER_PERSIST_DIR}${DOCKER_CONF_DIR} $DOCKER_CONF_DIR \
	&& ln -sf ${DOCKER_PERSIST_DIR}${DOCKER_LOG_DIR} $DOCKER_LOG_DIR \
	&& ln -sf ${DOCKER_PERSIST_DIR}${DOCKER_MOH_DIR} $DOCKER_MOH_DIR \
	&& apk --no-cache --update add \
	asterisk

#
# Rudimentary healthcheck
#

#HEALTHCHECK CMD asterisk status || exit 1

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
	&& setup-runit.sh \
	"syslogd -n -O - -l $SYSLOG_LEVEL $SYSLOG_OPTIONS" \
	"crond -f -c /etc/crontabs" \
	"-q asterisk -pf" \
	"php -S 0.0.0.0:80 -t $DOCKER_PHP_DIR smsd.php" \
	&& mkdir -p /var/spool/asterisk/staging

CMD	runsvdir -P ${DOCKER_RUNIT_DIR}

#
#
# target: full
#
# 
#
#

FROM	base AS full

#
# Install

RUN	apk --no-cache --update add \
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
	asterisk-alsa \
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



