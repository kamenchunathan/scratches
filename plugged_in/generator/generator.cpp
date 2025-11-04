#include "generator.hpp"

#include <filesystem>
#include <format>
#include <fstream>

#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceManager.h"
#include <llvm/Support/raw_ostream.h>

namespace fs = std::filesystem;

namespace type_erasure {

// --- MethodInfo implementation ---
std::string MethodInfo::to_string() const {
  std::string params_str;
  for (size_t i = 0; i < params.size(); ++i) {
    params_str += params[i];
    if (i < params.size() - 1) {
      params_str += ", ";
    }
  }
  return std::format("{} {}({})", return_type, name, params_str);
}

std::string MethodInfo::argument_list() const {
  // This is a simplification. A real implementation would need param names.
  return "";
}

// --- ConceptFinder implementation ---
bool ConceptFinder::VisitConceptDecl(clang::ConceptDecl *decl) {
  bool has_marker = false;

  // Check for [[clang::annotate("dynamic_concept")]]
  for (auto const *attr : decl->attrs()) {
    if (auto const *annotate = llvm::dyn_cast<clang::AnnotateAttr>(attr)) {
      if (annotate->getAnnotation() == "dynamic_concept") {
        has_marker = true;
        break;
      }
    }
  }

  if (!has_marker) {
    has_marker = check_for_comment_marker(decl);
  }

  if (!has_marker) {
    return true; // Skip this concept
  }

  ConceptInfo info;
  info.name = decl->getNameAsString();
  info.location = decl->getLocation();
  info.methods = extract_methods(decl);

  // For now, we don't have method extraction, so we can't generate anything
  // useful. Let's add a placeholder method to allow generation to proceed.
  if (info.methods.empty()) {
    llvm::errs() << "Warning: Concept '" << info.name
                 << "' has no extractable methods. Method extraction is not "
                    "implemented.\n";
  }

  concepts_.push_back(std::move(info));
  return true;
}

bool ConceptFinder::check_for_comment_marker(clang::ConceptDecl *decl) {
  auto &sm = context_->getSourceManager();
  auto loc = decl->getBeginLoc();
  if (loc.isInvalid())
    return false;

  auto file_id = sm.getFileID(loc);
  bool invalid = false;
  llvm::StringRef buffer = sm.getBufferData(file_id, &invalid);
  if (invalid)
    return false;

  auto offset = sm.getFileOffset(loc);
  size_t start = (offset > 100) ? offset - 100 : 0;
  size_t len = (offset > 100) ? 100 : offset;

  llvm::StringRef preceding = buffer.substr(start, len);
  return preceding.contains("GENERATE_TYPE_ERASURE");
}

std::vector<MethodInfo>
ConceptFinder::extract_methods(clang::ConceptDecl *decl) {
  // This is a complex task requiring deep AST traversal of the
  // requires-expression. This stub implementation returns an empty vector.
  llvm::errs() << "Warning: extract_methods is a stub and does not extract any "
                  "methods.\n";
  return {};
}

// --- TypeErasureASTConsumer implementation ---
TypeErasureASTConsumer::TypeErasureASTConsumer(clang::ASTContext *context,
                                               llvm::StringRef output_dir)
    : visitor_(context), output_dir_(output_dir), context_(context) {}

void TypeErasureASTConsumer::HandleTranslationUnit(clang::ASTContext &context) {
  visitor_.TraverseDecl(context.getTranslationUnitDecl());
  CodeGenerator generator(&context);
  for (const auto &concept_info : visitor_.getConcepts()) {
    std::string generated_code = generator.generate_all(concept_info);

    fs::path out_dir(output_dir_);
    fs::create_directories(out_dir);
    fs::path out_file = out_dir / (concept_info.name + "_erasure.hpp");

    std::ofstream ofs(out_file);
    ofs << generated_code;
    llvm::outs() << "Generated type erasure for concept '" << concept_info.name
                 << "' to " << out_file << "\n";
  }
}

// --- TypeErasureAction implementation ---
std::unique_ptr<clang::ASTConsumer>
TypeErasureAction::CreateASTConsumer(clang::CompilerInstance &ci,
                                     llvm::StringRef file) {
  return std::make_unique<TypeErasureASTConsumer>(&ci.getASTContext(),
                                                  output_dir_);
}

// --- CodeGenerator implementation ---
std::string CodeGenerator::generate_interface(const ConceptInfo &concept_info) {
  std::string methods;
  for (auto const &method : concept_info.methods) {
    methods += std::format("    virtual {} = 0;\n", method.to_string());
  }

  return std::format(R"(
struct I{} {{
    virtual ~I{}() = default;

{}
}};
)",
                     concept_info.name, concept_info.name, methods);
}

std::string CodeGenerator::generate_adapter(const ConceptInfo &concept_info) {
  std::string method_overrides;
  for (auto const &method : concept_info.methods) {
    method_overrides +=
        std::format(R"(
    {} override {{
        return obj_.{}({});
    }}")",
                    method.to_string(), method.name, method.argument_list());
  }

  return std::format(R"(
template<typename T>
requires {0} <T>
class {0}Adapter final : public I{0} {{
    T obj_;

public:
    explicit {0}Adapter(T obj) : obj_(std::move(obj)) {{}}
{1}
}};
)",
                     concept_info.name, method_overrides);
}

std::string CodeGenerator::generate_wrapper(const ConceptInfo &concept_info) {
  std::string forward_methods;
  for (auto const &method : concept_info.methods) {
    forward_methods +=
        std::format(R"(
    {} {{
        return impl_->{}({});
    }}")",
                    method.to_string(), method.name, method.argument_list());
  }

  return std::format(R"(
class Any{0} {{
    std::unique_ptr<I{0}> impl_;

public:
    template<typename T>
    requires {0}<T>
    Any{0}(T obj)
        : impl_(std::make_unique<{0}Adapter<T>>(std::move(obj))) {{}}

    Any{0}(Any{0}&&) = default;
    Any{0}& operator=(Any{0}&&) = default;
{1}
}};
)",
                     concept_info.name, forward_methods);
}

std::string CodeGenerator::generate_all(const ConceptInfo &concept_info) {
  auto const &sm = context_->getSourceManager();
  auto loc_str = concept_info.location.printToString(sm);

  return std::format(
      R"(
// Auto-generated type erasure for {0}
// Generated from concept at {1}
#pragma once

#include <memory>
#include <utility>

namespace type_erasure {{
{2}
{3}
{4}
}} // namespace type_erasure
)",
      concept_info.name, loc_str, generate_interface(concept_info),
      generate_adapter(concept_info), generate_wrapper(concept_info));
}

} // namespace type_erasure
