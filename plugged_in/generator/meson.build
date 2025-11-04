project(
  'dynamic_concept_generator',
  ['c', 'cpp'],
  version: '0.1.0',
  default_options: ['cpp_std=c++23'],
)

clang_dep = dependency('libclang', version: '>=17.0', required: false)

if not clang_dep.found()
  cpp = meson.get_compiler('cpp')
  clang_include_path = meson.get_env('CLANG_INCLUDE_PATH')
  llvm_lib_path = meson.get_env('LLVM_LIB_PATH')

  if clang_include_path != '' and llvm_lib_path != ''
    message('Found clang via environment variables.')
    includes = include_directories(clang_include_path)
    libclang_lib = cpp.find_library('clang', dirs: llvm_lib_path)
    clang_dep = declare_dependency(
      dependencies: libclang_lib,
      include_directories: includes
    )
  else
    error('Could not find libclang dependency')
  endif
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
  ],
)

# Make the executable available for use in other projects
meson.override_find_program('dynamic_concept_gen', type_erasure_gen)
