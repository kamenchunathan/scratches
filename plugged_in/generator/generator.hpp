#pragma once

#include <string>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"

namespace type_erasure {

struct MethodInfo {
    std::string name;
    std::string return_type;
    std::vector<std::string> params;
    std::string to_string() const;
    std::string argument_list() const;
};

struct ConceptInfo {
    std::string name;
    clang::SourceLocation location;
    std::vector<MethodInfo> methods;
};

class CodeGenerator {
public:
    CodeGenerator(clang::ASTContext* context): context_(context) {}
    std::string generate_all(const ConceptInfo& concept_info);

private:
    clang::ASTContext* context_;
    std::string generate_interface(const ConceptInfo& concept_info);
    std::string generate_adapter(const ConceptInfo& concept_info);
    std::string generate_wrapper(const ConceptInfo& concept_info);
};

class ConceptFinder: public clang::RecursiveASTVisitor<ConceptFinder> {
public:
    explicit ConceptFinder(clang::ASTContext* context): context_(context) {}
    bool VisitConceptDecl(clang::ConceptDecl* decl);
    const std::vector<ConceptInfo>& getConcepts() const {
        return concepts_;
    }

private:
    clang::ASTContext* context_;
    std::vector<ConceptInfo> concepts_;
    bool check_for_comment_marker(clang::ConceptDecl* decl);
    std::vector<MethodInfo> extract_methods(clang::ConceptDecl* decl);
};

class TypeErasureASTConsumer: public clang::ASTConsumer {
public:
    explicit TypeErasureASTConsumer(clang::ASTContext* context, llvm::StringRef output_dir);
    void HandleTranslationUnit(clang::ASTContext& context) override;

private:
    ConceptFinder visitor_;
    std::string output_dir_;
    clang::ASTContext* context_;
};

class TypeErasureAction: public clang::ASTFrontendAction {
public:
    explicit TypeErasureAction(llvm::StringRef output_dir): output_dir_(output_dir) {}
    std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance& ci, llvm::StringRef file) override;

private:
    std::string output_dir_;
};

} // namespace type_erasure
