#ifndef BIGTABLE_H
#define BIGTABLE_H


#include <map>

#include "ns3/event-id.h"
#include "ns3/application.h"
#include "ns3/node-container.h"
#include "ns3/internet-module.h"

#include "group-user.h"



namespace ns3 {

class BigTable : public Application {
public:
    static TypeId GetTypeId();

    BigTable();
    virtual ~BigTable();

    void setServerTable(map<pair<string, int>, string> *serverTable);
    string getServerTable(string server, int content);

    // void setNetwork(NetworkGraph* network);
    // NetworkGraph* getNetwork();

    void setClientContainers(NodeContainer* clients);
    NodeContainer* getClientContainers();

    vector<GroupUser *>& getGroups();

    GroupUser* AddUserInGroup(unsigned from, unsigned to, int content, unsigned userId);

private:
    string Ipv4AddressToString(Ipv4Address ad);

protected:
    vector<GroupUser *> groups;

private:
    NodeContainer* clientContainers;

    map<pair<string, int>, string> *serverTable;
};

}

#endif //BIGTABLE_H
