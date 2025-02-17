#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"  // Include for CompilerInstance
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <memory>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

// Define the matcher to find if-else statements followed by a return statement
StatementMatcher IfElseReturnMatcher = ifStmt(
    hasElse(stmt()),
    hasParent(compoundStmt(
        statementCountIs(2),
        has(returnStmt())
    ))
).bind("ifElseReturn");

class IfElseReturnRewriter : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override {
        Rewrite.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        Context = &CI.getASTContext();

        Finder = std::make_unique<MatchFinder>();
        Finder->addMatcher(IfElseReturnMatcher, this);

        return Finder->newASTConsumer();
    }

    void run(const MatchFinder::MatchResult &Result) {
        if (const IfStmt *IfStmt = Result.Nodes.getNodeAs<IfStmt>("ifElseReturn")) {
            const auto *Parent = Result.Context->getParents(*IfStmt)[0].get<Stmt>();
            if (const auto *CS = dyn_cast<CompoundStmt>(Parent)) {
                for (const auto *Child : CS->children()) {
                    if (isa<ReturnStmt>(Child)) {
                        Rewrite.InsertTextBefore(Child->getBeginLoc(), "// This return is after an if-else\n");
                        break;
                    }
                }
            }
        }
    }

    void EndSourceFileAction() override {
        const RewriteBuffer *RewriteBuf =
            Rewrite.getRewriteBufferFor(Rewrite.getSourceMgr().getMainFileID());
        if (RewriteBuf) {
            llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());
        }
    }

private:
    Rewriter Rewrite;
    ASTContext *Context;
    std::unique_ptr<MatchFinder> Finder;
};

int main(int argc, const char **argv) {
    llvm::cl::OptionCategory ToolCategory("if-else-detector");
    auto OptionsParser = CommonOptionsParser::create(argc, argv, ToolCategory);
    if (!OptionsParser) {
        llvm::errs() << OptionsParser.takeError();
        return 1;
    }

    ClangTool Tool(OptionsParser->getCompilations(), OptionsParser->getSourcePathList());

    int Result = Tool.run(newFrontendActionFactory<IfElseReturnRewriter>().get());

    return Result;
}
