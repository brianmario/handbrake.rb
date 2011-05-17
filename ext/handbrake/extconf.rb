require 'mkmf'

$CFLAGS << ' -Wall -funroll-loops'
$CFLAGS << " -I#{File.expand_path('../../../vendor/handbrake/build/libhb', __FILE__)}"
$CFLAGS << ' -Wextra -O0 -ggdb3' # if ENV['DEBUG']

$LDFLAGS << " #{File.expand_path('../libhandbrake.dylib', __FILE__)}"

create_makefile('handbrake')
