# Notes

NOTE: This doesn't work yet as a gem, the gemspec is there for Bundler at the moment.

I'll probably end up bundling `libhandbrake.dylib`, but FYI it's ~17MB.

# Compiling

To compile first run `rake vendor:compile` - this will clone/checkout the handbrake sources into
`vendor/handbrake`, compile them then finally build a static `libhandbrake.dylib` from the object files
and place it into `ext/handbrake`.

From there, you can use the standard rake-compiler tasks to compile the gem.

# Using it a project

For now I'd suggest just cloning this into `vendor/gems` and run the compile stuff up there ^^