# rspec
begin
  require 'rspec'
  require 'rspec/core/rake_task'

  desc "Run all examples with RCov"
  RSpec::Core::RakeTask.new('spec:rcov') do |t|
    t.rcov = true
  end
  RSpec::Core::RakeTask.new('spec') do |t|
    t.verbose = true
  end

  task :default => :spec
rescue LoadError
  puts "rspec, or one of its dependencies, is not available. Install it with: sudo gem install rspec"
end

# rake-compiler
require 'rake' unless defined? Rake

gem 'rake-compiler', '>= 0.7.5'
require "rake/extensiontask"

Rake::ExtensionTask.new('handbrake') do |ext|
  ext.cross_compile = true
  ext.cross_platform = ['x86-mingw32', 'x86-mswin32-60']

  ext.lib_dir = File.join 'lib', 'handbrake'
end

Rake::Task[:spec].prerequisites << :compile

namespace :vendor do
  has_svn  = !`which svn`.empty?
  has_git  = !`which git`.empty?
  git_repo = 'git@github.com:HandBrake/HandBrake.git'
  svn_repo = 'svn://svn.handbrake.fr/HandBrake/trunk'
  local_repo_path = File.expand_path('../vendor/handbrake',__FILE__)

  task :source do
    # TODO: update if checkout already exists
    if File.exist? local_repo_path
      puts "fetching the latest HandBrake source..."
      if File.exist? "#{local_repo_path}/.git"
        `cd #{local_repo_path}; git pull`
      elsif File.exist? "#{local_repo_path}/.svn"
        `cd #{local_repo_path}; svn up`
      else
        # not a git or svn repo, do nothing
      end
    else
      if has_git
        puts "using git to clone #{git_repo} to #{local_repo_path}"
        `git clone #{git_repo} #{local_repo_path}`
      elsif has_svn
        puts "using svn to chekcout #{git_repo} into #{local_repo_path}"
        `svn co #{svn_repo} #{local_repo_path}`
      else
        puts "You need git or svn installed to checkout HandBrake"
      end
    end
  end

  task :compile do
    build_cmd = proc do
      puts "Building HandBrake..."
      build_cmd = "cd #{local_repo_path}; ./configure --launch --launch-jobs=0"
      build_cmd << " --debug=max" if ENV['DEBUG']
      `#{build_cmd}`
    end
    if File.exist? local_repo_path
      build_cmd.call
    else
      Rake::Task['vendor:source'].invoke
      build_cmd.call
    end

    output_path = File.expand_path('../ext/handbrake',__FILE__)

    link_cmd = []
    link_cmd << "clang -arch x86_64 -v -static -shared"
    link_cmd << "-L#{local_repo_path}/build"
    link_cmd << "-L#{local_repo_path}/build/libhb"
    link_cmd << "-L#{local_repo_path}/build/contrib/lib"
    link_cmd << "-F#{local_repo_path}/build"
    link_cmd << "-F#{local_repo_path}/macosx"
    link_cmd << "-filelist #{local_repo_path}/build/xroot/HandBrakeCLI.build/Objects-normal/x86_64/HandBrakeCLI.LinkFileList"
    link_cmd << "#{Dir["#{local_repo_path}/build/contrib/lib/*.a"].join(" ")}"
    link_cmd << "/usr/lib/libpthread.dylib"
    link_cmd << "/usr/lib/libc.dylib"
    link_cmd << "/usr/lib/libstdc++.dylib"
    link_cmd << "-framework CoreServices"
    link_cmd << "-framework AudioToolbox"
    link_cmd << "-framework IOKit"
    link_cmd << "-lhb"
    link_cmd << "-liconv"
    link_cmd << "-lbz2"
    link_cmd << "-lz"
    link_cmd << "-o #{output_path}/libhandbrake.dylib"

    puts "Linking libhandbrake.dylib and placing it in #{output_path}"
    `#{link_cmd.join(" ")}`

  end
end