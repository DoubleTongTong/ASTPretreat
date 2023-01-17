#include "TSQLite.h"
#include "THeaderFileManager.h"
#include "TConfigurationManager.h"
#include <stdio.h>
#include <algorithm>
#include <fstream>
#include <experimental/filesystem>
#include <cstring>

THeaderFileManager THeaderFileManager::m_gHeaderFileManager;

THeaderFileManager* THeaderFileManager::GetInstance()
{
    return &m_gHeaderFileManager;
}

THeaderFileManager::THeaderFileManager()
{
    TSQLite::GetInstance()->Exec(
        "SELECT * FROM PREBUILTINC",
        CallbackGetPreBuiltIncs,
        &m_files
    );

    /************************************************************
     * 修正
     ***********************************************************/
    // Note: size_t
    {
        auto it = std::find(m_files.begin(), m_files.end(), "prebuilts/vndk/v31/x86_64/include/external/libcxx/include");
        if (it != m_files.end())
        {
        m_files.insert(it + 1, "prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.17-4.8/lib/gcc/x86_64-linux/4.8.3/include");
        }
    }

    // Note: signal
    {
        auto it = std::find(m_files.begin(), m_files.end(), "prebuilts/vndk/v31/x86_64/include/generated-headers/bionic/libc/libc/android_vendor.31_x86_64_shared/gen/include");
        if (it != m_files.end())
        {
        m_files.insert(it + 1, "prebuilts/vndk/v31/x86_64/include/bionic/libc/kernel/uapi/asm-generic");
        }
    }
    /************************************************************/

    GetManualHeaderFiles();
}

int THeaderFileManager::CallbackGetPreBuiltIncs(void* data, int argc, char** argv, char** azColName)
{
    //fprintf(stdout, "CallbackGetPreBuiltIncs : %s\n", argv[0]);
    std::vector<std::string>* files = (std::vector<std::string>*)data;
    files->push_back(argv[0]);
    return 0;
}

void THeaderFileManager::GetManualHeaderFiles()
{
    std::string path = TConfigurationManager::GetInstance()->GetManualHeaderFilePath();

    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line))
    {
        //fprintf(stdout, "GetManualHeaderFiles : %s\n", line.c_str());
        m_files.push_back(line);
    }
}

void THeaderFileManager::GenIncludeArgs(const std::vector<std::string>& includes, std::vector<std::string>* args)
{
    unsigned int i;
    std::experimental::filesystem::path codeRootPath(TConfigurationManager::GetInstance()->GetCodeRootPath());

    for (i = 0; i < includes.size(); i++)
    {
        std::experimental::filesystem::path path = codeRootPath;
        path.append(includes[i]);

        std::string arg("--extra-arg=-I");
        arg.append(path.c_str());

        args->push_back(arg);
    }

    for (i = 0; i < m_files.size(); i++)
    {
        std::experimental::filesystem::path path = codeRootPath;
        path.append(m_files[i]);

        std::string arg("--extra-arg=-I");
        arg.append(path.c_str());

        args->push_back(arg);
    }   
}

void THeaderFileManager::GenSrcArgs(const std::vector<std::string>& srcs, std::vector<std::string>* args, bool* isCpp)
{
    *isCpp = true;
    std::experimental::filesystem::path codeRootPath(TConfigurationManager::GetInstance()->GetCodeRootPath());
    for (unsigned int i = 0; i < srcs.size(); i++)
    {
        std::experimental::filesystem::path path = codeRootPath;
        path.append(srcs[i]);

        args->push_back(path.c_str());
        if (std::strcmp(path.extension().c_str(), ".cpp"))
            *isCpp = false;
    }
}