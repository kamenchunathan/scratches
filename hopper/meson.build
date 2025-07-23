project(
  'hopper',
  'cpp',
  version: '0.1',
  default_options: ['warning_level=3', 'cpp_std=c++23'],
)

src = ['src/hopper.cpp']

dependencies = [
  dependency('ftxui-screen', version: '>=5.0'),
  dependency('ftxui-dom', version: '>=5.0'),
  dependency('ftxui-component', version: '>=5.0'),
  dependency('asio', version: '>=1.30'),
]

exe = executable('hopper', src, install: true, dependencies: dependencies)

test('basic', exe)
