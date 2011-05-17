#include <ruby.h>
#include <hb.h>

static VALUE rb_cHandBrake_cTitle, rb_cHandBrake_cTitle_cMetadata,
             rb_cHandBrake_cTitle_cChapter, rb_cHandBrake_cTitle_cAudioTrack,
             rb_cHandBrake_cTitle_cSubtitle, rb_cHandBrake_cTitle_cAttachment,
             rb_cHandBrake_cTitle_cLanguage;

static void rb_cHandBrake__logger(const char* message) {
  // noop
}

static void rb_cHandBrake__free(void * ptr) {
  hb_handle_t *handle = (hb_handle_t *)ptr;

  hb_close(&handle);
}

static VALUE rb_cHandBrake__allocate(VALUE klass) {
  hb_handle_t *handle;

  handle = hb_init(0, 0);

  // silence logging messages
  hb_register_logger(rb_cHandBrake__logger);

  return Data_Wrap_Struct(klass, NULL, rb_cHandBrake__free, handle);
}

static VALUE rb_cHandBrake__new_title(hb_title_t *title) {
  uint8_t num_chapters;
  uint8_t num_audio_tracks;
  uint8_t num_subtitles;
  uint8_t num_attachments;
  VALUE rb_title;

  rb_title = rb_class_new_instance(0, NULL, rb_cHandBrake_cTitle);

  rb_iv_set(rb_title, "@path", rb_str_new2(title->path));
  rb_iv_set(rb_title, "@name", rb_str_new2(title->name));

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

  rb_iv_set(rb_title, "@video_codec_name", rb_str_new2(title->video_codec_name));

  rb_iv_set(rb_title, "@video_bitrate", INT2FIX(title->video_bitrate));

  rb_iv_set(rb_title, "@container_name", rb_str_new2(title->container_name));

  rb_iv_set(rb_title, "@data_rate", INT2FIX(title->data_rate));

  return rb_title;
}

static VALUE rb_cHandBrake_scan(int argc, VALUE *argv, VALUE self) {
  hb_handle_t *handle;
  hb_list_t  *titles;
  hb_state_t hb_state;
  uint64_t min_title_duration = 900000LL;
  uint8_t num_titles = 0;
  uint8_t i = 0;
  const char *path;
  VALUE rb_path;

  Data_Get_Struct(self, hb_handle_t, handle);

  // TODO: support the following parameters in some way:
  //
  // title_index    - Desired title to scan. 0 for all titles.
  // preview_count  - Number of preview images to generate.
  // store_previews - Whether or not to write previews to disk.
  rb_scan_args(argc, argv, "10", &rb_path);
  Check_Type(rb_path, T_STRING);
  path = RSTRING_PTR(rb_path);

  hb_scan(handle, path, 0, 10, 0, min_title_duration);

  // Now lets wait for the scan to complete
  while (hb_state.state != HB_STATE_SCANDONE) {
    hb_snooze(200);
    hb_get_state(handle, &hb_state);
  }

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
    return Qnil;
  }
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

  rb_define_alloc_func(rb_cHandBrake, rb_cHandBrake__allocate);

  rb_define_method(rb_cHandBrake, "scan", rb_cHandBrake_scan, -1);
}