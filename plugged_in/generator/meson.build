project(
  'dynamic_concept_generator',
  ['c', 'cpp'],
  version: '0.1.0',
  default_options: ['cpp_std=c++23'],
)

clang_dep = dependency('libclang', version: '>=17.0', required: false)

if not clang_dep.found()
  cpp = meson.get_compiler('cpp')
  message(
    'Could not find libclang dependency via pkg-config, trying hardcoded paths.',
  )
  clang_include_path = '/usr/lib/llvm-20/include'
  llvm_lib_path = '/usr/lib/llvm-20/lib'
  includes = include_directories(clang_include_path)
  libclang_lib = cpp.find_library('clang', dirs: llvm_lib_path)
  clang_dep = declare_dependency(dependencies: libclang_lib, include_directories: includes)
endif

llvm_dep = dependency('llvm', version: '>=17.0', required: true)

type_erasure_gen = executable(
  'dynamic_concept_gen',
  'main.cpp',
  'generator.cpp',
  dependencies: [llvm_dep, clang_dep],
  install: true,
  cpp_args: [
    '-fno-rtti', # LLVM is built without RTTI
    '-Wno-deprecated-declarations',
  ],
)

# Make the executable available for use in other projects
meson.override_find_program('dynamic_concept_gen', type_erasure_gen)
