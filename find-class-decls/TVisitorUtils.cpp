#include "TVisitorUtils.h"
#include "TSQLite.h"
#include <string>
#include <vector>

#define TRELEASE 1

TVisitorUtils::TVisitorUtils(ASTContext* context)
{
    m_context = context;
    m_rewriter.setSourceMgr(context->getSourceManager(), context->getLangOpts());
}

const char* TVisitorUtils::GetFileName(FullSourceLoc& location)
{
    const SourceManager& sourceManager = m_context->getSourceManager();
    return sourceManager.getFilename(location).data();  
}

std::string TVisitorUtils::GetRawCodeTxt(const Stmt* S)
{
    clang::SourceRange sourceRange = S->getSourceRange();
    //clang::Rewriter rewriter;
    //rewriter.setSourceMgr(m_context->getSourceManager(), m_context->getLangOpts());
    return m_rewriter.getRewrittenText(sourceRange);
}

void TVisitorUtils::ProcessNamedDecl(NamedDecl* namedDecl)
{
    std::string location = GetLocation(namedDecl);
    if (location.empty())
        return;

    std::string declName = namedDecl->getQualifiedNameAsString();
    std::string typeName;
    std::string pointeeTypeName;
    if (ValueDecl* valueDecl = dyn_cast<ValueDecl>(namedDecl))
    {
        typeName = valueDecl->getType().getAsString();
        pointeeTypeName = typeName;
        const clang::Type* pType = valueDecl->getType().getTypePtr();
        if (pType)
        {
            if (auto pointerType = dyn_cast_or_null<clang::PointerType>(pType))
            {
                pointeeTypeName = pointerType->getPointeeType().getAsString();
            }
            else if (auto referenceType = dyn_cast_or_null<clang::ReferenceType>(pType))
            {
                pointeeTypeName = referenceType->getPointeeType().getAsString();
            }
        }
    }
    const char* kindName = namedDecl->getDeclKindName();

    snprintf(m_sql, sizeof(m_sql),
             "INSERT INTO NAMEDDECL (LOCATION,DECL_NAME,TYPE_NAME,POINTEE_TYPE_NAME,KIND_NAME) "
             "VALUES ('%s', '%s', '%s', '%s', '%s')",
             location.c_str(), declName.c_str(), typeName.c_str(), pointeeTypeName.c_str(),
             kindName ? kindName : "");

#if TRELEASE  
    TSQLite::GetInstance()->Exec(m_sql, TSQLite::CallbackDefault, NULL);
#else
    fprintf(stdout, "ProcessNamedDecl:[%s]\n", m_sql);
#endif

    if (FunctionDecl* functionDecl = dyn_cast<FunctionDecl>(namedDecl))
    {
        ProcessFunctionDecl(functionDecl);
    }
}

void TVisitorUtils::ProcessFunctionDecl(FunctionDecl* functionDecl)
{
    std::string location = GetLocation(functionDecl);
    if (location.empty())
        return;

    std::string bodyLocation;
    if (functionDecl->hasBody())
    {
        Stmt* body = functionDecl->getBody();
        bodyLocation = GetLocation(body);
    }


    //std::string functionType = functionDecl->getType().getAsString();
    //std::string functionName = functionDecl->getNameInfo().getName().getAsString();

    unsigned int nums = functionDecl->getNumParams();
    if (nums == 0)
    {
        snprintf(m_sql, sizeof(m_sql),
                "INSERT INTO FUNCTIONDECL (LOCATION,PARAM_INDEX,PARAM_VAR_DECL,BODY) "
                "VALUES ('%s', '-1', '', '%s')",
                location.c_str(), bodyLocation.c_str());

#if TRELEASE
            TSQLite::GetInstance()->Exec(m_sql, TSQLite::CallbackDefault, NULL);
#else
            fprintf(stdout, "ProcessFunctionDecl:[%s]\n", m_sql);     
#endif
    }
    else
    {
        for (unsigned int i = 0; i < nums; i++)
        {
            ParmVarDecl* parmVarDecl = functionDecl->parameters()[i];
            std::string parmLocation = GetLocation(parmVarDecl);
            snprintf(m_sql, sizeof(m_sql),
                "INSERT INTO FUNCTIONDECL (LOCATION,PARAM_INDEX,PARAM_VAR_DECL,BODY) "
                "VALUES ('%s', '%u', '%s', '%s')",
                location.c_str(), i, parmLocation.c_str(), bodyLocation.c_str());

#if TRELEASE
            TSQLite::GetInstance()->Exec(m_sql, TSQLite::CallbackDefault, NULL);
#else
            fprintf(stdout, "ProcessFunctionDecl:[%s]\n", m_sql);     
#endif
        }
    }
}

void TVisitorUtils::ProcessStmt(Stmt* S)
{
    const char* pStmtClassName = S->getStmtClassName();
    if (pStmtClassName == NULL)
        return;
    //fprintf(stdout, "[%s]\n", pStmtClassName);
    if (DeclRefExpr* declRefExpr = dyn_cast<DeclRefExpr>(S))
    {
        ProcessDeclRefExpr(declRefExpr);
    }
    else if (CallExpr* callExpr = dyn_cast<CallExpr>(S))
    {
        ProcessCallExpr(callExpr);
    }
    else if (BinaryOperator* binaryOperator = dyn_cast<BinaryOperator>(S))
    {
        ProcessBinaryOperator(binaryOperator);
    }
    else if (DeclStmt* declStmt = dyn_cast<DeclStmt>(S))
    {
        ProcessDeclStmt(declStmt);
    }
    else if (CompoundStmt* compoundStmt = dyn_cast<CompoundStmt>(S))
    {
        ProcessCompoundStmt(compoundStmt);
    }
    else if (IfStmt* ifStmt = dyn_cast<IfStmt>(S))
    {
        ProcessIfStmt(ifStmt);
    }
    else if (MemberExpr* memberExpr = dyn_cast<MemberExpr>(S))
    {
        ProcessMemberExpr(memberExpr);
    }
}

void TVisitorUtils::ProcessDeclRefExpr(DeclRefExpr* declRefExpr)
{
    std::string location = GetLocation(declRefExpr);
    if (location.empty())
        return;

    std::string rawCodeTxt = GetRawCodeTxt(declRefExpr);
    ValueDecl* valueDecl = declRefExpr->getDecl();
    std::string declLocation = GetLocation(valueDecl);

    snprintf(m_sql, sizeof(m_sql),
             "INSERT INTO DECLREF (LOCATION,DECL_REF,DECL_TXT) "
             "VALUES ('%s', '%s', '%s')",
             location.c_str(), declLocation.c_str(), rawCodeTxt.c_str());

#if TRELEASE
    TSQLite::GetInstance()->Exec(m_sql, TSQLite::CallbackDefault, NULL);
#else
    fprintf(stdout, "ProcessDeclRefExpr:[%s]\n", m_sql);
#endif
}

void TVisitorUtils::ProcessCallExpr(CallExpr* callExpr)
{
    std::string location = GetLocation(callExpr);
    if (location.empty())
        return;

    bool hasDirectCallee = false;
    std::string declLocation;
    FunctionDecl* functionDecl = callExpr->getDirectCallee();
    if (functionDecl)
    {
        hasDirectCallee = true;
        declLocation = GetLocation(functionDecl);
    }
    else
    {
        hasDirectCallee = false;
        Decl* decl = callExpr->getCalleeDecl();
        if (decl == NULL)
            return;
        declLocation = GetLocation(decl);
    }

    std::string parentCompoundStmtLocation = GetParentTStmtLocation<CompoundStmt>(callExpr);

    unsigned int nums = callExpr->getNumArgs();  
    if (nums == 0)
    {
        snprintf(m_sql, sizeof(m_sql),
            "INSERT INTO CALLREF (LOCATION,ARG_INDEX,ARG_TXT,ARG_FINAL_DECL,ARG_FIRST_REFDECL,CALL_REF,COMPOUNDSTMT,HAS_DIRECT_CALLEE) "
            "VALUES ('%s', '-1', '', '','', '%s', '%s','%s')",
            location.c_str(),
            declLocation.c_str(), 
            parentCompoundStmtLocation.c_str(), hasDirectCallee ? "TRUE" : "FALSE");
#if TRELEASE
        TSQLite::GetInstance()->Exec(m_sql, TSQLite::CallbackDefault, NULL);
#else
        fprintf(stdout, "ProcessCallExpr:[%s]\n", m_sql);
#endif
    }
    else
    {
        for (unsigned int i = 0; i < nums; i++)
        {
            Expr* expr = callExpr->getArg(i);
            std::string exprLocation = GetLocation(expr);
            std::string rawCodeTxt;
            if (exprLocation.empty() == false)
                rawCodeTxt = GetRawCodeTxt(expr);

            /*
            std::string finalDeclLocation, firstRefDeclLocation;
            GetRHSExprDeclLocation(expr, &finalDeclLocation, &firstRefDeclLocation);
            */
            
            std::vector<std::string> txts, locations;
            bool isRelatedAddress = false;
            GetExprDeclLocation(expr, &txts, &locations, &isRelatedAddress);
            if (txts.size() == 0 || locations.size() == 0)
                return;

            snprintf(m_sql, sizeof(m_sql),
                "INSERT INTO CALLREF (LOCATION,ARG_INDEX,ARG_TXT,ARG_FINAL_DECL,ARG_FIRST_REFDECL,CALL_REF,COMPOUNDSTMT,HAS_DIRECT_CALLEE) "
                "VALUES ('%s', '%u', '%s', '%s','%s', '%s', '%s','%s')",
                location.c_str(),
                i, rawCodeTxt.c_str(),
                locations[0].c_str(), locations[locations.size() - 1].c_str(),
                declLocation.c_str(), 
                parentCompoundStmtLocation.c_str(), hasDirectCallee ? "TRUE" : "FALSE");
#if TRELEASE
            TSQLite::GetInstance()->Exec(m_sql, TSQLite::CallbackDefault, NULL);
#else
            fprintf(stdout, "ProcessCallExpr:[%s]\n", m_sql);
#endif
        }
    }
}

void TVisitorUtils::ProcessCXXMemberCallExpr(CXXMemberCallExpr* cxxMemberCallExpr)
{
    std::string location = GetLocation(cxxMemberCallExpr);
    if (location.empty())
        return;
    fprintf(stdout, "ProcessCXXMemberCallExpr:[%s]\n", location.c_str());
}

void TVisitorUtils::ProcessBinaryOperator(BinaryOperator* binaryOperator)
{
    std::string location = GetLocation(binaryOperator);
    if (location.empty())
        return;

    std::string opsName = BinaryOperator::getOpcodeStr(binaryOperator->getOpcode()).str();
    if (opsName != "=")
        return;

    Expr* rhs = binaryOperator->getRHS();

    std::string rhsFinalDeclLocation, rhsFirstRefDeclLocation, rhsTxt;
    /*
    GetRHSExprDeclLocation(rhs, &rhsFinalDeclLocation, &rhsFirstRefDeclLocation);
    */
    std::vector<std::string> txtsRHS, locationsRHS;
    bool isRelatedAddress = false;
    GetExprDeclLocation(rhs, &txtsRHS, &locationsRHS, &isRelatedAddress);
    if (txtsRHS.size() == 0 || locationsRHS.size() == 0)
        return;

    rhsTxt = GetRawCodeTxt(rhs);
    rhsFinalDeclLocation = locationsRHS[0];
    rhsFirstRefDeclLocation = locationsRHS[locationsRHS.size() - 1];
    
    

    Expr* lhs = binaryOperator->getLHS();
    isRelatedAddress = false;
    std::vector<std::string> txts, locations;
    GetExprDeclLocation(lhs, &txts, &locations, &isRelatedAddress);

    if (txts.size() == 0 || locations.size() == 0)
    {
        //fprintf(stderr, "ProcessBinaryOperator:[%s]", location.c_str());
        //binaryOperator->dump();
        //exit(0);
        return;
    }

    std::string parentCompoundStmtLocation = GetParentTStmtLocation<CompoundStmt>(binaryOperator);

    std::string wholeTxt = txts[0];
    std::string refDeclLocation = locations[locations.size() - 1];
    for (unsigned int i = 0; i < txts.size(); i++)
    {
        std::string txt = txts[i];
        std::string suffix = wholeTxt;
        std::string::size_type pos = suffix.find(txt);
        if (pos == std::string::npos)
        {
            //fprintf(stderr, "ProcessBinaryOperator:[%s]", location.c_str());
            //binaryOperator->dump();
            return;
        }
        suffix.replace(suffix.find(txt), txt.length(), "");

        snprintf(m_sql, sizeof(m_sql),
             "INSERT INTO BINARYOPERATOR (LOCATION,OPERATOR_KIND,COMPOUNDSTMT,RHS_TXT,RHS_FINAL_DECL,RHS_FIRST_REFDECL,LHS_DECL_INDEX,LHS_TXT,LHS_DECL,LHS_SUFFIX,LHS_FINAL_DECL,LHS_FIRST_REFDECL,LHS_IS_RELATED_ADDRESS) "
             "VALUES ('%s','%s', '%s', '%s','%s','%s', '%u','%s','%s','%s','%s', '%s','%s')",
             location.c_str(), opsName.c_str(),
             parentCompoundStmtLocation.c_str(),
             rhsTxt.c_str(), rhsFinalDeclLocation.c_str(), rhsFirstRefDeclLocation.c_str(),
             i, txt.c_str(), locations[i].c_str(), suffix.c_str(), locations[0].c_str(),
             refDeclLocation.c_str(), isRelatedAddress ? "TRUE" : "FALSE");

#if TRELEASE
        TSQLite::GetInstance()->Exec(m_sql, TSQLite::CallbackDefault, NULL);
#else
        fprintf(stdout, "ProcessBinaryOperator:[%s]\n", m_sql);
#endif     
    }
}

void TVisitorUtils::ProcessDeclStmt(DeclStmt* declStmt)
{
    std::string location = GetLocation(declStmt);
    if (location.empty())
        return;

    unsigned int i = 0;
    auto itD = declStmt->decl_begin();
    auto itS = declStmt->child_begin();
    for ( ; itD != declStmt->decl_end() && itS != declStmt->child_end(); 
        itD++, itS++, i++)
    {
        Decl* decl = *itD;
        std::string declLocation = GetLocation(decl);

        Stmt* S = *itS;
        std::string txt;
        if (declLocation.empty() == false)
            txt = GetRawCodeTxt(S);

        snprintf(m_sql, sizeof(m_sql),
             "INSERT INTO DECLSTMT (LOCATION,DECL_INDEX,DECL_REF,DECL_TXT) "
             "VALUES ('%s', '%u', '%s', '%s')",
             location.c_str(), i, declLocation.c_str(), txt.c_str());

#if TRELEASE
        TSQLite::GetInstance()->Exec(m_sql, TSQLite::CallbackDefault, NULL);
#else
        fprintf(stdout, "ProcessDeclStmt:[%s]\n", m_sql);
#endif             
    }
}

void TVisitorUtils::GetExprDeclLocation(Expr* expr, std::vector<std::string>* txts, std::vector<std::string>* locations, bool* isRelatedAddress)
{
    Stmt* pStmt = expr;
    do
    {
        /*
        const char* pStmtClassName = pStmt->getStmtClassName();
        if (pStmtClassName != NULL)
        {
            fprintf(stdout, "GetExprDeclLocation %s\n", pStmtClassName);
        }
        */

        if (MemberExpr* memberExpr = dyn_cast<MemberExpr>(pStmt))
        {
            std::string codeTxt;
            std::string location;
            ValueDecl* valueDecl = memberExpr->getMemberDecl();
            if (valueDecl)
            {
                location = GetLocation(valueDecl);
                if (location.empty() == false)
                    codeTxt = GetRawCodeTxt(memberExpr);
            }
            locations->push_back(location);
            txts->push_back(codeTxt);

            *isRelatedAddress = true;
        }
        else if (DeclRefExpr* declRefExpr = dyn_cast<DeclRefExpr>(pStmt))
        {
            std::string codeTxt;
            std::string location;
            ValueDecl* valueDecl = declRefExpr->getDecl();
            if (valueDecl)
            {
                location = GetLocation(valueDecl);
                if (location.empty() == false)
                    codeTxt = GetRawCodeTxt(declRefExpr);
            }
            locations->push_back(location);
            txts->push_back(codeTxt);
        }
        else if (UnaryOperator* unaryOperator = dyn_cast<UnaryOperator>(pStmt))
        {
            std::string opsName = UnaryOperator::getOpcodeStr(unaryOperator->getOpcode()).str();
            if (opsName == "*")
            {
                *isRelatedAddress = true;
            }
            //fprintf(stdout, "GetLHSExprDeclLocation: opsName = %s\n", opsName.c_str());     
        }

        StmtIterator it = pStmt->child_begin();
        if (it != pStmt->child_end())
            pStmt = *it;
        else
            break;
    } while (true);
}

/*
void TVisitorUtils::GetRHSExprDeclLocation(Expr* expr, std::string* finalDeclLocation, std::string* firstRefDeclLocation)
{
    bool hasFoundFinalDecl = false;
    bool hasFoundFirstRefDecl = false;

    Stmt* pStmt = expr;
    do
    {
        if (MemberExpr* memberExpr = dyn_cast<MemberExpr>(pStmt))
        {
            if (hasFoundFinalDecl == false)
            {
                hasFoundFinalDecl = true;
                ValueDecl* valueDecl = memberExpr->getMemberDecl();
                if (valueDecl)
                    finalDeclLocation->append(GetLocation(valueDecl));
            }
        }
        else if (DeclRefExpr* declRefExpr = dyn_cast<DeclRefExpr>(pStmt))
        {
            if (hasFoundFirstRefDecl == false)
            {
                hasFoundFirstRefDecl = true;

                ValueDecl* valueDecl = declRefExpr->getDecl();
                if (valueDecl)
                {
                    std::string location = GetLocation(valueDecl);
                    firstRefDeclLocation->append(location);
                    if (hasFoundFinalDecl == false)
                    {
                        hasFoundFinalDecl = true;
                        finalDeclLocation->append(location);                        
                    }
                }
            }
        }

        if (hasFoundFinalDecl && hasFoundFirstRefDecl)
            break;

        StmtIterator it = pStmt->child_begin();
        if (it != pStmt->child_end())
            pStmt = *it;
        else
            break;
    } while (true);
}
*/

void TVisitorUtils::ProcessCompoundStmt(CompoundStmt* compoundStmt)
{
    std::string location = GetLocation(compoundStmt);
    if (location.empty())
        return;
    
    std::string code = GetRawCodeTxt(compoundStmt);

    bool hasFoundCompound = false;
    bool hasFoundIf = false;
    std::string parentCompoundLocation;
    std::string parentIfLocation;

    const Stmt* pStmt = compoundStmt;

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
                if (hasFoundCompound == false)
                {
                    if (const CompoundStmt* parentCompoundStmt = dyn_cast<CompoundStmt>(pStmt))
                    {
                        hasFoundCompound = true;
                        parentCompoundLocation = GetLocation(parentCompoundStmt);
                    }
                }

                if (hasFoundIf == false)
                {
                    if (const IfStmt* parentIfStmt = dyn_cast<IfStmt>(pStmt))
                    {
                        hasFoundIf = true;
                        parentIfLocation = GetLocation(parentIfStmt);
                    }
                }
            }
        }

        if (hasFoundCompound && hasFoundIf)
            break;
    }


    snprintf(m_sql, sizeof(m_sql),
             "INSERT INTO COMPOUNDSTMT (LOCATION,PARENT_COMPOUNDSTMT,PARENT_IFSTMT) "
             "VALUES ('%s', '%s', '%s')",
             location.c_str(), parentCompoundLocation.c_str(), parentIfLocation.c_str());

#if TRELEASE
    TSQLite::GetInstance()->Exec(m_sql, TSQLite::CallbackDefault, NULL);
#else
    fprintf(stdout, "ProcessCompoundStmt:[%s]\n", m_sql);
#endif
}

void TVisitorUtils::ProcessIfStmt(IfStmt* ifStmt)
{
    std::string location = GetLocation(ifStmt);
    if (location.empty())
        return;
    
    std::string code = GetRawCodeTxt(ifStmt);

    Stmt* thenStmt = ifStmt->getThen();
    Stmt* elseStmt = ifStmt->getElse();
    std::string thenLocation, elseLocation;
    if (thenStmt)
        thenLocation = GetLocation(thenStmt);
    if (elseStmt)
        elseLocation = GetLocation(elseStmt);

    std::string parentIfLocation = GetParentTStmtLocation<IfStmt>(ifStmt);

    snprintf(m_sql, sizeof(m_sql),
             "INSERT INTO IFSTMT (LOCATION,THENSTMT,ELSESTMT,PARENT_IFSTMT) "
             "VALUES ('%s', '%s', '%s', '%s')",
             location.c_str(), thenLocation.c_str(), elseLocation.c_str(), parentIfLocation.c_str());

#if TRELEASE
    TSQLite::GetInstance()->Exec(m_sql, TSQLite::CallbackDefault, NULL);
#else
    fprintf(stdout, "ProcessIfStmt:[%s]\n", m_sql);
#endif
}

void TVisitorUtils::ProcessMemberExpr(MemberExpr* memberExpr)
{
    std::string location = GetLocation(memberExpr);
    if (location.empty())
        return;

    ValueDecl* valueDecl = memberExpr->getMemberDecl();
    if (valueDecl == NULL)
        return;

    std::string valueDeclLocation = GetLocation(valueDecl);
    if (valueDeclLocation.empty())
        return;

    snprintf(m_sql, sizeof(m_sql),
             "INSERT INTO MEMBEREXPR (LOCATION,DECL) "
             "VALUES ('%s', '%s')",
             location.c_str(), valueDeclLocation.c_str());

#if TRELEASE
    TSQLite::GetInstance()->Exec(m_sql, TSQLite::CallbackDefault, NULL);
#else
    fprintf(stdout, "ProcessMemberExpr:[%s]\n", m_sql);
#endif
}