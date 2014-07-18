noinst_LTLIBRARIES =                            \
	libgstcodecanalyzer-codecparsers.la     \
	$(NULL)

libgstcodecanalyzer_codecparsers_la_SOURCES =   \
	gstmpegvideoparser.c                    \
	gstmpegvideometa.c                      \
	parserutils.c                           \
	$(NULL)

noinst_HEADERS =                                \
	gstmpegvideoparser.h                    \
	gstmpegvideometa.h                      \
	parserutils.h                           \
	$(NULL)

libgstcodecanalyzer_codecparsers_la_CFLAGS =    \
	-DGST_USE_UNSTABLE_API                  \
	$(GST_BASE_CFLAGS)                      \
	$(GST_CFLAGS)                           \
	$(NULL)

libgstcodecanalyzer_codecparsers_la_LIBADD =    \
	$(GST_BASE_LIBS)                        \
	$(GST_LIBS)                             \
	$(NULL)

libgstcodecanalyzer_codecparsers_la_LDFLAGS =   \
	$(GST_ALL_LDFLAGS)                      \
	$(NULL)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in
