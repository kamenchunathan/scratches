project(
  'remember',
  'cpp',
  version: '0.1',
  default_options: ['warning_level=3', 'cpp_std=c++23'],
)

lib = static_library(
  'remember',
  ['./src/wrapper.cpp'],
  include_directories: './src/include'
)

executable('wrapper', './labs/wrapper.cpp', install: false, link_with: [lib])
