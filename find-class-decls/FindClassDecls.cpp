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





#include <string>
#include <experimental/filesystem>
#include "TSQLite.h"
#include "TConfigurationManager.h"
#include "THeaderFileManager.h"
#include "TVisitorUtils.h"
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include <memory>
#include <cstring>

class FindNamedClassVisitor
  : public RecursiveASTVisitor<FindNamedClassVisitor> {
public:
  explicit FindNamedClassVisitor(ASTContext *Context)
    : Context(Context), TUtils(Context) {}
    
    bool VisitNamedDecl(NamedDecl* namedDecl)
    {
      TUtils.ProcessNamedDecl(namedDecl);
      return true;
    }

    bool VisitStmt(Stmt* S)
    {
      TUtils.ProcessStmt(S);
      return true;
    }
private:
  ASTContext *Context;
  TVisitorUtils TUtils;
};


class FindNamedClassConsumer : public clang::ASTConsumer {
public:
  explicit FindNamedClassConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    //Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    TranslationUnitDecl* decl = Context.getTranslationUnitDecl();

    //std::error_code ec;
    //raw_fd_ostream raw("/home/tongtong/llvm/test/test1.ast.txt", ec);
    //decl->dump(raw, true);
    Visitor.TraverseDecl(decl);
  }
private:
  FindNamedClassVisitor Visitor;
};

class FindNamedClassAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    //ASTUnit* astUnit = ASTUnit::LoadFromCompilerInvocationAction(std::make_shared<clang::CompilerInvocation>(Compiler.getInvocation()), Compiler.getPCHContainerOperations(), llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine>(&Compiler.getDiagnostics()));
    //astUnit->Save("/home/tongtong/llvm/test/test.ast.bin");
    //llvm::outs() << "ASTUnit=" << astUnit << "\n";
    return std::make_unique<FindNamedClassConsumer>(&Compiler.getASTContext());
  }
};

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");


unsigned int gItemCnt, gItemIt = 1;

static int Run(void* data, int argc, char** argv, char** azColName)
{
  char* exeName = (char*)data;
  char* dbModule   = argv[0];
  char* dbSrcs     = argv[1];
  char* dbIncludes = argv[2];
  char* dbState    = argv[5];

  if (std::strcmp(dbState, "DONE") == 0)
  {
    fprintf(stdout, "[%u/%u] '%s' has been parsed before.\n", gItemIt, gItemCnt, dbModule);
    gItemIt++;
    return 0;
  }
  else if (dbSrcs == NULL || *dbSrcs == 0)
  {
    fprintf(stdout, "[%u/%u] '%s' no source files, no need parse.\n", gItemIt, gItemCnt, dbModule);
    gItemIt++;
    return 0;    
  }

  fprintf(stdout, "[%u/%u] '%s' is in parsing ... ...\n", gItemIt, gItemCnt, dbModule);

  //clock_t tStart = clock();
  boost::posix_time::ptime tStart = boost::posix_time::microsec_clock::local_time();

  std::experimental::filesystem::path logPath(TConfigurationManager::GetInstance()->GetOutputLogPath());
  std::experimental::filesystem::path stdOutPath = logPath;
  std::experimental::filesystem::path stdErrPath = logPath;

  stdOutPath.append(dbModule + std::string(".stdout.log"));
  stdErrPath.append(dbModule + std::string(".stderr.log"));
  FILE* fpOut = freopen(stdOutPath.c_str(), "w", stdout);
  FILE* fpErr = freopen(stdErrPath.c_str(), "w", stderr);
  /********************************************************************/
  std::vector<std::string> toolArgv;
  toolArgv.push_back(exeName);
  toolArgv.push_back("--extra-arg=-std=c++17");
  toolArgv.push_back("--extra-arg=-D_LINUX");
  toolArgv.push_back("--extra-arg=-D__XIAOMI_CAMERA__");
  toolArgv.push_back("--extra-arg=-DANDROID");
  toolArgv.push_back("--extra-arg=-DCAMX");
  toolArgv.push_back("--extra-arg=-DCAMX_ANDROID_API=32");
  toolArgv.push_back("--extra-arg=-D_FORTIFY_SOURCE"); 
  ///////////
  std::vector<std::string> includes;
  boost::split(includes, dbIncludes, boost::is_any_of(" "));
  THeaderFileManager::GetInstance()->GenIncludeArgs(includes, &toolArgv);
  ///////////
  std::vector<std::string> srcs;
  boost::split(srcs, dbSrcs, boost::is_any_of(" "));
  bool isCpp = true;
  THeaderFileManager::GetInstance()->GenSrcArgs(srcs, &toolArgv, &isCpp);
  if (isCpp == false)
  {
    toolArgv.erase(toolArgv.begin() + 1);
  }
  ///////////
  toolArgv.push_back("--");
  ///////////
  int toolArgc = toolArgv.size();
  std::unique_ptr<const char*[]> pToolArgv = std::make_unique<const char*[]>(toolArgc);
  //const char** pToolArgv = new const char*[toolArgc];
  for (int i = 0; i < toolArgc; i++)
  {
    pToolArgv[i] = toolArgv[i].c_str();
    fprintf(stdout, "\t%s\n", pToolArgv[i]);
  }
  /********************************************************************/
  // 开启事务
  TSQLite::GetInstance()->Exec("BEGIN", TSQLite::CallbackDefault, NULL);
  /********************************************************************/

  auto ExpectedParser = CommonOptionsParser::create(toolArgc, pToolArgv.get(), MyToolCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());
  
  Tool.run(newFrontendActionFactory<FindNamedClassAction>().get());

  /********************************************************************/
  fflush(fpOut);
  fflush(fpErr);
  freopen("/dev/tty", "w", stdout);
  freopen("/dev/tty", "w", stderr);
  /********************************************************************/


  /********************************************************************/
  // 提交事务
  TSQLite::GetInstance()->Exec("COMMIT", TSQLite::CallbackDefault, NULL);
  /********************************************************************/

  bool hasError = false;
  std::ifstream fin(stdErrPath.c_str());
  std::string line;
  while (std::getline(fin, line))
  {
    std::size_t found = line.find("error generated");
    if (found != std::string::npos)
    {
      hasError = true;
      break;
    }
  }
  fin.close();

  if (hasError)
  {
    fprintf(stdout, "[%u/%u] '%s' parse error. program exit.\n", gItemIt, gItemCnt, dbModule);
    exit(0);
  }
  else
  {
    //clock_t tEnd = clock();
    boost::posix_time::ptime tEnd = boost::posix_time::microsec_clock::local_time();
    //double duration = (double)(tEnd - tStart);
    boost::posix_time::time_duration duration = tEnd - tStart;
    fprintf(stdout, "[%u/%u] '%s' parse completely. It takes %s.\n", gItemIt, gItemCnt, dbModule,
      boost::posix_time::to_simple_string(duration).c_str());
    gItemIt++;

    char sql[4096];
    snprintf(sql, sizeof(sql), "UPDATE CAMERAMODULES SET PARSE_STATE='DONE' WHERE LOCAL_MODULE='%s'", dbModule);
    TSQLite::GetInstance()->Exec(sql, TSQLite::CallbackDefault, NULL);
  }
  
  return 0;
}

int main(int argc, const char **argv)
{
#if 1
  gItemCnt = TSQLite::GetInstance()->GetAllCount("CAMERAMODULES");
  fprintf(stdout, "CAMERAMODULES = %u\n", gItemCnt);

  TSQLite::GetInstance()->Exec(
    "SELECT * FROM CAMERAMODULES",
    Run,
    (void*)argv[0]);
#endif



#if 0
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());
  
  return Tool.run(newFrontendActionFactory<FindNamedClassAction>().get());
#endif
}