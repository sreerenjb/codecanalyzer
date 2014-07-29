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
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "codec_utils.h"
#include "mpeg_xml.h"

/* *INDENT-OFF* */
static CodecInfo codec_infos[] = {
  { "mpeg2", "video/mpeg", "Gst.Meta.MpegVideo",
    CODEC_MPEG2_VIDEO,
    gst_mpeg_video_meta_api_get_type,
    gst_buffer_get_meta,
    analyzer_create_mpeg2video_frame_xml,
    NULL
  },

  { "unknown", "video/unknown", NULL,
    CODEC_UNKNOWN,
    NULL,
    NULL,
    NULL,
    NULL
  }
};
/* *INDENT-ON* */

static CodecInfo *
get_analyzer_codec_info_from_mime_type (const char *mime_type)
{
  guint i = 0;
  for (i = 0; i < G_N_ELEMENTS (codec_infos); i++) {
    if (!g_strcmp0 (codec_infos[i].mime_type, mime_type))
      return &codec_infos[i];
  }
  return NULL;
}

CodecInfo *
gst_codec_info_new_from_mime_type (const char *mime_type)
{
  CodecInfo *info = NULL;

  if (mime_type == NULL)
    return;

  info = get_analyzer_codec_info_from_mime_type (mime_type);
  if (!info || info->type == CODEC_UNKNOWN) {
    GST_ERROR ("Unknown codec name");
    return FALSE;
  }

  if (!info->headers)
    info->headers = g_slice_new0 (CodecGeneralHeaders);

  return info;
}

static void
mpeg2_headers_free (Mpeg2Headers * mpeg2_hdrs)
{
  if (!mpeg2_hdrs)
    return;

  if (mpeg2_hdrs->sequencehdr)
    g_slice_free (GstMpegVideoSequenceHdr, mpeg2_hdrs->sequencehdr);
  if (mpeg2_hdrs->sequenceext)
    g_slice_free (GstMpegVideoSequenceExt, mpeg2_hdrs->sequenceext);
  if (mpeg2_hdrs->sequencedispext)
    g_slice_free (GstMpegVideoSequenceDisplayExt, mpeg2_hdrs->sequencedispext);
  if (mpeg2_hdrs->quantext)
    g_slice_free (GstMpegVideoQuantMatrixExt, mpeg2_hdrs->quantext);
}

void
gst_destroy_codec_info (CodecInfo * info)
{
  CodecGeneralHeaders *headers = NULL;
  Mpeg2Headers *mpeg2_headers = NULL;

  if (!info || !info->headers)
    return;

  headers = info->headers;
  switch (info->type) {
    case CODEC_MPEG2_VIDEO:
      mpeg2_headers_free (&headers->mpeg2_headers);
      break;
    default:
      break;
  }
}
