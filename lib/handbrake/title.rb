class HandBrake
  class Title
    # Title metadata (like ID3 tags for audio)
    class Metadata
      attr_reader :name
      attr_reader :artist
      attr_reader :composer
      attr_reader :release_date
      attr_reader :comment
      attr_reader :album
      attr_reader :genre
      attr_reader :coverart_size
      attr_reader :coverart
    end

    class Chapter
      
    end

    class AudioTrack
      attr_reader :codec
      attr_reader :regular_description
      attr_reader :stream_type
      attr_reader :substream_type
      attr_reader :codec_param
      attr_reader :version
      attr_reader :mode
      attr_reader :sample_rate
      attr_reader :bitrate
      attr_reader :channel_layout

      # will be either :ac3 or :dca
      attr_reader :flags
      attr_reader :language
    end

    class Subtitle
      
    end

    # These are usually used for attaching embedded fonts to movies containing SSA subtitles.
    class Attachment
      
    end

    class Language
      attr_reader :description
      attr_reader :simple
      attr_reader :iso639_2
      attr_reader :type
    end

    # Symbol - :stream or :dvd
    attr_reader :type

    # Fixnum - no idea
    attr_reader :reg_desc

    # String - like "/path/to/some_file.mp4" or "/Volumes/SomeDVD"
    attr_reader :path

    # String - like "some_file"
    attr_reader :name

    # Fixnum - the title's index
    attr_reader :index

    # Fixnum - no idea
    attr_reader :vts

    # Fixnum - no idea
    attr_reader :ttn

    # Fixnum - cell offset of the start of this track
    attr_reader :cell_start

    # Fixnum - cell offset of the end of this track
    attr_reader :cell_end

    # Fixnum - block offset of the start of this track
    attr_reader :block_start

    # Fixnum - block offset of the end of this track
    attr_reader :block_end

    # Fixnum - the size of this track in blocks
    attr_reader :block_count

    # Fixnum - number of angles
    attr_reader :angle_count

    # Fixnum - these are for display purposes like 1:35:16
    attr_reader :hours
    attr_reader :minutes
    attr_reader :seconds

    # Fixnum - exact duration (in 1/90000s)
    attr_reader :duration

    # Float - aspect ratio for the title's video
    attr_reader :aspect

    # Float - aspect ratio from container (0 if none)
    attr_reader :container_aspect

    # Fixnum - width of this title's video in pixels
    attr_reader :width

    # Fixnum - height of this title's video in pixels
    attr_reader :height

    # Fixnum - usually shown like "pixel aspect: #{pixel_aspect_width}/#{pixel_aspect_height}"
    attr_reader :pixel_aspect_width
    attr_reader :pixel_aspect_height

    # Float - the framerate for this title's video
    attr_reader :fps

    # Array - of 4 Fixnums, the detected auto-crop dimensions
    # attr_reader :crop

    # (true/false) - combing was detected, it may be interlaced or telecined
    attr_reader :detected_interlacing

    # Fixnum - PCR PID for TS streams
    attr_reader :pcr_pid

    # Fixnum - demuxer stream id for video
    # attr_reader :video_id

    # Fixnum - codec id for this tite's video
    # attr_reader :video_codec

    # Fixnum - no idea
    # attr_reader :video_stream_type

    # Fixnum - no idea
    # attr_reader :video_codec_param

    # String - like "mp4"
    attr_reader :video_codec_name

    # Fixnum - bitrate for this track's video
    attr_reader :video_bitrate

    # String - like "mov,mp4,m4a,3gp,3g2,mj2"
    attr_reader :container_name

    # Fixnum - no idea
    attr_reader :data_rate

    # 
    attr_reader :metadata

    # Array of HandBrake::Title::Chapter objects
    # the collection of chapters for this title
    attr_reader :chapters

    # Array of HandBrake::Title::AudioTrack objects
    # the collection of audio tracks for this title
    attr_reader :audio_tracks

    # Array of HandBrake::Title::Subtitle objects
    # the collection of embedded subtitles for this title
    attr_reader :subtitles

    # Array of HandBrake::Title::Attachment objects
    # the collection of embedded attachments for this title
    attr_reader :attachments

    # Fixnum - no idea
    # attr_reader :flags
  end
end