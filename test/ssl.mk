# ssl.mk
#
# SSL and TLS   make-functions
#

SSL_O    ?= example.com
SSL_KEY  ?= rsa:2048 # rsa:2048 rsa:4096
SSL_MAIL ?=
SSL_PASS ?= secret
SSL_SAN  ?=
SSL_TRST ?=

#
# Usage: OpenLDAP
#
#SSL_O     = $(AD_DOM)
#target: ssl/auth.crt ssl/demo.crt

#
# Usage: SMIME
#
#SSL_O     = $(MAIL_DOMAIN)
#SSL_MAIL  = auto
#SSL_PASS  = $(AD_USR_PW)
##SSL_TRST  = $(SSL_SMIME)
#target: ssl/$(AD_USR_CN)@$(MAIL_DOMAIN).p12
SSL_SMIME =  -setalias "Self Signed SMIME" -addtrust emailProtection \
	-addreject clientAuth -addreject serverAuth

#
# Usage: SUbject Alternate Name SAN
#
#SSL_O     = example.com
#SSL_SAN   = "subjectAltName=DNS:auth,DNS:*.docker"
#target: ssl/auth.crt


#
# $(call ssl_subj,root,example.com,) -> -subj "/CN=root/O=example.com"
# $(call ssl_subj,root,example.com,auto) -> -subj "/CN=root/O=example.com/emailAddress=root@example.com"
# $(call ssl_subj,root,example.com,admin@my.org) -> -subj "/CN=root/O=example.com/emailAddress=admin@my.org"
#
ssl_subj  = -subj "/CN=$(1)/O=$(2)$(if $(3),/emailAddress=$(if $(findstring @,$(3)),$(3),$(1)@$(2)),)"

#
# $(call ssl_extfile,"subjectAltName=DNS:auth") -> -extfile <(printf "subjectAltName=DNS:auth")
#
ssl_extfile = $(if $(1),-extfile <(printf $(1)),)


.PRECIOUS: %.crt %.csr %.key
SHELL   = /bin/bash

#
# Personal information exchange file PKCS#12
#
%.p12: %.crt
	openssl pkcs12 -export -in $< -inkey $*.key -out $@ \
	-passout pass:$(SSL_PASS)

#
# Certificate PEM
#
%.crt: %.csr ssl/ca.crt
	openssl x509 -req -in $< -CA $(@D)/ca.crt -CAkey $(@D)/ca.key -out $@ \
	$(call ssl_extfile,$(SSL_SAN)) $(SSL_TRST) -CAcreateserial

#
# Certificate signing request PEM
#
%.csr: ssl
	openssl req -new -newkey $(SSL_KEY) -nodes -keyout $*.key -out $@ \
	$(call ssl_subj,$(*F),$(SSL_O),$(SSL_MAIL))

#
# Certificate authority certificate PEM
#
ssl/ca.crt: ssl
	openssl req -x509 -new -newkey $(SSL_KEY) -nodes -keyout ssl/ca.key -out $@ \
	$(call ssl_subj,root,$(SSL_O),$(SSL_MAIL))

#
# SSL directory
#
ssl:
	mkdir -p $@

#
# Remove all files in SSL directory
#
ssl-destroy:
	rm -f ssl/*

#
# Inspect all files in SSL directory
#
ssl-list:
	@for file in $$(ls ssl/*); do \
	case $$file in \
	*.crt) \
	printf "\e[33;1m%s\e[0m\n" $$file; \
	openssl x509 -noout -issuer -subject -ext basicConstraints,keyUsage,extendedKeyUsage,subjectAltName -in $$file;; \
	*.csr) \
	printf "\e[33;1m%s\e[0m\n" $$file; \
	openssl req -noout -subject -in $$file;; \
	*.key) \
	printf "\e[33;1m%s\e[0m\n" $$file; \
	openssl rsa -text -noout -in $$file | head -n 1;; \
	esac \
	done

ssl-inspect:
	@for file in $$(ls ssl/*); do \
	case $$file in \
	*.crt) \
	printf "\e[33;1m%s\e[0m " $$file; \
	openssl x509 -text -noout -certopt no_sigdump,no_pubkey -in $$file;; \
	*.csr) \
	printf "\e[33;1m%s\e[0m " $$file; \
	openssl req -text -noout -reqopt no_sigdump,no_pubkey,ext_default -in $$file;; \
	*.key) \
	printf "\e[33;1m%s\e[0m " $$file; \
	openssl rsa -text -noout -in $$file | head -n 1;; \
	esac \
	done
