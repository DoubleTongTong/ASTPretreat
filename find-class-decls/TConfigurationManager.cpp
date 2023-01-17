#include "TConfigurationManager.h"
#include <stdio.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

TConfigurationManager TConfigurationManager::m_gConfigurationManager;

TConfigurationManager* TConfigurationManager::GetInstance()
{
    return &m_gConfigurationManager;
}

TConfigurationManager::TConfigurationManager()
{
    boost::property_tree::ptree root;
    try
    {
        boost::property_tree::xml_parser::read_xml("tConfiguration.xml", root);
    }
    catch(const std::exception& e)
    {
        fprintf(stderr, "read_xml failed : %s\n", e.what());
        exit(0);
    }

    m_databasePath         = root.get<std::string>("Configuration.DatabasePath");
    fprintf(stdout, "Database Path = %s\n", m_databasePath.c_str());

    m_manualHeaderFilePath = root.get<std::string>("Configuration.ManualHeaderFilePath");
    fprintf(stdout, "Manual Header File Path = %s\n", m_manualHeaderFilePath.c_str());

    m_outputLogPath        = root.get<std::string>("Configuration.OutputLogPath");
    fprintf(stdout, "Output Log Path = %s\n", m_outputLogPath.c_str());

    m_codeRootPath         = root.get<std::string>("Configuration.CodeRootPath");
    fprintf(stdout, "Code Root Path = %s\n", m_codeRootPath.c_str());
}

std::string TConfigurationManager::GetDatabasePath()
{
    return m_databasePath;
}

std::string TConfigurationManager::GetManualHeaderFilePath()
{
    return m_manualHeaderFilePath;
}

std::string TConfigurationManager::GetOutputLogPath()
{
    return m_outputLogPath;
}

std::string TConfigurationManager::GetCodeRootPath()
{
    return m_codeRootPath;
}