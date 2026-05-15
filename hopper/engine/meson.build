engine_inc = include_directories('include')

engine_sources = [
  'src/application.cpp',
  'src/input.cpp',
  'src/renderer.cpp',
  'src/transform.cpp',
  'src/ecs/archetype.cpp',
  'src/ecs/world.cpp',
  'src/log/layer.cpp',
  'src/physics/components.cpp',
  'src/physics/layer.cpp',
  'src/physics/systems.cpp',
  'src/physics/shape.cpp',
  'src/renderer/graph.cpp',
  'src/term/layer.cpp',
  'src/term/presenter.cpp',
  'src/term/input_parser.cpp',
  'src/util/text.cpp',
  'src/web/layer.cpp',
  'src/web/presenter.cpp',
]

engine_lib_deps = [
  dependency('asio', version: '>=1.30'),
  dependency('eigen3', version: '>=-3.4'),
]

engine_dep_deps = [
  dependency('eigen3', version: '>=3.4'),
]

if get_option('hopper_logging')
  spdlog_dep = subproject(
    'spdlog',
    default_options: ['default_library=static'],
  ).get_variable('spdlog_dep')

  engine_lib_deps += spdlog_dep
  engine_dep_deps += spdlog_dep
  add_project_arguments('-DHOPPER_LOGGING', language: 'cpp')
endif

if get_option('hopper_metrics')
  add_project_arguments('-DHOPPER_METRICS', language: 'cpp')
endif

if get_option('hopper_tracy')
  tracy_dep = dependency(
    'tracy',
    fallback: ['tracy', 'tracy_dep'],
    default_options: ['tracy_enable=true', 'tracy_on_demand=true'],
  )
  add_project_arguments('-DHOPPER_TRACY', language: 'cpp')
else
  tracy_dep = dependency(
    'tracy',
    fallback: ['tracy', 'tracy_dep'],
    default_options: ['tracy_enable=false'],
  )
endif

emscripten_link_args = []

if host_machine.system() == 'emscripten'
  emscripten_link_args += ['--js-library=' + meson.current_source_dir() / 'web_presenter.js']
else
  engine_lib_deps += dependency('libpng', static: true)
  engine_dep_deps += dependency('libpng', static: true)
endif

engine_lib = static_library(
  'engine',
  engine_sources,
  include_directories: engine_inc,
  install: true,
  dependencies: engine_lib_deps,
)

engine_dep = declare_dependency(
  include_directories: engine_inc,
  link_with: engine_lib,
  dependencies: engine_dep_deps,
  link_args: emscripten_link_args,
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

application_test_exe = executable(
  'application_test',
  'test/application.cpp',
  dependencies: [
    engine_dep,
    dependency('gtest', main: false, fallback: ['gtest', 'gtest_dep']),
  ],
  install: false,
)

test('Application tests', application_test_exe)
