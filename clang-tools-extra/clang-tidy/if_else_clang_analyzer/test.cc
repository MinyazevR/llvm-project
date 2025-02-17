#include "clang/AST/AST.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Analysis/CFG.h"
#include "clang/Analysis/Analyses/Dominators.h"
#include "clang/Analysis/Analyses/CFGReachabilityAnalysis.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Analysis/CFGStmtMap.h"
#include "clang/AST/ParentMap.h"
#include "clang/AST/Stmt.h"
#include "clang/Analysis/CFG.h"
#include "clang/AST/ASTContext.h"
#include "../utils/ASTUtils.h"
#include "../utils/ExprSequence.h"
#include  "clang/Analysis/CFGStmtMap.h"
#include "clang/Basic/SourceManager.h"
#include "test.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTDumper.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;
using namespace clang::tidy::utils;
using namespace clang::tidy::test;

class IfStmtVisitor final: public RecursiveASTVisitor<IfStmtVisitor> {

    IfElseReturnChecker *mChecker;

    public:
        explicit IfStmtVisitor(IfElseReturnChecker *checker)
            : mChecker(checker) {}

        bool VisitIfStmt(clang::IfStmt *ifStmt) {
            if (ifStmt->hasElseStorage()) {
                return mChecker->runInternal(ifStmt);
            }
            return RecursiveASTVisitor::VisitIfStmt(ifStmt);
        }

        bool TraverseStmt(Stmt *stmt) {
            if (!stmt) return true;

            if (auto *ifStmt = dyn_cast<clang::IfStmt>(stmt)) {
                if (Stmt *thenStmt = ifStmt->getThen()) TraverseStmt(thenStmt);
                if (Stmt *elseStmt = ifStmt->getElse()) TraverseStmt(elseStmt);
                return VisitIfStmt(ifStmt);
            }

            return RecursiveASTVisitor::TraverseStmt(stmt);
        }
};

IfElseReturnChecker::IfElseReturnChecker(
        StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context) {}


void IfElseReturnChecker::registerMatchers(MatchFinder *Finder) {
    Finder->addMatcher(
      functionDecl(isDefinition(),
                   unless(anyOf(isDefaulted(), isDeleted(), isWeak())))
          .bind("functionDecl"), this);
}

bool IfElseReturnChecker::runInternal(IfStmt* ifStmt) {
    mIterruptionsBlock.clear();

    ifStmt->dumpPretty(*mContext);

    auto *thenStmt = ifStmt->getThen();
    auto *elseStmt = ifStmt->getElse();
    auto *thenBranch = mStmtToBlockMap1->getBlock(thenStmt);
    auto *elseBranch = mStmtToBlockMap1->getBlock(elseStmt);

    // bool isThenFirst = getNodeWeight(thenBranch) < getNodeWeight(elseBranch);
    bool isThenFirst = false;

    auto* targetStmt = isThenFirst ? thenStmt : elseStmt;

    if(!createIterruptFrontierMapForStmt(targetStmt)) { 
        return true;
    }

    if (!addIterruptionBlockToStmts(targetStmt)) {
        return false;
    }
    if (!isThenFirst) {
        if (!reversCondition(ifStmt)) {
            return false;
        }
        reverseStmt(ifStmt);
    }
    return true;
}

void IfElseReturnChecker::check(const MatchFinder::MatchResult &Result) {
    mContext = Result.Context;
    mLangOptions = mContext->getLangOpts();
    mManager = Result.SourceManager;
    Rewrite.setSourceMgr(*mManager, mLangOptions);

    mFunctionDecl = Result.Nodes.getNodeAs<clang::FunctionDecl>("functionDecl");  
    mCfg = CFG::buildCFG(mFunctionDecl, mFunctionDecl->getBody(),
                    mContext, CFG::BuildOptions());

    auto parentMap = new ParentMap(mFunctionDecl->getBody());
    mStmtToBlockMap1 = CFGStmtMap::Build(mCfg.get(), parentMap);

    IfStmtVisitor Visitor(this);
    if (Visitor.TraverseDecl(const_cast<clang::FunctionDecl *>(mFunctionDecl))) {
        Rewrite.overwriteChangedFiles();
    }
}  

bool IfElseReturnChecker::reversCondition(IfStmt *ifStmt) {

    auto *condition = ifStmt->getCond();
    auto *conditionStmt = condition->getExprStmt();
    auto conditionType = conditionStmt->getType();
    
    if (!conditionType->isBooleanType()) {
        return false;
    }

    auto booleanCondition = dyn_cast<BinaryOperator>(condition);
    auto opcode = booleanCondition->getOpcode();

    if (booleanCondition->isComparisonOp(opcode)) {
        if (auto negateOpCode = booleanCondition->negateComparisonOp(opcode)) {
            booleanCondition->getBeginLoc();
            booleanCondition->getExprLoc();
            booleanCondition->getEndLoc();
            auto opcodeStr = BinaryOperator::getOpcodeStr(negateOpCode);
            Rewrite.ReplaceText(booleanCondition->getOperatorLoc(), opcodeStr);
            return true;
        }
        // FPOptionsOverride t(mLangOptions);

        // ParenExpr pexpr(
        //     condition->getSourceRange().getBegin(),
        //     condition->getSourceRange().getEnd(),
        //     condition
        // );

        // UnaryOperator *invertedCond = UnaryOperator::Create(
        //     *mContext,
        //     &pexpr,
        //     UO_LNot,
        //     (&pexpr)->getType(),
        //     VK_PRValue,
        //     OK_Ordinary,
        //     (&pexpr)->getSourceRange().getBegin(),
        //     false,
        //     t
        // );

        // nonConstIfElseStmt->setCond(invertedCond); 
    }
    return false;
}

int IfElseReturnChecker::getNodeWeight(const clang::CFGBlock* block) {
    return block->size() + block->succ_size();
}

const Stmt* IfElseReturnChecker::getBlockStmt(const clang::CFGBlock* B) {
    for (const auto &Elem : *B) {
        if (auto S = Elem.getAs<CFGStmt>())
            return S->getStmt();
    }
    return nullptr;
}

bool IfElseReturnChecker::setIterruptionsBlockListForStmt(const Stmt *stmt,
                                const clang::CFGBlock *exitBlock,
                                bool needPush) {
    ExprSequence exprSequence(mCfg.get(), mFunctionDecl->getBody(), mContext);
    for (const clang::CFGBlock* block: exitBlock->preds()) {
        auto *blockStmt = getBlockStmt(block);
        if (isOnlyIterruptionBlock(block)) {
            bool condition = needPush && (isBlockInCurrentIfStmt(block, stmt) 
                || exprSequence.inSequence(stmt, blockStmt));
            if (condition) {
                mIterruptionsBlock.insert(block);
            }
        } else if (!isBlockInCurrentIfStmt(block, stmt) && 
            exprSequence.inSequence(stmt, blockStmt)) {
            mIterruptionsBlock.clear();
            return false;
        }
    }
    return true;
}

bool IfElseReturnChecker::createIterruptFrontierMapForStmt(const Stmt* stmt) {
    if(!setIterruptionsBlockListForStmt(stmt, &mCfg->getExit())) {
        return false;
    }

    for (auto &&iterruptionsBlock: mIterruptionsBlock) {
        if (!setIterruptionsBlockListForStmt(stmt, iterruptionsBlock, false)) {
            return false;
        }
    }

    return true;
}

bool IfElseReturnChecker::isOnlyIterruptionBlock(const CFGBlock *block) {
    return block->getBlockID() == mCfg->getExit().getBlockID() 
            || isOnlyReturnBlock(block) 
            || block->hasNoReturnElement();

}

const Stmt* IfElseReturnChecker::getIterruptStatement(const clang::CFGBlock *block) {
    auto iterruptInstruction = block->back();
    if (auto returnBlockCFGStmt = iterruptInstruction.getAs<CFGStmt>()) {
        auto *answer = dyn_cast<ReturnStmt>(returnBlockCFGStmt->getStmt());
        return answer;
    }
    return nullptr;
}

const Stmt* IfElseReturnChecker::getBlockStatement(const clang::CFGBlock *block) {
    const Stmt *stmt = nullptr;
    for (auto &element: block->Elements) {
        auto CFGStmtElement = element.getAs<clang::CFGStmt>();
        if (CFGStmtElement) {
            stmt = CFGStmtElement->getStmt();
        }
    }
    return stmt;
}

llvm::StringRef IfElseReturnChecker::get_source_text(clang::SourceRange range, const clang::SourceManager& sm) {
    clang::LangOptions lo;
    auto start_loc = sm.getSpellingLoc(range.getBegin());
    auto last_token_loc = sm.getSpellingLoc(range.getEnd());
    auto end_loc = clang::Lexer::getLocForEndOfToken(last_token_loc, 0, sm, lo);
    auto printable_range = clang::SourceRange{start_loc, end_loc};
    return get_source_text_raw(printable_range, sm);
}

llvm::StringRef IfElseReturnChecker::get_source_text_raw(clang::SourceRange range, const clang::SourceManager& sm) {
    return clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range), sm, mLangOptions);
}

void IfElseReturnChecker::appendStmt(const CompoundStmt* stmt, Stmt* stmtToAdd) {
    auto* lastStmt = stmt->body_back();

    auto ref = get_source_text(stmtToAdd->getSourceRange(), *mManager);


    auto range = lastStmt->getSourceRange();
    auto last_token_loc = (*mManager).getSpellingLoc(range.getEnd());
    auto indent = clang::Lexer::getIndentationForLine(lastStmt->getBeginLoc(), *mManager);
    auto end_loc = clang::Lexer::getLocForEndOfToken(last_token_loc, 0, *mManager, mLangOptions);
    std::string t = "\n" + indent.str() + ref.str();

    Rewrite.InsertTextAfterToken(end_loc, t);
}

void IfElseReturnChecker::reverseStmt(IfStmt* ifStmt) {
    
    auto thenRange = ifStmt->getThen()->getSourceRange();
    auto elseRange = ifStmt->getElse()->getSourceRange();

    auto ifText = Rewrite.getRewrittenText(thenRange);
    auto elseText = Rewrite.getRewrittenText(elseRange);

    Rewrite.ReplaceText(thenRange, elseText);
    Rewrite.ReplaceText(elseRange, ifText);
}

bool IfElseReturnChecker::addIterruptionBlockToStmts(const Stmt* stmt) {

    for (auto &&iterruptionBlock: mIterruptionsBlock) {
        auto cmdCompound = dyn_cast<CompoundStmt>(stmt);
        auto *lastStmt = cmdCompound->body_back();
        if (isa<ReturnStmt>(lastStmt)) {
            return false;
        }
        if (isBlockInCurrentIfStmt(iterruptionBlock, stmt)) {
            continue;
        }
        // for (auto &&frontierBlock: iterruptionBlock->preds()) {
            auto *iterruptionBlockStmt = getIterruptStatement(iterruptionBlock);
            llvm::errs() << "frontierBlock \n";
            // frontierBlock->dump();
            llvm::errs() << "getIterruptStatement \n";
            iterruptionBlockStmt->dump();
            // if (frontierBlock->succ_size() == 1) {
                llvm::errs() << "succ_size == 1 \n";
                // frontierBlock->dump();
                stmt->dumpPretty(*mContext);
                // auto frontierBlockStmt = getBlockStatement(frontierBlock);
                appendStmt(dyn_cast<CompoundStmt>(stmt),
                    const_cast<Stmt *>(iterruptionBlockStmt));
                return true;
            // }
            /*
            
            direct clang::tidy::utils::findEndLocation()

            */
        // }
    }
    return false;

}

bool IfElseReturnChecker::isOnlyReturnBlock(const clang::CFGBlock *block) {
    auto returnBlock = block->back();

    if (auto returnBlockCFGStmt = returnBlock.getAs<CFGStmt>()) {
        if (auto returnBlockStmt = dyn_cast<ReturnStmt>(returnBlockCFGStmt->getStmt())) {
            return isBlockInCurrentIfStmt(block, returnBlockStmt);
        }
        return false;
    }
    return false;
}

bool IfElseReturnChecker::isBlockInCurrentIfStmt(const clang::CFGBlock *block, const Stmt* stmt) {
    for(auto &&blockElement: block->Elements) {
        auto cfgStmtElement = blockElement.getAs<CFGStmt>();
        if (!cfgStmtElement) {
            return false;
        }

        auto *cfgStmtElementStmt = cfgStmtElement->getStmt();
        if (stmt->getBeginLoc() > cfgStmtElementStmt->getBeginLoc()
            || stmt->getEndLoc() < cfgStmtElementStmt->getEndLoc()) {
            return false;
        }
    }

    return true;
}

unsigned IfElseReturnChecker::getLineNumber(const Stmt *stmt, SourceManager &SM) {
    return SM.getSpellingLineNumber(stmt->getBeginLoc());
}
