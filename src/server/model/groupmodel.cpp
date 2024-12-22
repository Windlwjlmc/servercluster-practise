#include "groupmodel.hpp"
#include "db.h"
//create group
int GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into AllGroup values(id,'%s','%s')", 
        group.getName().c_str(), group.getDesc().c_str());
    MySQL mysql;

    if(mysql.connect())
    {
        mysql.update(sql);
    }

    ::memset(sql, 0, sizeof(sql));
    sprintf(sql, "select id from AllGroup where groupname = '%s'", group.getName().c_str());

    MySQL mysql2;
    if(mysql2.connect())
    {
        MYSQL_RES* res = mysql2.query(sql);
        MYSQL_ROW row = mysql_fetch_row(res);
        group.setID(atoi(row[0]));
        return group.getId();
    }

    return -1;
}

//join a group
void GroupModel::joinGroup(int id, int groupid, std::string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into GroupUser values('%d','%d','%s')", groupid, id, role.c_str());

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

//query group info
std::vector<Group> GroupModel::queryGroups(int id)
{
    char sql[1024] = {0};
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from AllGroup a inner join \
        GroupUser b on b.groupid = a.id where b.userid='%d'", id);
    
    std::vector<Group> gvec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setID(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                gvec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    return gvec;
}

//query group members info excpet yourself, send msg to members by use this func.
std::vector<int> GroupModel::queryGroupUsers(int id, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from GroupUser where groupid = %d and userid != %d",
        groupid, id);
    
    std::vector<int> vec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res!=nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!= nullptr)
            {
                vec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return vec;
}