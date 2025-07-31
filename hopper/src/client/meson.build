client_deps = [
  dependency('ftxui-screen', version: '>=5.0'),
  dependency('ftxui-dom', version: '>=5.0'),
  dependency('ftxui-component', version: '>=5.0'),
]

executable(
  'hopper-client',
  'main.cpp',
  install: true,
  dependencies: [engine_dep] + client_deps,
)
