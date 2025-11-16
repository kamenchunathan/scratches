{ lib, stdenv, python315, cmake, ninja, bashInteractive }:

stdenv.mkDerivation rec {
  pname = "clang-p2996";
  version = "unstable-2023-10-27"; # Using a date-based version as it's from a git repo

  src = fetchGit {
    url = "https://github.com/bloomberg/clang-p2996.git";
    ref = "p2996";
    rev = "d34e5cd278803baff7e73e67507b370e7d4ad1b3";
    shallow = true;
  };

  nativeBuildInputs = [
    python315
    cmake
    ninja
    bashInteractive
  ];

  buildInputs = [ ];

  cmakeFlags = [
    "-G"
    "Ninja"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DLLVM_ENABLE_PROJECTS=clang;lld"
    "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}/opt/clang-p2996"
    "-DLLVM_ENABLE_RUNTIMES=libc;libcxx;libcxxabi;libunwind"
  ];

  configurePhase = ''
    cmake ../source/llvm $cmakeFlags
  '';

  buildPhase = ''
    echo "Build environment ready. Dropping into an interactive shell."
    echo "You are in the build directory. The source code is in ../source"
    echo "To exit, type 'exit'."
    bash
  '';

  meta = {
    description = "Clang compiler with P2996 support";
    homepage = "https://github.com/bloomberg/clang-p2996";
    license = "Apache-2.0"; # Using a string for now to bypass lib.licenses
    platforms = [ "x86_64-linux" ]; # Using a string for now to bypass lib.platforms
  };
}

