include $(top_srcdir)/example/Makefile.inc

#VPATH = $(srcdir) $(builddir)
#VPATH += $(top_srcdir)/example/p4/includes
#VPATH += $(top_srcdir)/example/p4/ctrl_plane
#VPATH += $(top_srcdir)/example/p4/data_plane
#VPATH += $(top_srcdir)/example/p4/src_hardware_indep

#AM_CFLAGS += -I "$(top_srcdir)/example/p4/includes"
#AM_CFLAGS += -I "$(top_srcdir)/example/p4/ctrl_plane"
#AM_CFLAGS += -I "$(top_srcdir)/example/p4/data_plane"
#AM_CFLAGS += -I "$(top_srcdir)/example/p4/src_hardware_indep"

bindir=${exec_prefix}/odp/bin
#AM_CFLAGS += -O0
#AM_CFLAGS += -Wall 
#AM_CFLAGS += -Wno-error=all
#AM_CFLAGS += -g
#AM_CFLAGS += -std=gnu99


bin_PROGRAMS = odp_p4

odp_p4_LDFLAGS = $(AM_LDFLAGS) -static
odp_p4_CFLAGS = $(AM_CFLAGS) -I${top_srcdir}/example -Wno-error

#noinst_HEADERS = \
		  $(top_srcdir)/example/p4/includes/compiled/actions.h \
		  $(top_srcdir)/example/p4/includes/compiled/data_plane_data.h \
		  $(top_srcdir)/example/p4/includes/compiled/parser.h \
		  $(top_srcdir)/example/p4/includes/freescale/freescale_lib.h \
		  $(top_srcdir)/example/p4/includes/freescale/aliases.h \
		  $(top_srcdir)/example/p4/includes/freescale/backend.h \
		  $(top_srcdir)/example/p4/includes/freescale/freescale_primitives.h \
		  $(top_srcdir)/example/p4/includes/freescale/freescale_tables.h \
		  $(top_srcdir)/example/p4/includes/shared/dataplane.h \
		  $(top_srcdir)/example/p4/includes/shared/ternary_naive.h \
		  $(top_srcdir)/example/example_debug.h

dist_odp_p4_SOURCES = main.c \
                      includes/compiled/actions.c \
                      includes/compiled/dataplane.c \
                      includes/compiled/parser.c \
                      includes/compiled/tables.c \
                      includes/freescale/backend.c \
                      includes/freescale/freescale_tables.c \
                      includes/shared/ternary_naive.c

