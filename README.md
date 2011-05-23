# Notes

NOTE: This doesn't work yet as a gem, the gemspec is there for Bundler at the moment.

I'll probably end up bundling `libhandbrake.dylib`, but FYI it's ~17MB.

If you want encrypted DVD support, you'll need to either install libdvdcss (it's in homebrew) or
you can set the `DYLD_FALLBACK_LIBRARY_PATH` environment variable before starting the Ruby process.

The `DYLD_FALLBACK_LIBRARY_PATH` variable should be a path (or set) to look for the libdvdcss library.
If you have VLC installed, you can try setting it to `/Applications/VLC.app/Contents/MacOS/lib`

# Compiling

To compile first run `rake vendor:compile` - this will clone/checkout the handbrake sources into
`vendor/handbrake`, compile them then finally build a static `libhandbrake.dylib` from the object files
and place it into `ext/handbrake`.

From there, you can use the standard rake-compiler tasks to compile the gem.

# Using it a project

For now I'd suggest just cloning this into `vendor/gems` and run the compile stuff up there ^^

# Usage

``` ruby
# source can be anything HandBrake supports
# could be an mp4, an mkv or a DVD or even BlueRay disk folder/iso
source = "/path/to/source"

titles = HandBrake.scan source

# See HandBrake::Title for some basic docs
puts titles.first.width
```

# TODO

* Metadata
* Chapter
* AudioTrack
* Subtitle
* Attachment