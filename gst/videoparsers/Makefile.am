noinst_LTLIBRARIES =                               \
	libgstcodecanalyzer-videoparsers.la        \
	$(NULL)

libgstcodecanalyzer_videoparsers_la_SOURCES =      \
	gstmpegvideoparse.c                       \
	plugin.c                                   \
	videoparsers_utils.c                       \
	$(NULL)

noinst_HEADERS =                                   \
	gstmpegvideoparse.h                       \
	videoparsers_utils.h                       \
        $(NULL)

libgstcodecanalyzer_videoparsers_la_CFLAGS =       \
	-DGST_USE_UNSTABLE_API                     \
	-I$(top_srcdir)/gst-libs	     	   \
	-I$(top_builddir)/gst-libs		   \
	$(GST_BASE_CFLAGS)                         \
	$(GST_VIDEO_CFLAGS)                        \
	$(GST_CODEC_PARSERS_CFLAGS)                \
	$(GST_CFLAGS)                              \
	$(NULL)

libgstcodecanalyzer_videoparsers_la_LIBADD =       \
	$(GST_BASE_LIBS)                           \
	$(GST_LIBS)                                \
	$(GST_VIDEO_LIBS)                          \
	$(top_builddir)/gst-libs/gst/codecparsers/libgstcodecanalyzer-codecparsers.la \
	$(NULL)

libgstcodecanalyzer_videoparsers_la_LDFLAGS =      \
	$(GST_ALL_LDFLAGS)                         \
	$(NULL)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in
