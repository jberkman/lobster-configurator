# -*- Makefile -*-

bin_PROGRAMS += lobster-configurator

lobster_configurator_SOURCES :=			\
	callbacks.c				\
	callbacks.h				\
	interface.c				\
	interface.h				\
	lobster.c				\
	lobster.h				\
	lobsterio.c				\
	lobsterio.h				\
	main.c					\
	support.c				\
	support.h

lobster_configurator_CFLAGS :=				       \
    -DPACKAGE_DATA_DIR=\""$(datadir)"\"			       \
    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
    $(PACKAGE_CFLAGS)

lobster_configurator_LDADD := $(PACKAGE_LIBS) $(INTLLIBS)

dist_noinst_DATA +=				\
	autogen.sh				\
	intltool-extract.in			\
	intltool-merge.in			\
	intltool-update.in			\
	lobster-configurator.glade		\
	lobster-configurator.gladep		\
	lobster-configurator.spec

DISTCLEANFILES =				\
	intltool-extract			\
	intltool-merge				\
	intltool-update
