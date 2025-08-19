engine_inc = include_directories('include')

engine_sources = [
  'src/engine.cpp',
  'src/ecs/world.cpp',
  'src/ecs/archetype.cpp',
]

engine_lib = static_library(
  'engine',
  engine_sources,
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

executable(
  'ecs-test',
  'test/ecs.cpp',
  dependencies: [engine_dep],
  install: false,
)
