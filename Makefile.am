#
# This is a non-recursive automake file.
#
# The style was inspired by Robert Collins, as mentioned here:
#
#     http://sources.redhat.com/ml/automake/2001-08/msg00061.html
#
# And the original Recursive Make Considered Harmful:
#
#     http://www.pcug.org.au/~millerp/rmch/recu-make-cons-harm.html
#

AUTOMAKE_OPTIONS := subdir-objects
ACLOCAL_AMFLAGS = # -I m4 -I macros

NULL :=

appsdir := $(datadir)/applications

bin_PROGRAMS := 
sbin_PROGRAMS :=
noinst_PROGRAMS :=
check_PROGRAMS :=

sbin_SCRIPTS :=

dist_sbin_SCRIPTS :=
dist_noinst_SCRIPTS :=
dist_pkglibexec_SCRIPTS :=

lib_LTLIBRARIES :=

apps_DATA :=
dist_noinst_DATA := 
nobase_dist_pkgdata_DATA :=
noinst_DATA :=

noinst_HEADERS :=

BUILT_SOURCES :=
CLEANFILES :=

TESTS :=

AM_CPPFLAGS :=

EXTRA_DIST := Makefile.am.subdir

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA := 

pkglibexecdir = $(libexecdir)/$(PACKAGE)

SUBDIRS = . po

include_subdirs :=

DIST_SUBDIRS := $(SUBDIRS) $(include_subdirs)

include Makefile.inc

@INTLTOOL_DESKTOP_RULE@

rpm: dist-gzip
	rpmbuild -tb $(distdir).tar.gz

srpm: dist-gzip
	rpmbuild -ts $(distdir).tar.gz

rpms: dist-gzip
	rpmbuild -ta $(distdir).tar.gz

update-makefiles:
	@for dir in $(include_subdirs) ; do \
		echo " cp $(top_srcdir)/Makefile.am.subdir $(top_srcdir)/$$dir/Makefile.am" ; \
		cp "$(top_srcdir)/Makefile.am.subdir" "$(top_srcdir)/$$dir/Makefile.am" || exit 1 ; \
	done
