/*
 * Copyright (c) <2013-2014>, Intel Corporation.
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
/*SECTION: analyzersink
 * A sink element to generate xml and hex files for each
 * video frame providing by the upstream parser element
 */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "gstanalyzersink.h"

#include <string.h>

#define GST_MPEG_VIDEO_CAPS_MAKE_WITH_FEATURES(features)                \
    "video/mpeg(" features "), "                                        \
    "mpegversion = (int) 2"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_MPEG_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MPEG_VIDEO_META) ";"
        "video/x-h264" ";" "video/x-h265"));

/* AnalyzerSink signals and args */
enum
{
  SIGNAL_NEW_FRAME,
  LAST_SIGNAL
};

#define DEFAULT_SYNC FALSE
#define DEFAULT_DUMP TRUE
#define DEFAULT_NUM_BUFFERS -1

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_SILENT,
  PROP_DUMP,
  PROP_NUM_BUFFERS
};

#define gst_analyzer_sink_parent_class parent_class
G_DEFINE_TYPE (GstAnalyzerSink, gst_analyzer_sink, GST_TYPE_BASE_SINK);

static gboolean gst_analyzer_sink_set_caps (GstBaseSink * sink, GstCaps * caps);
static void gst_analyzer_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_analyzer_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_analyzer_sink_finalize (GObject * obj);

static GstStateChangeReturn gst_analyzer_sink_change_state (GstElement *
    element, GstStateChange transition);

static GstFlowReturn gst_analyzer_sink_render (GstBaseSink * bsink,
    GstBuffer * buffer);
static gboolean gst_analyzer_sink_query (GstBaseSink * bsink, GstQuery * query);
static gboolean gst_analyzer_sink_propose_allocation (GstBaseSink * base_sink,
    GstQuery * query);

static guint gst_analyzer_sink_signals[LAST_SIGNAL] = { 0 };

static void
gst_analyzer_sink_class_init (GstAnalyzerSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbase_sink_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);
  gstbase_sink_class = GST_BASE_SINK_CLASS (klass);

  gstbase_sink_class->set_caps = gst_analyzer_sink_set_caps;
  gobject_class->set_property = gst_analyzer_sink_set_property;
  gobject_class->get_property = gst_analyzer_sink_get_property;
  gobject_class->finalize = gst_analyzer_sink_finalize;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "xml/hex files location",
          "Location of the xml/hex folder/files to write", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DUMP,
      g_param_spec_boolean ("dump", "Dump",
          "Dump frame contents as hex to the specified location", DEFAULT_DUMP,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_NUM_BUFFERS,
      g_param_spec_int ("num-frames", "num-frames",
          "Number of frames to accept before going EOS", -1, G_MAXINT,
          DEFAULT_NUM_BUFFERS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstAnalyzerSink::new-frame:
   * @analyzersink: the analyzersink instance
   * @buffer: the buffer that just has been received and analysed
   * @frame_num: the frame count
   *
   * This signal gets emitted before unreffing the buffer.
   */
  gst_analyzer_sink_signals[SIGNAL_NEW_FRAME] =
      g_signal_new ("new-frame", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstAnalyzerSinkClass, new_frame), NULL, NULL,
      g_cclosure_marshal_generic, G_TYPE_NONE, 2,
      GST_TYPE_BUFFER | G_SIGNAL_TYPE_STATIC_SCOPE, G_TYPE_INT);

  gst_element_class_set_static_metadata (gstelement_class,
      "Codec Analyzer Sink",
      "Sink",
      "Sink to dump the parsed information",
      "Sreerenj Balachandran<sreerenj.balachandran@intel.com>");
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sinktemplate));

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_analyzer_sink_change_state);

  gstbase_sink_class->render = GST_DEBUG_FUNCPTR (gst_analyzer_sink_render);
  gstbase_sink_class->query = GST_DEBUG_FUNCPTR (gst_analyzer_sink_query);
  gstbase_sink_class->propose_allocation = gst_analyzer_sink_propose_allocation;
}

static void
gst_analyzer_sink_init (GstAnalyzerSink * analyzersink)
{
  analyzersink->dump = DEFAULT_DUMP;
  analyzersink->num_buffers = DEFAULT_NUM_BUFFERS;
  analyzersink->frame_num = 0;
  analyzersink->location = NULL;
  analyzersink->codec_info = NULL;
  gst_base_sink_set_sync (GST_BASE_SINK (analyzersink), DEFAULT_SYNC);
}

static void
gst_analyzer_sink_finalize (GObject * obj)
{
  GstAnalyzerSink *sink = GST_ANALYZER_SINK (obj);

  if (sink->location)
    g_free (sink->location);

  if (sink->codec_info)
    gst_destroy_codec_info (sink->codec_info);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static gboolean
gst_analyzer_sink_set_caps (GstBaseSink * bsink, GstCaps * caps)
{
  GstStructure *structure;
  GstAnalyzerSink *sink = GST_ANALYZER_SINK (bsink);

  if (!caps)
    return FALSE;

  if (sink->codec_info)
    return TRUE;

  structure = gst_caps_get_structure (caps, 0);
  const gchar *name = gst_structure_get_name (structure);

  sink->codec_info = gst_codec_info_new_from_mime_type (name);
  if (!sink->codec_info || (sink->codec_info->type == CODEC_UNKNOWN)) {
    GST_ERROR_OBJECT (sink, "Failed to handle the input codec");
    return FALSE;
  }

  return TRUE;
}

static void
gst_analyzer_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAnalyzerSink *sink;
  const gchar *location;

  sink = GST_ANALYZER_SINK (object);

  switch (prop_id) {
    case PROP_LOCATION:
      location = g_value_get_string (value);
      if (sink->location)
        g_free (sink->location);
      sink->location = g_strdup (location);
      break;
    case PROP_DUMP:
      sink->dump = g_value_get_boolean (value);
      break;
    case PROP_NUM_BUFFERS:
      sink->num_buffers = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_analyzer_sink_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstAnalyzerSink *sink;

  sink = GST_ANALYZER_SINK (object);

  switch (prop_id) {
    case PROP_LOCATION:
      g_value_set_string (value, sink->location);
      break;
    case PROP_DUMP:
      g_value_set_boolean (value, sink->dump);
      break;
    case PROP_NUM_BUFFERS:
      g_value_set_int (value, sink->num_buffers);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_analyzer_sink_dump_mem (GstAnalyzerSink * sink, const guchar * mem,
    guint size)
{
  GString *string;
  FILE *fd;
  gchar *name;
  gchar *file_name;
  guint i = 0, j = 0;

  GST_DEBUG ("dump frame content with size = %d", size);

  /* create a new hex file for each frame */
  name =
      g_strdup_printf ("%s-%d.hex", sink->codec_info->codec_name,
      sink->frame_num);
  file_name = g_build_filename (sink->location, "hex", name, NULL);
  GST_LOG ("Created a New hex file %s to dump the content", file_name);
  free (name);

  fd = fopen (file_name, "w");
  if (fd == NULL)
    return FALSE;

  string = g_string_sized_new (50);

  while (i < size) {
    g_string_append_printf (string, "%02x   ", mem[i]);

    j++;
    i++;

    if (j == 32 || i == size) {
      fprintf (fd, "%s \n", string->str);
      g_string_set_size (string, 0);
      j = 0;
    }
  }
  g_string_free (string, TRUE);
  if (file_name)
    g_free (file_name);

  fclose (fd);
  return TRUE;
}

static GstFlowReturn
gst_analyzer_sink_render (GstBaseSink * bsink, GstBuffer * buf)
{
  GstAnalyzerSink *sink = GST_ANALYZER_SINK_CAST (bsink);
  CodecInfo *codecinfo = NULL;
  GType codecmeta_api;
  GstMeta *codecmeta = NULL;
  gboolean ret;

  codecinfo = sink->codec_info;

  if (sink->num_buffers_left == 0)
    goto eos;

  if (sink->num_buffers_left != -1)
    sink->num_buffers_left--;

  if (sink->dump) {
    GstMapInfo info;

    gst_buffer_map (buf, &info, GST_MAP_READ);
    ret = gst_analyzer_sink_dump_mem (sink, info.data, info.size);
    gst_buffer_unmap (buf, &info);
  }

  codecmeta_api = codecinfo->get_codec_meta_api_type ();
  codecmeta = codecinfo->get_codec_meta (buf, codecmeta_api);
  if (!codecmeta)
    goto no_codec_meta;

  GST_DEBUG_OBJECT (sink, "creatin video_frame_xml for frame-%d \n",
      sink->frame_num);
  if (!codecinfo->create_xml (codecmeta, sink->location, sink->frame_num,
          codecinfo->headers))
    goto error_create_xml;

  g_signal_emit (sink, gst_analyzer_sink_signals[SIGNAL_NEW_FRAME], 0, buf,
      sink->frame_num);
  sink->frame_num++;

  if (sink->num_buffers_left == 0)
    goto eos;

  return GST_FLOW_OK;

  /* ERRORS */
no_codec_meta:
  {
    GST_DEBUG_OBJECT (sink, "no codec meta");
    return GST_FLOW_EOS;
  }
error_create_xml:
  {
    GST_DEBUG_OBJECT (sink, "failed to create xml for meta");
    return GST_FLOW_EOS;
  }
eos:
  {
    GST_DEBUG_OBJECT (sink, "we are EOS");
    return GST_FLOW_EOS;
  }
}

static gboolean
gst_analyzer_sink_query (GstBaseSink * bsink, GstQuery * query)
{
  gboolean ret;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_SEEKING:{
      GstFormat fmt;

      /* we don't supporting seeking */
      gst_query_parse_seeking (query, &fmt, NULL, NULL, NULL);
      gst_query_set_seeking (query, fmt, FALSE, 0, -1);
      ret = TRUE;
      break;
    }
    default:
      ret = GST_BASE_SINK_CLASS (parent_class)->query (bsink, query);
      break;
  }

  return ret;
}

const gchar *
get_mime_type (GstAnalyzerSink * sink)
{
  GstCaps *caps = NULL;
  GstStructure *structure;
  const gchar *name;

  caps = gst_pad_get_allowed_caps (GST_BASE_SINK_PAD (sink));
  if (caps) {
    caps = gst_caps_truncate (caps);
    structure = gst_caps_get_structure (caps, 0);
    name = g_strdup (gst_structure_get_name (structure));
    gst_caps_unref (caps);
    return name;
  }
  return NULL;
}

static gboolean
gst_analyzer_sink_propose_allocation (GstBaseSink * base_sink, GstQuery * query)
{
  GstStructure *param;
  const gchar *mime_type = NULL;
  GstAnalyzerSink *analyzersink = GST_ANALYZER_SINK (base_sink);
  CodecInfo *info = analyzersink->codec_info;

  if (!info) {
    mime_type = get_mime_type (analyzersink);
    if (!mime_type) {
      GST_ERROR_OBJECT (analyzersink, "Failed to identify the mime type");
      return FALSE;
    }

    info = gst_codec_info_new_from_mime_type (mime_type);
    if (!info) {
      GST_ERROR_OBJECT (analyzersink, "Failed to create the CodecInfo");
      return FALSE;
    }

    g_free ((gchar *) mime_type);
    analyzersink->codec_info = info;
  }

  param =
      gst_structure_new (info->query_param_name, "need-slice-header",
      G_TYPE_BOOLEAN, TRUE, NULL);
  gst_query_add_allocation_meta (query, info->get_codec_meta_api_type (),
      param);
  gst_structure_free (param);

  return TRUE;
}

static GstStateChangeReturn
gst_analyzer_sink_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstAnalyzerSink *analyzersink = GST_ANALYZER_SINK (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      analyzersink->num_buffers_left = analyzersink->num_buffers;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;

  /* ERROR */
error:
  GST_ELEMENT_ERROR (element, CORE, STATE_CHANGE, (NULL),
      ("Erroring out on state change as requested"));
  return GST_STATE_CHANGE_FAILURE;
}
