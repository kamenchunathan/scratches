project(
  'hopper',
  'cpp',
  version: '0.1',
  default_options: ['warning_level=3', 'cpp_std=c++23'],
)

cc = meson.get_compiler('cpp')
assert(cc.get_id() == 'gcc', 'This project only support the Clang compiler.')

subdir('engine')
subdir('hopper')
subdir('labs')
