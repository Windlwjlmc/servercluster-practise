#include "usermodel.hpp"
#include "db.h"
#include <iostream>

bool UserModel::insert(User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name, password, state) values('%s','%s','%s')",
        user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            LOG_INFO << "UPDATE SUCCESSFULLY!";
            //获取插入成功用户的primarykey
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from User where id = %d", id);
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);  //未查到返回null
        if(res!=nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res); //此方法返回的都是字符串
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPassword(row[2]);
            user.setState(row[3]);
            mysql_free_result(res);
            return user;
        }
    }
    return User();
}

bool UserModel::updateState(User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "update User set state = '%s' where id = '%d'", user.getState().c_str(), user.getId());
    
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

void UserModel::resetAll()
{
    char sql[1024] = "update User set state = 'offline' where state = 'online'";
    MySQL mysql;
    
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}