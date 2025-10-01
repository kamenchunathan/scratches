gtest_dep = dependency('gtest', main: true, fallback: ['gtest', 'gtest_dep'])

engine_inc = include_directories('include')

engine_sources = [
  'src/engine.cpp',
  'src/ecs/world.cpp',
  'src/ecs/archetype.cpp',
  'src/renderer.cpp',
  'src/renderer/present/term.cpp',
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

renderer_test_exe = executable(
  'test_renderer',
  'test/renderer.cpp',
  dependencies: [
    engine_dep,
    gtest_dep,
  ],
  install: false,
)

test('Renderer tests', renderer_test_exe)
