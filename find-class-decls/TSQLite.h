#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>

class TSQLite
{
    sqlite3* m_db;
    char     m_sql[4096];

    TSQLite();
    
    static int CallbackGetAllCount(void* data, int argc, char** argv, char** azColName);


    static TSQLite m_gTSQLite;
public:
    static TSQLite* GetInstance();
    static int CallbackDefault(void* data, int argc, char** argv, char** azColName);
    unsigned int GetAllCount(const char* table);

    void Exec(
        const char *sql,                           /* SQL to be evaluated */
        int (*callback)(void*,int,char**,char**),  /* Callback function */
        void *                                    /* 1st argument to callback */
    );
};