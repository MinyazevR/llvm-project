#include "clang/AST/AST.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"  // Include for CompilerInstance
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <memory>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

using namespace clang;
using namespace clang::ast_matchers;

/*StatementMatcher IfElseReturnMatcher =
    ifStmt(hasElse(stmt()),
           hasParent(compoundStmt(
               statementCountIs(2),
               has(returnStmt())
           ))).bind("ifElseReturn");*/


StatementMatcher IfElseReturnMatcher =
    compoundStmt(
        has(ifStmt(hasElse(stmt())).bind("ifElseReturn"))
    );
            
    
class IfElseReturnChecker : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const IfStmt *IfStmt = Result.Nodes.getNodeAs<clang::IfStmt>("ifElseReturn");
        if (IfStmt) {
        
            const auto *Parent = Result.Context->getParents(*IfStmt)[0].get<Stmt>();
            Parent->dump();
            if (const auto *CS = dyn_cast<CompoundStmt>(Parent)) {
                for (const auto *Child : CS->children()) {
                    if (isa<ReturnStmt>(Child)) {
                        llvm::outs() << "AAAAAAAAA";
                        clang::ASTContext  &context = *Result.Context;
                        Rewrite.setSourceMgr(context.getSourceManager(), context.getLangOpts());
                        Rewrite.InsertTextBefore(Child->getBeginLoc(), "// This return is after an if-else\n");
	
			const RewriteBuffer *RewriteBuf =
			    Rewrite.getRewriteBufferFor(Rewrite.getSourceMgr().getMainFileID());
			if (RewriteBuf) {
			    const FileEntry *Entry = Rewrite.getSourceMgr().getFileEntryForID(Rewrite.getSourceMgr().getMainFileID());
			    if (Entry) {
				std::string filename = Entry->getName().str();
				std::ofstream output(filename);
				output << std::string(RewriteBuf->begin(), RewriteBuf->end()); 
				output.close();
			    }
			}
        
                        break;
                    }
                }
            }
            
            const SourceManager &SM = *Result.SourceManager;
            auto Loc = IfStmt->getIfLoc();

            llvm::outs() << "Found if-else with immediate return at "
                         << SM.getFilename(Loc) << ":"
                         << SM.getSpellingLineNumber(Loc) << "\n";

            IfStmt->dump();
        }
    }
private:
    Rewriter Rewrite;
};


int main(int argc, const char **argv) {
    llvm::cl::OptionCategory ToolCategory("if-else-detector");
    auto OptionsParser = tooling::CommonOptionsParser::create(argc, argv, ToolCategory);
    if (!OptionsParser) {
        llvm::errs() << OptionsParser.takeError();
        return 1;
    }

    clang::tooling::ClangTool Tool(OptionsParser->getCompilations(), OptionsParser->getSourcePathList());

    IfElseReturnChecker Printer;
    MatchFinder Finder;
    Finder.addMatcher(IfElseReturnMatcher, &Printer);

    return Tool.run(tooling::newFrontendActionFactory(&Finder).get());
}

