#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"

// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/Comment.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "clang/Basic/TargetInfo.h"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;

class TVisitorUtils
{
    /*
    struct TValueDeclInfo
    {
        std::string location;
        std::string declName;
        std::string typeName;
        std::string  
    };
    */
    
    ASTContext* m_context;
    clang::Rewriter m_rewriter;
    char m_sql[10240];

    template<typename T>
    std::string GetLocation(T* t);

    template<typename T>
    std::string GetParentTStmtLocation(const Stmt* pStmt);

    const char* GetFileName(FullSourceLoc& location);
    std::string GetRawCodeTxt(const Stmt* S);

    //void GetRHSExprDeclLocation(Expr* expr, std::string* finalDeclLocation, std::string* firstRefDeclLocation);
    void GetExprDeclLocation(Expr* expr, std::vector<std::string>* txts, std::vector<std::string>* locations, bool* isRelatedAddress);

public:
    TVisitorUtils(ASTContext* context);

    void ProcessNamedDecl(NamedDecl* namedDecl);
    void ProcessFunctionDecl(FunctionDecl* functionDecl);
    void ProcessStmt(Stmt* S);
    void ProcessDeclRefExpr(DeclRefExpr* declRefExpr);
    void ProcessCXXMemberCallExpr(CXXMemberCallExpr* cxxMemberCallExpr);
    void ProcessCallExpr(CallExpr* callExpr);
    void ProcessBinaryOperator(BinaryOperator* binaryOperator);
    void ProcessDeclStmt(DeclStmt* declStmt);
    void ProcessCompoundStmt(CompoundStmt* compoundStmt);
    void ProcessIfStmt(IfStmt* ifStmt);
    void ProcessMemberExpr(MemberExpr* memberExpr);
};

template<typename T>
std::string TVisitorUtils::GetLocation(T* t)
{
    std::string location;

    FullSourceLoc fullLocation = m_context->getFullLoc(t->getBeginLoc());
    if (fullLocation.isValid())
    {
        const SourceManager& sourceManager = m_context->getSourceManager();
        const char* pFileName = sourceManager.getFilename(fullLocation).data();
        // Note:不关心没有源码对应的内容
        if (pFileName == NULL)
        {
            PresumedLoc presumedLoc = sourceManager.getPresumedLoc(fullLocation);
            pFileName = presumedLoc.getFilename();
            if (pFileName == NULL)
                return location;
            else
            {
                location.append(pFileName);
                location.append(" ");
                location += std::to_string(presumedLoc.getLine());
                location.append(":");
                location += std::to_string(presumedLoc.getColumn());                
            }
        }
        else
        {
            location.append(pFileName);
            location.append(" ");
            location += std::to_string(fullLocation.getSpellingLineNumber());
            location.append(":");
            location += std::to_string(fullLocation.getSpellingColumnNumber());
        }
    }
    return location;
}

template<typename T>
std::string TVisitorUtils::GetParentTStmtLocation(const Stmt* pStmt)
{
    std::string parentTLocation;
    while (pStmt)
    {
        DynTypedNodeList parents = m_context->getParents(*pStmt);
        if (parents.size() < 1)
            pStmt = NULL;
        else
        {
            const DynTypedNode& node = parents[0];
            pStmt = node.get<Stmt>();
            if (pStmt)
            {
                if (const T* parentTStmt = dyn_cast<T>(pStmt))
                {
                    parentTLocation = GetLocation(parentTStmt);
                    break;
                }
            }
        }
    }

    return parentTLocation;
}