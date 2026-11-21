SUBDIRS=		sw_vers

.PHONY: all clean show-config

all clean show-config:
	@for subdir in ${SUBDIRS}; do \
		${MAKE} -C "$$subdir" $@ || exit $$?; \
	done
