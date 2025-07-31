engine_inc = include_directories('include')

engine_lib = static_library(
  'engine',
  'src/engine.cpp',
  include_directories: engine_inc,
  install: true,
  dependencies: [
    dependency('asio', version: '>=1.30'),
  ],
)

engine_dep = declare_dependency(
  include_directories: engine_inc,
  link_with: engine_lib,
)
