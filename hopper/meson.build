project(
  'hopper',
  'cpp',
  version: '0.2.0',
  default_options: ['warning_level=3', 'cpp_std=c++23'],
)

version_array = meson.project_version().split('.')
add_project_arguments([
  '-DHOPPER_VERSION_MAJOR=' + version_array[0],
  '-DHOPPER_VERSION_MINOR=' + version_array[1],
  '-DHOPPER_VERSION_PATCH=' + version_array[2],
], language: 'cpp')

subdir('engine')
subdir('hopper')
subdir('labs')
