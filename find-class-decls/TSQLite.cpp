#include "TSQLite.h"
#include "TConfigurationManager.h"
#include <string>

TSQLite TSQLite::m_gTSQLite;

TSQLite* TSQLite::GetInstance()
{
    return &m_gTSQLite;
}

TSQLite::TSQLite()
{
    std::string dbPath = TConfigurationManager::GetInstance()->GetDatabasePath();
    int rc = sqlite3_open(dbPath.c_str(), &m_db);
    
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(m_db));
        exit(0);
    }
    else
    {
        fprintf(stdout, "Opened database successfully\n");
    }
}

unsigned int TSQLite::GetAllCount(const char* table)
{
    unsigned int cnt = 0;

    snprintf(m_sql, sizeof(m_sql), "SELECT COUNT(*) FROM %s", table);
    fprintf(stdout, "%s\n", m_sql);
    
    char* zErrMsg = NULL;
    int rc = sqlite3_exec(m_db, m_sql, CallbackGetAllCount, &cnt, &zErrMsg);
    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        exit(0);
    }
    else
    {
        fprintf(stdout, "TSQLite::GetAllCount Operation done successfully\n");
    }

    return cnt;
}

int TSQLite::CallbackGetAllCount(void* data, int argc, char** argv, char** azColName)
{
    if (argc != 1)
    {
        fprintf(stderr, "GetAllCount failed: argc != 1");
        exit(0);  
    } 

    unsigned int* cnt = (unsigned int*)data;
    sscanf(argv[0], "%u", cnt);

    return 0;
}

void TSQLite::Exec(
    const char* sql,                           /* SQL to be evaluated */
    int (*callback)(void*,int,char**,char**),  /* Callback function */
    void* argument                             /* 1st argument to callback */
)
{
    //fprintf(stdout, "Exec : %s\n", sql);

    char* zErrMsg = NULL;
    int rc = sqlite3_exec(m_db, sql, callback, argument, &zErrMsg);
    if(rc != SQLITE_OK)
    {
        // Note: stdout
        fprintf(stdout, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        //exit(0);
    }
    else
    {
        //fprintf(stdout, "TSQLite::Exec Operation done successfully\n");
    }
}

int TSQLite::CallbackDefault(void* data, int argc, char** argv, char** azColName)
{
    return 0;
}