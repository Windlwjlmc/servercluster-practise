#ifndef DB_H
#define DB_H

#include <muduo/base/Logging.h>
#include <mysql/mysql.h>
#include <string>

class MySQL
{
public:
    MySQL();
    ~MySQL();

    bool connect();

    bool update(std::string sql);

    MYSQL_RES* query(std::string sql);

    MYSQL* getConnection();

private:
    MYSQL* conn_;
};

#endif