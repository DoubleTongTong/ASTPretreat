#include <string>

class TConfigurationManager
{
    static TConfigurationManager m_gConfigurationManager;
    TConfigurationManager();

    std::string m_databasePath;
    std::string m_manualHeaderFilePath;
    std::string m_outputLogPath;
    std::string m_codeRootPath;
public:
    static TConfigurationManager* GetInstance();

    std::string GetDatabasePath();    
    std::string GetManualHeaderFilePath();
    std::string GetOutputLogPath();
    std::string GetCodeRootPath();
};