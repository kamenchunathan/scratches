
#include <filesystem>
#include <vector>

#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

namespace fs = std::filesystem;

// Recursively find all header files
auto find_headers(fs::path const& dir) -> std::vector<std::string> {
    std::vector<std::string> headers;

    for (auto const& entry: fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension();
            if (ext == ".hpp" || ext == ".h" || ext == ".hxx") {
                headers.push_back(entry.path().string());
            }
        }
    }

    return headers;
}

int main(int argc, char const** argv) {
    llvm::cl::opt<std::string> include_dir(
        "include-dir",
        llvm::cl::desc("Include directory to scan recursively"),
        llvm::cl::value_desc("directory"),
        llvm::cl::Required
    );

    llvm::cl::opt<std::string> output_dir(
        "output-dir",
        llvm::cl::desc("Output directory for generated files"),
        llvm::cl::value_desc("directory"),
        llvm::cl::init("generated")
    );

    llvm::cl::opt<std::string> compile_commands(
        "compile-commands",
        llvm::cl::desc("Path to compile_commands.json"),
        llvm::cl::value_desc("file"),
        llvm::cl::init("compile_commands.json")
    );

    llvm::cl::ParseCommandLineOptions(argc, argv);

    // Find all headers
    auto headers = find_headers(include_dir.getValue());
    if (headers.empty()) {
        llvm::errs() << "No headers found in " << include_dir.getValue() << "\n";
        return 1;
    }

    llvm::outs() << "Found " << headers.size() << " headers\n";

    // Load compilation database
    std::string error;
    auto compile_db = clang::tooling::CompilationDatabase::loadFromDirectory(
        fs::path(compile_commands.getValue()).parent_path().string(),
        error
    );

    if (!compile_db) {
        llvm::errs() << "Failed to load compilation database: " << error << "\n";
        return 1;
    }

    // Process all headers
    clang::tooling::ClangTool tool(*compile_db, headers);

    auto factory = clang::tooling::newFrontendActionFactory([&]() {
        return std::make_unique<type_erasure::TypeErasureAction>(output_dir.getValue());
    });

    return tool.run(factory.get());
}
