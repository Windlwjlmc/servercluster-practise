#ifndef USER_H
#define USER_H

#include <string>

class User
{
public:
    User(int id = -1, std::string name="", std::string password="", std::string state = "offline")
    {
        this->id = id;
        this->password = password;
        this->name = name;
        this->state = state;
    }

    void setId(int id) { this->id = id; }
    void setName(std::string name) { this->name = name; }
    void setPassword(std::string password) { this->password = password; }
    void setState(std::string state) { this->state = state; }

    int getId() { return id; }
    std::string getName() { return name; }
    std::string getPassword() { return password; }
    std::string getState() { return state; }   

private:
    int id;
    std::string name;
    std::string password;
    std::string state;
};

#endif