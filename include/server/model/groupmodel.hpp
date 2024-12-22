#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <iostream>
#include <string>

class GroupModel
{
public:
    //create group
    int createGroup(Group &group);

    //join a group
    void joinGroup(int id, int groupid, std::string role);

    //query group info
    std::vector<Group> queryGroups(int id);

    //query group members info excpet yourself, send msg to members by use this func.
    std::vector<int> queryGroupUsers(int id, int groupid);
};

#endif