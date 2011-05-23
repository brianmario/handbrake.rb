require 'handbrake/title'
require 'handbrake/handbrake'

at_exit do
  HandBrake.cleanup!
end