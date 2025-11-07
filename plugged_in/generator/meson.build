project(
  'dynamic_concept_generator',
  ['c', 'cpp'],
  version: '0.1.0',
  default_options: ['cpp_std=c++23'],
)

cpp = meson.get_compiler('cpp')

# LLVM dependency is required for core data structures
llvm_dep = dependency('llvm', version: '>=17.0', required: true, components: [
    'RISCV',
    'RISCVDesc',
    'RISCVInfo',
    'RISCVAsmParser',
    'RISCVCodeGen',
  ]
)

# Clang libraries are needed for the C++ API
clang_include_path = '/usr/lib/llvm-20/include'
llvm_lib_path = '/usr/lib/llvm-20/lib'
includes = include_directories(clang_include_path)

# List of Clang static libraries to link against.
# The order can be important, from high-level to low-level.
clang_lib_names = [
  'clangAnalysis',
  'clangAnalysisFlowSensitive',
  'clangAnalysisFlowSensitiveModels',
  'clangAPINotes',
  'clangApplyReplacements',
  'clangARCMigrate',
  'clangAST',
  'clangASTMatchers',
  'clangBasic',
  'clangChangeNamespace',
  'clangCodeGen',
  'clangCrossTU',
  'clangDaemon',
  'clangDaemonTweaks',
  'clangDependencyScanning',
  'clangDirectoryWatcher',
  'clangdMain',
  'clangdMonitoringServiceProto',
  'clangDoc',
  'clangdRemoteIndex',
  'clangdRemoteIndexProto',
  'clangdRemoteIndexServiceProto',
  'clangdRemoteMarshalling',
  'clangDriver',
  'clangdSupport',
  'clangDynamicASTMatchers',
  'clangEdit',
  'clangExtractAPI',
  'clangFormat',
  'clangFrontend',
  'clangFrontendTool',
  'clangHandleCXX',
  'clangHandleLLVM',
  'clangIncludeCleaner',
  'clangIncludeFixer',
  'clangIncludeFixerPlugin',
  'clangIndex',
  'clangIndexSerialization',
  'clangInstallAPI',
  'clangInterpreter',
  'clangLex',
  'clangMove',
  'clangParse',
  'clangQuery',
  'clangReorderFields',
  'clangRewrite',
  'clangRewriteFrontend',
  'clangSema',
  'clangSerialization',
  'clangStaticAnalyzerCheckers',
  'clangStaticAnalyzerCore',
  'clangStaticAnalyzerFrontend',
  'clangSupport',
  'clangTidy',
  'clangTidyAbseilModule',
  'clangTidyAlteraModule',
  'clangTidyAndroidModule',
  'clangTidyBoostModule',
  'clangTidyBugproneModule',
  'clangTidyCERTModule',
  'clangTidyConcurrencyModule',
  'clangTidyCppCoreGuidelinesModule',
  'clangTidyDarwinModule',
  'clangTidyFuchsiaModule',
  'clangTidyGoogleModule',
  'clangTidyHICPPModule',
  'clangTidyLinuxKernelModule',
  'clangTidyLLVMLibcModule',
  'clangTidyLLVMModule',
  'clangTidyMain',
  'clangTidyMiscModule',
  'clangTidyModernizeModule',
  'clangTidyMPIModule',
  'clangTidyObjCModule',
  'clangTidyOpenMPModule',
  'clangTidyPerformanceModule',
  'clangTidyPlugin',
  'clangTidyPortabilityModule',
  'clangTidyReadabilityModule',
  'clangTidyUtils',
  'clangTidyZirconModule',
  'clangTooling',
  'clangToolingASTDiff',
  'clangToolingCore',
  'clangToolingInclusions',
  'clangToolingInclusionsStdlib',
  'clangToolingRefactoring',
  'clangToolingSyntax',
  'clangTransformer'
]
clang_libs = []
foreach lib_name : clang_lib_names
  found_lib = cpp.find_library(lib_name, dirs: llvm_lib_path)
  if found_lib.found()
    clang_libs += found_lib
  else
    error('Could not find required clang library: ' + lib_name)
  endif
endforeach

clang_dep = declare_dependency(dependencies: clang_libs, include_directories: includes)

dynamic_concept_gen = executable(
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

