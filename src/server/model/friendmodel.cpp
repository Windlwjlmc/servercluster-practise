#include "friendmodel.hpp"

void FriendModel::insert(int id, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into Friend values('%d', '%d')", id, friendid);
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            LOG_INFO << "ADD FRIEND SUCCESSFULLY!";
        }
    }
}

//return friends lists;
std::vector<User> FriendModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select b.id, b.state, b.name from User b inner join Friend a on a.friendid = b.id where a.userid = %d", id);
    MySQL mysql;
    std::vector<User> uservec;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);  //未查到返回null
        if(res!=nullptr)
        {
            MYSQL_ROW row; //此方法返回的都是字符串
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setState(row[1]);
                user.setName(row[2]);
                uservec.push_back(user);
            } 
            mysql_free_result(res);
        }
    }
    return uservec;
}