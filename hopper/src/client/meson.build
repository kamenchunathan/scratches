osmanip_dep = dependency('osmanip', fallback: ['osmanip', 'osmanip_dep'], required: true)

client_deps = [
  osmanip_dep,
]

executable(
  'hopper-client',
  'main.cpp',
  install: true,
  dependencies: [engine_dep] + client_deps,
)
