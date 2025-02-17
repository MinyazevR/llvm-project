//===--- PreferMemberInitializerCheck.h - clang-tidy ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CPPCOREGUIDELINES_PREFERMEMBERINITIALIZERCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CPPCOREGUIDELINES_PREFERMEMBERINITIALIZERCHECK_H

#include "../ClangTidyCheck.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Analysis/CFG.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <map>

class CFGBlock;
class ASTContext;
class CFGStmtMap;
class IfStmt;
class DiagnosticBuilder;

using namespace clang;

namespace clang::tidy::test {

class IfElseReturnChecker : public ClangTidyCheck {
public:
  IfElseReturnChecker(StringRef Name, ClangTidyContext *Context);
  bool isLanguageVersionSupported(const LangOptions &LangOpts) const override {
    return LangOpts.CPlusPlus;
  }
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  bool runInternal(IfStmt *ifStmt);

private:
    std::vector<const IfStmt *> iFStmts;
    CFGBlock* mBlock  = nullptr;
    ASTContext* mContext = nullptr;
    clang::LangOptions mLangOptions;
    SourceManager *mManager;
    std::unique_ptr<CFG> mCfg;
    CFGStmtMap *mStmtToBlockMap1 = nullptr; // ?
    std::map<int64_t, SourceRange> mList;
    IfStmt* mNewStmt;
    Rewriter Rewrite;
    const FunctionDecl *mFunctionDecl = nullptr;
    std::set<const CFGBlock *> mIterruptionsBlock;
    std::map<int64_t, IfStmt*> mIdToStmtMap;

    bool result = true;

    bool reversCondition(IfStmt* ifStmt); 
    int getNodeWeight(const CFGBlock* block);
    const Stmt* getBlockStmt(const CFGBlock* B);
    bool setIterruptionsBlockListForStmt(const Stmt *stmt,
                                        const CFGBlock *exitBlock,
                                     bool needPush = true);

    bool createIterruptFrontierMapForStmt(const Stmt* stmt);

    bool isOnlyIterruptionBlock(const CFGBlock *block);

    void privateAddIterruptionBlockToStmt(CFGBlock *block, Stmt* b, Stmt* endStmt);
    const Stmt* getIterruptStatement(const CFGBlock *block);
    const Stmt* getBlockStatement(const CFGBlock *block);
    llvm::StringRef get_source_text(clang::SourceRange range, const clang::SourceManager& sm);
    llvm::StringRef get_source_text_raw(clang::SourceRange range, const clang::SourceManager& sm);
    void appendStmt(const CompoundStmt* stmt, Stmt* stmtToAdd);
    void reverseStmt(IfStmt* ifStmt);
    bool addIterruptionBlockToStmts(const Stmt* stmt);
    bool isOnlyReturnBlock(const CFGBlock *block);
    bool isBlockInCurrentIfStmt(const CFGBlock *block, const Stmt* stmt);
    unsigned getLineNumber(const Stmt *stmt, SourceManager &SM);
};
};

 // namespace clang::tidy::cppcoreguidelines

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CPPCOREGUIDELINES_PREFERMEMBERINITIALIZERCHECK_H
