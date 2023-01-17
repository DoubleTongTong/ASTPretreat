#include <vector>
#include <string>

class THeaderFileManager
{
    static THeaderFileManager m_gHeaderFileManager;
    THeaderFileManager();

    std::vector<std::string> m_files;

    static int CallbackGetPreBuiltIncs(void* data, int argc, char** argv, char** azColName);

    void GetManualHeaderFiles();
public:
    static THeaderFileManager* GetInstance();

    void GenIncludeArgs(const std::vector<std::string>& includes, std::vector<std::string>* args);
    void GenSrcArgs(const std::vector<std::string>& srcs, std::vector<std::string>* args, bool* isCpp);
};