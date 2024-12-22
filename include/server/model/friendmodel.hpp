#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include "db.h"

#include <vector>

class FriendModel
{
public:
    void insert(int id, int friendid);

    //return friends lists;
    std::vector<User> query(int id);
};

#endif