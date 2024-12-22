#include "offlinemessagemodel.hpp"
#include "db.h"
#include <iostream>

void OfflineMessageModel::insertOfflineMessage(int id, std::string buffer)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into OfflineMessage values(%d, '%s')", id, buffer.c_str());
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

std::vector<std::string> OfflineMessageModel::queryOfflineMessage(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select message from OfflineMessage where userid=%d", id);
    MySQL mysql;
    std::vector<std::string> vec_;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                vec_.push_back(row[0]);
            }

            mysql_free_result(res);
        }
    }
    return vec_;
}

void OfflineMessageModel::removeOfflineMessage(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from OfflineMessage where userid=%d", id);
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}