/*
 * Copyright (c) 2013, Intel Corporation.
 * Author: Sreerenj Balachandran <sreerenj.balachandran@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#ifndef __CODEC_UTILS__
#define __CODEC_UTILS__

#include <gst/gst.h>
#include <gst/codecparsers/gstmpegvideoparser.h>
#include <gst/codecparsers/gstmpegvideometa.h>

typedef enum {
  CODEC_UNKNOWN = 0,
  CODEC_MPEG2_VIDEO = 1,
  CODEC_H264 = 2,
  CODEC_VC1 = 3,
  CODEC_MPEG4_PART_TWO = 4,
  CODEC_H265 = 5,
  CODEC_VP8 = 6,
  CODEC_VP9 = 7
} CodecType;

typedef struct {
  GstMpegVideoSequenceHdr        *sequencehdr;
  GstMpegVideoSequenceExt        *sequenceext;
  GstMpegVideoSequenceDisplayExt *sequencedispext;
  GstMpegVideoQuantMatrixExt     *quantext;
}Mpeg2Headers;

typedef union{
  Mpeg2Headers mpeg2_headers;
}CodecGeneralHeaders;

typedef GType    (*GetGstCodecMetaApiType)     (void);

typedef GstMeta* (*GetGstCodecMetaFromBuffer)  (GstBuffer *buffer,
                                                GType api_type);

typedef gboolean (*CreateVideoFrameMetaXml)    (GstMeta *meta,
                                                gchar * location,
                                                gint frame_num,
                                                CodecGeneralHeaders *headers);
typedef struct {
  const gchar *codec_name;
  const gchar *mime_type;
  const gchar *query_param_name;
  CodecType type;

  GetGstCodecMetaApiType get_codec_meta_api_type;
  GetGstCodecMetaFromBuffer get_codec_meta;
  CreateVideoFrameMetaXml create_xml;

  CodecGeneralHeaders *headers;
}CodecInfo;

CodecInfo *
gst_codec_info_new_from_mime_type (const char *mime_type);

void
gst_destroy_codec_info (CodecInfo *info);

#endif
