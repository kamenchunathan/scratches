project(
  'plugged',
  'cpp',
  version: '0.1',
  default_options: ['warning_level=3', 'cpp_std=c++26'],
)

dynamic_concept_gen = find_program(
  'dynamic_concept_gen',
  required: true,
  dirs: [join_paths(meson.build_root(), 'generator')],
)

generate_dynamic_concept = custom_target(
  'generate_type_erasure',
  output: 'Printable_erasure.hpp',
  input: files('include/sortable.hpp'),
  command: [
    dynamic_concept_gen,
    '--include-dir=' + join_paths(meson.current_source_dir(), 'include'),
    '--output-dir=@OUTDIR@',
    '--compile-commands='
    + join_paths(meson.build_root(), 'compile_commands.json'),
  ],
  build_by_default: true,
)

sources = ['src/main.cpp', generate_dynamic_concept]

plugged = executable(
  'plugged',
  sources,
  include_directories: ['include'],
)
