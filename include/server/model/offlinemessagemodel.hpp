#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include <string>
#include <vector>

class OfflineMessageModel
{
public:
    void insertOfflineMessage(int id, std::string buffer);

    std::vector<std::string> queryOfflineMessage(int id);

    void removeOfflineMessage(int id);
};

#endif