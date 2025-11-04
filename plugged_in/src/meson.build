project(
  'plugged',
  'cpp',
  version: '0.1',
  default_options: ['warning_level=3', 'cpp_std=c++26'],
)

dynamic_concept_gen = find_program('dynamic_concept_gen', required: true)

dynamic_concept_generator = generator(
  dynamic_concept_gen,
  output: '@BASENAME@_erasure.hpp',
  arguments: [
    '@INPUT@',
    '--output-dir=@OUTDIR@',
    '--',
    '-std=c++23',
    '-I' + meson.current_source_dir(),
  ],
)

# Custom target for recursive directory scanning
generate_dynamic_concept = custom_target(
  'generate_type_erasure',
  output: 'dynamic_concept_manifest.txt',
  input: files('include'), # Directory containing headers
  command: [
    dynamic_concept_gen,
    '--include-dir=@INPUT@',
    '--output-dir=@OUTDIR@',
    '--compile-commands='
    + join_paths(meson.build_root(), 'compile_commands.json'),
    # Redirect output list to manifest
    '>',
    '@OUTPUT@',
  ],
  build_by_default: true,
)

# Include the generated directory
inc_generated = include_directories('build/generated')

sources = ['main.cpp']

plugged = executable(
  'plugged',
  sources,

  include_directories: [inc_generated],
)

plugged.add_depends(generate_dynamic_concept)
