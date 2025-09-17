executable(
  'manip',
  'manip.cpp',
  install: false,
  dependencies: [
    dependency('osmanip', fallback: ['osmanip', 'osmanip_dep'], required: true),
    dependency('libpng', version: '>=1.6'),
    dependency('eigen_dep', version: '>=3.4', fallback: ['eigen', 'eigen_dep']),
  ],
)

executable(
  'iterators',
  'iterators.cpp',
  install: false,
  dependencies: [],
)

executable(
  'term',
  'term.cpp',
  install: false,
  dependencies: [],
)
