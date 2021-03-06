#include <ruby.h>
#include <hb.h>

#if defined(__GNUC__) && (__GNUC__ >= 3)
#define RB_HB_UNUSED __attribute__ ((unused))
#else
#define RB_HB_UNUSED
#endif

static VALUE rb_cHandBrake_cTitle, rb_cHandBrake_cTitle_cMetadata,
             rb_cHandBrake_cTitle_cChapter, rb_cHandBrake_cTitle_cAudioTrack,
             rb_cHandBrake_cTitle_cSubtitle, rb_cHandBrake_cTitle_cAttachment,
             rb_cHandBrake_cTitle_cLanguage;

static hb_handle_t *handle;
static bool scanBusy = FALSE;

/* Hack */
struct hb_metadata_t {
   char  name[255];
   char  artist[255];
   char  composer[255];
   char  release_date[255];
   char  comment[1024];
   char  album[255];
   char  genre[255];
   uint32_t coverart_size;
   uint8_t *coverart;
};

static void rb_cHandBrake__logger(RB_HB_UNUSED const char* message) {
  // noop
}

static VALUE rb_cHandBrake__new_attachment(hb_attachment_t *attachment) {
  VALUE rb_attachment;

  rb_attachment = rb_class_new_instance(0, NULL, rb_cHandBrake_cTitle_cAttachment);

  switch(attachment->type) {
    case FONT_TTF_ATTACH:
      rb_iv_set(rb_attachment, "@type", ID2SYM(rb_intern("ttf_font")));
    default:
      rb_iv_set(rb_attachment, "@type", ID2SYM(rb_intern("unknown")));
      break;
  }

  if (attachment->name && strlen(attachment->name)) {
    rb_iv_set(rb_attachment, "@name", rb_str_new2(attachment->name));
  }

  if (attachment->size) {
    rb_iv_set(rb_attachment, "@data", rb_str_new(attachment->data, attachment->size));
  }

  return rb_attachment;
}

static VALUE rb_cHandBrake__new_subtitle(hb_subtitle_t *subtitle) {
  VALUE rb_subtitle;
  VALUE rb_language;

  rb_subtitle = rb_class_new_instance(0, NULL, rb_cHandBrake_cTitle_cSubtitle);

  rb_iv_set(rb_subtitle, "@track", INT2FIX(subtitle->track));

  rb_iv_set(rb_subtitle, "@format", ID2SYM(rb_intern(subtitle->format == TEXTSUB ? "text" : "bitmap")));

  rb_iv_set(rb_subtitle, "@source", ID2SYM(rb_intern(hb_subsource_name(subtitle->source))));

  rb_language = rb_class_new_instance(0, NULL, rb_cHandBrake_cTitle_cLanguage);
  if (subtitle->lang && strlen(subtitle->lang)) {
    rb_iv_set(rb_language, "@description", rb_str_new2(subtitle->lang));
  }
  if (subtitle->iso639_2 && strlen(subtitle->iso639_2)) {
    rb_iv_set(rb_language, "@iso639_2", rb_str_new2(subtitle->iso639_2));
  }
  rb_iv_set(rb_subtitle, "@language", rb_language);

  rb_iv_set(rb_subtitle, "@type", INT2FIX(subtitle->type));

  if (subtitle->config.src_filename && strlen(subtitle->config.src_filename)) {
    rb_iv_set(rb_subtitle, "@filename", rb_str_new2(subtitle->config.src_filename));
  }

  if (subtitle->config.src_codeset && strlen(subtitle->config.src_codeset)) {
    rb_iv_set(rb_subtitle, "@codeset", rb_str_new2(subtitle->config.src_codeset));
  }

  rb_iv_set(rb_subtitle, "@offset", ULL2NUM(subtitle->config.offset));

  return rb_subtitle;
}

static VALUE rb_cHandBrake__new_audio_track(hb_audio_config_t *audio_track) {
  VALUE rb_audio_track;
  VALUE rb_language;

  rb_audio_track = rb_class_new_instance(0, NULL, rb_cHandBrake_cTitle_cAudioTrack);

  rb_iv_set(rb_audio_track, "@track", INT2FIX(audio_track->in.track));

  // TODO: Find out what these all are.
  // I bet some of these aren't actually input codecs but rather used for the encode
  // process or something else
  switch(audio_track->in.codec) {
    case HB_ACODEC_FAAC:
      rb_iv_set(rb_audio_track, "@codec", rb_str_new2("AAC"));
      break;
    case HB_ACODEC_LAME:
    rb_iv_set(rb_audio_track, "@codec", rb_str_new2("MP3"));
      break;
    case HB_ACODEC_VORBIS:
      rb_iv_set(rb_audio_track, "@codec", rb_str_new2("Vorbis"));
      break;
    case HB_ACODEC_AC3:
      rb_iv_set(rb_audio_track, "@codec", rb_str_new2("AC3"));
      break;
    // case HB_ACODEC_MPGA:
    //   // rb_iv_set(rb_audio_track, "@codec", rb_str_new2("MPGA"));
    //   switch(audio_track->in.stream_type) {
    //     case 0x80:
    //       // LPCM audio in bluray have an stype of 0x80
    //       rb_iv_set(rb_audio_track, "@codec", rb_str_new2("BD LPCM"));
    //       break;
    //     case 0x83:
    //       // This is an interleaved TrueHD/AC-3 stream and the esid of
    //       // the AC-3 is 0x76
    //       if (audio_track->in.substream_type == HB_SUBSTREAM_BD_AC3) {
    //         rb_iv_set(rb_audio_track, "@codec", rb_str_new2("AC3"));
    //       } else {
    //         rb_iv_set(rb_audio_track, "@codec", rb_str_new2("TrueHD"));
    //       }
    //       break;
    //     case 0x85:
    //       // DTS-HD HRA audio in bluray has an stype of 0x85
    //       // which conflicts with ATSC Program ID
    //       // To distinguish, Bluray streams have a reg_desc of HDMV
    //       // This is an interleaved DTS-HD HRA/DTS stream and the
    //       // esid of the DTS is 0x71
    //       if (audio_track->in.substream_type == HB_SUBSTREAM_BD_DTS) {
    //         rb_iv_set(rb_audio_track, "@codec", rb_str_new2("DTS"));
    //       } else {
    //         rb_iv_set(rb_audio_track, "@codec", rb_str_new2("DTS-HD HRA"));
    //       }
    //       break;
    //     case 0x84:
    //       // EAC3 audio in bluray has an stype of 0x84
    //       // which conflicts with SDDS
    //       // To distinguish, Bluray streams have a reg_desc of HDMV
    //       rb_iv_set(rb_audio_track, "@codec", rb_str_new2("E-AC3"));
    //       break;
    //     case 0x86:
    //       // This is an interleaved DTS-HD MA/DTS stream and the
    //       // esid of the DTS is 0x71
    //       if (audio_track->in.substream_type == HB_SUBSTREAM_BD_DTS) {
    //         rb_iv_set(rb_audio_track, "@codec", rb_str_new2("DTS"));
    //       } else {
    //         rb_iv_set(rb_audio_track, "@codec", rb_str_new2("DTS-HD MA"));
    //       }
    //       break;
    //     default:
    //       rb_iv_set(rb_audio_track, "@codec", ID2SYM(rb_intern("unknown")));
    //       break;
    //   }
    //   break;
    case HB_ACODEC_LPCM:
      rb_iv_set(rb_audio_track, "@codec", rb_str_new2("LPCM"));
      break;
    case HB_ACODEC_DCA:
      rb_iv_set(rb_audio_track, "@codec", rb_str_new2("DTS"));
      break;
    case HB_ACODEC_FFMPEG:
      rb_iv_set(rb_audio_track, "@codec", rb_str_new2("FFMPEG"));
      break;
    case HB_ACODEC_CA_AAC:
      rb_iv_set(rb_audio_track, "@codec", rb_str_new2("AAC"));
      break;
    case HB_ACODEC_CA_HAAC:
      rb_iv_set(rb_audio_track, "@codec", rb_str_new2("HAAC"));
      break;
    default:
      rb_iv_set(rb_audio_track, "@codec", rb_str_new2("unknown"));
      break;
  }

  // rb_iv_set(rb_audio_track, "@registration_descriptor", INT2FIX(audio_track->in.reg_desc));
  // rb_iv_set(rb_audio_track, "@stream_type", INT2FIX(audio_track->in.stream_type));
  // rb_iv_set(rb_audio_track, "@substream_type", INT2FIX(audio_track->in.substream_type));
  // rb_iv_set(rb_audio_track, "@codec_params", INT2FIX(audio_track->in.codec_param));

  rb_iv_set(rb_audio_track, "@version", INT2FIX(audio_track->in.version));

  rb_iv_set(rb_audio_track, "@mode", INT2FIX(audio_track->in.mode));

  rb_iv_set(rb_audio_track, "@samplerate", INT2FIX(audio_track->in.samplerate));

  rb_iv_set(rb_audio_track, "@bitrate", INT2FIX(audio_track->in.bitrate));

  switch (audio_track->in.channel_layout) {
    case HB_INPUT_CH_LAYOUT_MONO:
      rb_iv_set(rb_audio_track, "@channel_layout", ID2SYM(rb_intern("mono")));
      break;
    case HB_INPUT_CH_LAYOUT_STEREO:
      rb_iv_set(rb_audio_track, "@channel_layout", ID2SYM(rb_intern("stereo")));
      break;
    case HB_INPUT_CH_LAYOUT_DOLBY:
      rb_iv_set(rb_audio_track, "@channel_layout", ID2SYM(rb_intern("dolby")));
      break;
    case HB_INPUT_CH_LAYOUT_3F:
      rb_iv_set(rb_audio_track, "@channel_layout", ID2SYM(rb_intern("3F")));
      break;
    case HB_INPUT_CH_LAYOUT_2F1R:
      rb_iv_set(rb_audio_track, "@channel_layout", ID2SYM(rb_intern("2F1R")));
      break;
    case HB_INPUT_CH_LAYOUT_3F1R:
      rb_iv_set(rb_audio_track, "@channel_layout", ID2SYM(rb_intern("3F1R")));
      break;
    case HB_INPUT_CH_LAYOUT_2F2R:
      rb_iv_set(rb_audio_track, "@channel_layout", ID2SYM(rb_intern("2F2R")));
      break;
    case HB_INPUT_CH_LAYOUT_3F2R:
      rb_iv_set(rb_audio_track, "@channel_layout", ID2SYM(rb_intern("3F2R")));
      break;
    case HB_INPUT_CH_LAYOUT_4F2R:
      rb_iv_set(rb_audio_track, "@channel_layout", ID2SYM(rb_intern("4F2R")));
      break;
    case HB_INPUT_CH_LAYOUT_3F4R:
      rb_iv_set(rb_audio_track, "@channel_layout", ID2SYM(rb_intern("3F4R")));
      break;
    default:
      rb_iv_set(rb_audio_track, "@channel_layout", ID2SYM(rb_intern("unknown")));
      break;
  }

  rb_language = rb_class_new_instance(0, NULL, rb_cHandBrake_cTitle_cLanguage);
  if (audio_track->lang.description && strlen(audio_track->lang.description)) {
    rb_iv_set(rb_language, "@description", rb_str_new2(audio_track->lang.description));
  }
  if (audio_track->lang.simple && strlen(audio_track->lang.simple)) {
    rb_iv_set(rb_language, "@simple", rb_str_new2(audio_track->lang.simple));
  }
  if (audio_track->lang.iso639_2 && strlen(audio_track->lang.iso639_2)) {
    rb_iv_set(rb_language, "@iso639_2", rb_str_new2(audio_track->lang.iso639_2));
  }
  rb_iv_set(rb_language, "@type", INT2FIX(audio_track->lang.type));
  rb_iv_set(rb_audio_track, "@language", rb_language);

  return rb_audio_track;
}

static VALUE rb_cHandBrake__new_chapter(hb_chapter_t *chapter) {
  VALUE rb_chapter;

  rb_chapter = rb_class_new_instance(0, NULL, rb_cHandBrake_cTitle_cChapter);

  rb_iv_set(rb_chapter, "@index", INT2FIX(chapter->index));

  rb_iv_set(rb_chapter, "@pgcn", INT2FIX(chapter->pgcn));

  rb_iv_set(rb_chapter, "@pgn", INT2FIX(chapter->pgn));

  rb_iv_set(rb_chapter, "@cell_start", INT2FIX(chapter->cell_start));
  rb_iv_set(rb_chapter, "@cell_end", INT2FIX(chapter->cell_end));

  rb_iv_set(rb_chapter, "@block_start", ULL2NUM(chapter->block_start));
  rb_iv_set(rb_chapter, "@block_end", ULL2NUM(chapter->block_end));
  rb_iv_set(rb_chapter, "@block_count", ULL2NUM(chapter->block_count));

  rb_iv_set(rb_chapter, "@hours", INT2FIX(chapter->hours));
  rb_iv_set(rb_chapter, "@minutes", INT2FIX(chapter->minutes));
  rb_iv_set(rb_chapter, "@seconds", INT2FIX(chapter->seconds));
  rb_iv_set(rb_chapter, "@duration", ULL2NUM(chapter->duration));

  if (chapter->title && strlen(chapter->title)) {
    rb_iv_set(rb_chapter, "@title", rb_str_new2(chapter->title));
  }

  return rb_chapter;
}

static VALUE rb_cHandBrake__new_metadata(hb_metadata_t *metadata) {
  VALUE rb_metadata;

  rb_metadata = rb_class_new_instance(0, NULL, rb_cHandBrake_cTitle_cMetadata);

  if (metadata->name && strlen(metadata->name)) {
    rb_iv_set(rb_metadata, "@name", rb_str_new2(metadata->name));
  }

  if (metadata->artist && strlen(metadata->artist)) {
    rb_iv_set(rb_metadata, "@artist", rb_str_new2(metadata->artist));
  }

  if (metadata->composer && strlen(metadata->composer)) {
    rb_iv_set(rb_metadata, "@composer", rb_str_new2(metadata->composer));
  }

  if (metadata->release_date && strlen(metadata->release_date)) {
    rb_iv_set(rb_metadata, "@release_date", rb_str_new2(metadata->release_date));
  }

  if (metadata->comment && strlen(metadata->comment)) {
    rb_iv_set(rb_metadata, "@comment", rb_str_new2(metadata->comment));
  }

  if (metadata->album && strlen(metadata->album)) {
    rb_iv_set(rb_metadata, "@album", rb_str_new2(metadata->album));
  }

  if (metadata->genre && strlen(metadata->genre)) {
    rb_iv_set(rb_metadata, "@genre", rb_str_new2(metadata->genre));
  }

  if (metadata->coverart_size) {
    rb_iv_set(rb_metadata, "@coverart", rb_str_new((const char*)metadata->coverart, metadata->coverart_size));
  }

  return rb_metadata;
}

static VALUE rb_cHandBrake__new_title(hb_title_t *title) {
  uint8_t i = 0;
  uint8_t num_chapters = 0;
  uint8_t num_audio_tracks = 0;
  uint8_t num_subtitles = 0;
  uint8_t num_attachments = 0;
  VALUE rb_title;
  VALUE rb_metadata;

  rb_title = rb_class_new_instance(0, NULL, rb_cHandBrake_cTitle);

  if (title->path && strlen(title->path)) {
    rb_iv_set(rb_title, "@path", rb_str_new2(title->path));
  }

  if (title->name && strlen(title->name)) {
    rb_iv_set(rb_title, "@name", rb_str_new2(title->name));
  }

  if (title->type == HB_STREAM_TYPE || title->type == HB_FF_STREAM_TYPE) {
    rb_iv_set(rb_title, "@type", ID2SYM(rb_intern("stream")));
  } else if (title->type == HB_DVD_TYPE) {
    rb_iv_set(rb_title, "@type", ID2SYM(rb_intern("dvd")));

    rb_iv_set(rb_title, "@vts", INT2FIX(title->vts));
    rb_iv_set(rb_title, "@ttn", INT2FIX(title->ttn));

    // TODO: should this a Range?
    rb_iv_set(rb_title, "@cell_start", ULL2NUM(title->cell_start));
    rb_iv_set(rb_title, "@cell_end", ULL2NUM(title->cell_end));

    // TODO: should this a Range?
    rb_iv_set(rb_title, "@block_start", ULL2NUM(title->block_start));
    rb_iv_set(rb_title, "@block_end", ULL2NUM(title->block_end));

    rb_iv_set(rb_title, "@pcr_pid", INT2FIX(title->pcr_pid));
  }

  rb_iv_set(rb_title, "@angle_count", INT2FIX(title->angle_count));

  rb_iv_set(rb_title, "@hours", INT2FIX(title->hours));
  rb_iv_set(rb_title, "@minutes", INT2FIX(title->minutes));
  rb_iv_set(rb_title, "@seconds", INT2FIX(title->seconds));

  rb_iv_set(rb_title, "@duration", ULL2NUM(title->duration));

  rb_iv_set(rb_title, "@aspect", rb_float_new(title->aspect));
  rb_iv_set(rb_title, "@container_aspect", rb_float_new(title->container_aspect));

  rb_iv_set(rb_title, "@width", INT2FIX(title->width));
  rb_iv_set(rb_title, "@height", INT2FIX(title->height));

  rb_iv_set(rb_title, "@pixel_aspect_width", INT2FIX(title->pixel_aspect_width));
  rb_iv_set(rb_title, "@pixel_aspect_height", INT2FIX(title->pixel_aspect_height));

  rb_iv_set(rb_title, "@fps", rb_float_new((double)(title->rate / title->rate_base)));

  rb_iv_set(rb_title, "@detected_interlacing", title->detected_interlacing ? Qtrue : Qfalse);

  if (title->video_codec_name && strlen(title->video_codec_name)) {
    rb_iv_set(rb_title, "@video_codec_name", rb_str_new2(title->video_codec_name));
  }

  rb_iv_set(rb_title, "@video_bitrate", INT2FIX(title->video_bitrate));

  if (title->container_name && strlen(title->container_name)) {
    rb_iv_set(rb_title, "@container_name", rb_str_new2(title->container_name));
  }

  rb_iv_set(rb_title, "@data_rate", INT2FIX(title->data_rate));

  if (title->metadata) {
    rb_metadata = rb_cHandBrake__new_metadata(title->metadata);
    rb_iv_set(rb_title, "@metadata", rb_metadata);
  }

  num_chapters = hb_list_count(title->list_chapter);
  if (num_chapters) {
    hb_chapter_t *chapter;
    VALUE rb_chapters = rb_ary_new2(num_chapters);

    rb_iv_set(rb_title, "@chapters", rb_chapters);

    for(i=0; i < num_chapters; i++) {
      chapter = hb_list_item(title->list_chapter, i);
      rb_ary_push(rb_chapters, rb_cHandBrake__new_chapter(chapter));
    }
  }

  num_audio_tracks = hb_list_count(title->list_audio);
  if (num_audio_tracks) {
    hb_audio_config_t *audio_track;
    VALUE rb_audio_tracks = rb_ary_new2(num_chapters);

    rb_iv_set(rb_title, "@audio_tracks", rb_audio_tracks);

    for(i=0; i < num_audio_tracks; i++) {
      audio_track = hb_list_audio_config_item(title->list_audio, i);
      rb_ary_push(rb_audio_tracks, rb_cHandBrake__new_audio_track(audio_track));
    }
  }

  num_subtitles = hb_list_count(title->list_subtitle);
  if (num_subtitles) {
    hb_subtitle_t *subtitle;
    VALUE rb_subtitles = rb_ary_new2(num_subtitles);

    rb_iv_set(rb_title, "@subtitles", rb_subtitles);

    for(i=0; i < num_subtitles; i++) {
      subtitle = hb_list_item(title->list_subtitle, i);
      rb_ary_push(rb_subtitles, rb_cHandBrake__new_subtitle(subtitle));
    }
  }

  num_attachments = hb_list_count(title->list_attachment);
  if (num_attachments) {
    hb_attachment_t *attachment;
    VALUE rb_attachments = rb_ary_new2(num_attachments);

    rb_iv_set(rb_title, "@attachments", rb_attachments);

    for(i=0; i < num_attachments; i++) {
      attachment = hb_list_item(title->list_attachment, i);
      rb_ary_push(rb_attachments, rb_cHandBrake__new_attachment(attachment));
    }
  }

  return rb_title;
}

static VALUE rb_cHandBrake_scan(int argc, VALUE *argv, RB_HB_UNUSED VALUE klass) {
  hb_list_t  *titles;
  hb_state_t hb_state;
  uint64_t min_title_duration = 900000LL;
  uint8_t num_titles = 0;
  uint8_t i = 0;
  const char *path;
  VALUE rb_path;

  if(scanBusy) {
    rb_raise(rb_eRuntimeError, "You can only run one scan at a time per process, please wait until the previous scan finishes.");
  }

  // TODO: support the following parameters in some way:
  //
  // title_index    - Desired title to scan. 0 for all titles.
  // preview_count  - Number of preview images to generate.
  // store_previews - Whether or not to write previews to disk.
  rb_scan_args(argc, argv, "10", &rb_path);
  Check_Type(rb_path, T_STRING);
  path = RSTRING_PTR(rb_path);

  if(!handle) {
    handle = hb_init(0, 0);
  }

  hb_scan(handle, path, 0, 10, 0, min_title_duration);

  scanBusy = TRUE;
  // Now lets wait for the scan to complete
  hb_get_state(handle, &hb_state);
  while (hb_state.state != HB_STATE_SCANDONE) {
    hb_snooze(10);
    hb_get_state(handle, &hb_state);
  }
  scanBusy = FALSE;

  titles = hb_get_titles(handle);
  num_titles = hb_list_count(titles);
  if(num_titles) {
    hb_title_t *title;
    VALUE rb_titles = rb_ary_new2(num_titles);
    VALUE rb_title;
    for (i=0; i < num_titles; i++) {
      title = hb_list_item(titles, i);
      rb_title = rb_cHandBrake__new_title(title);
      rb_ary_push(rb_titles, rb_title);
    }

    return rb_titles;
  } else {
    return rb_ary_new();
  }
}

static VALUE rb_cHandBrake_scanning(RB_HB_UNUSED VALUE self) {
  return scanBusy ? Qtrue : Qfalse;
}

static VALUE rb_cHandBrake_cleanup(RB_HB_UNUSED VALUE self) {
  hb_close(&handle);
  hb_global_close();

  handle = NULL;

  return Qnil;
}

void Init_handbrake() {
  VALUE rb_cHandBrake = rb_define_class("HandBrake", rb_cObject);
  rb_cHandBrake_cTitle = rb_const_get(rb_cHandBrake, rb_intern("Title"));
  rb_cHandBrake_cTitle_cMetadata = rb_const_get(rb_cHandBrake_cTitle, rb_intern("Metadata"));
  rb_cHandBrake_cTitle_cChapter = rb_const_get(rb_cHandBrake_cTitle, rb_intern("Chapter"));
  rb_cHandBrake_cTitle_cAudioTrack = rb_const_get(rb_cHandBrake_cTitle, rb_intern("AudioTrack"));
  rb_cHandBrake_cTitle_cSubtitle = rb_const_get(rb_cHandBrake_cTitle, rb_intern("Subtitle"));
  rb_cHandBrake_cTitle_cAttachment = rb_const_get(rb_cHandBrake_cTitle, rb_intern("Attachment"));
  rb_cHandBrake_cTitle_cLanguage = rb_const_get(rb_cHandBrake_cTitle, rb_intern("Language"));

  rb_define_singleton_method(rb_cHandBrake, "scan", rb_cHandBrake_scan, -1);
  rb_define_singleton_method(rb_cHandBrake, "scanning?", rb_cHandBrake_scanning, 0);
  rb_define_singleton_method(rb_cHandBrake, "cleanup!", rb_cHandBrake_cleanup, 0);

  // globally initialize libhb
  handle = hb_init(0, 0);
  hb_dvd_set_dvdnav(1);

  // silence logging messages
  hb_register_logger(rb_cHandBrake__logger);
}