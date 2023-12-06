# bld.mk
#
# Docker build   make-functions
#

BLD_VER  ?= latest
BLD_TGT  ?= full
BLD_CMT  ?= HEAD

#
# $(call bld_tags,mini,) -> mini mini-1.2.3 mini-1.2 mini-1
# $(call bld_tags,full,) -> latest full 1.2.3 1.2 1 full-1.2.3 full-1.2 full-1
# $(call bld_tags,,) -> latest 1.2.3 1.2 1
#
# $(call bld_tags,mini,something) -> mini-something
# $(call bld_tags,full,something) -> something full-something
# $(call bld_tags,,something) -> something
#
# $(call bld_tags,mini,latest) -> mini
# $(call bld_tags,full,latest) -> latest full
# $(call bld_tags,,latest) -> latest
#
bld_tags = $(if $(2),\
$(call bld_ver,$(1),$(2)),\
$(call bld_ver,$(1),latest) $(call bld_ver,$(1),$(call bld_gittags)))

#
# $(call bld_ver,mini,something) -> mini-something
# $(call bld_ver,full,something) -> something full-something
# $(call bld_ver,,something) -> something
#
# $(call bld_ver,mini,latest) -> mini
# $(call bld_ver,full,latest) -> latest full
# $(call bld_ver,,latest) -> latest
#
bld_ver = $(if $(1),\
$(if $(findstring $(BLD_TGT),$(1)),\
$(if $(findstring latest,$(2)),latest $(1),$(2) $(addprefix $(1)-,$(2))),\
$(if $(findstring latest,$(2)),$(1),$(addprefix $(1)-,$(2)))),\
$(2))

#
# $(call bld_tag,full,) -> full
# $(call bld_tag,,) -> latest
#
# $(call bld_tag,full,something) -> full-something
# $(call bld_tag,,something) -> something
#
# $(call bld_tag,full,latest) -> full
# $(call bld_tag,,latest) -> latest
#
bld_tag = $(strip $(if $(1),\
$(if $(2),$(if $(findstring latest,$(2)),$(1),$(1)-$(2)),$(1)),\
$(if $(2),$(2),latest)))

#
# $(call bld_gittags,HEAD) -> 1.2.3 1.2 1
#
bld_gittags = $(subst v,,$(shell git tag --points-at $(BLD_CMT)))
