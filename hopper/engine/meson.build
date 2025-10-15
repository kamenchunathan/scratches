engine_inc = include_directories('include')

engine_sources = [
  'src/application.cpp',
  'src/input.cpp',
  'src/renderer.cpp',
  'src/ecs/archetype.cpp',
  'src/ecs/world.cpp',
  'src/renderer/graph.cpp',
  'src/renderer/sprite.cpp',
  'src/term/layer.cpp',
  'src/term/presenter.cpp',
  'src/term/input_parser.cpp',
  'src/util/text.cpp',
]

engine_lib = static_library(
  'engine',
  engine_sources,
  include_directories: engine_inc,
  install: true,
  dependencies: [
    dependency('asio', version: '>=1.30'),
    dependency('eigen3', version: '>=-3.4'),
    dependency('libpng', static: true),
  ],
)

engine_dep = declare_dependency(
  include_directories: engine_inc,
  link_with: engine_lib,
  dependencies: [
    dependency('eigen3', version: '>=3.4'),
    dependency('libpng', static: true),
  ],
)

diff_test_exe = executable(
  'diff_test',
  'test/diff.cpp',
  dependencies: [
    engine_dep,
    dependency('gtest', main: true, fallback: ['gtest', 'gtest_dep']),
  ],
  install: false,
)

test('Diff tests', diff_test_exe)
