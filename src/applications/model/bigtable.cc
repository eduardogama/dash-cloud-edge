#include "bigtable.h"

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/string.h"
#include "ns3/simulator.h"

#include <iostream>
#include <fstream>


using namespace std;


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BigTable");

NS_OBJECT_ENSURE_REGISTERED(BigTable);


TypeId BigTable::GetTypeId (void)
{
  static TypeId tid = TypeId("ns3::BigTable")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<BigTable> ()
  ;
  return tid;
}

BigTable::BigTable()
{

}

BigTable::~BigTable()
{

}

void BigTable::setServerTable(map<pair<string, int>, string> *serverTable)
{
	this->serverTable = serverTable;
}

string BigTable::getServerTable(string server, int content)
{
	return (*serverTable)[{server, content}];
}

void BigTable::setClientContainers(NodeContainer* clients)
{
    this->clientContainers = clients;
}

NodeContainer* BigTable::getClientContainers()
{
    return this->clientContainers;
}

vector<GroupUser *>& BigTable::getGroups()
{
    return groups;
}

GroupUser* BigTable::AddUserInGroup(unsigned from, unsigned to, int content, unsigned userId)
{
    Ptr<Node> user = getClientContainers()->Get(userId);
    Ptr<Ipv4> ipv4src = user->GetObject<Ipv4>();

    string strIpv4Src = Ipv4AddressToString(ipv4src->GetAddress(1,0).GetLocal());
    string strIpv4Bst = Ipv4AddressToString(ipv4src->GetAddress(1,0).GetBroadcast());

    EndUser *new_user = new EndUser(user->GetId(), strIpv4Src, content);

    bool insertGroup = false;
    for (auto& group : groups) {
        string strGroupAddr = group->getId();

        if (strIpv4Bst == strGroupAddr && content == group->getContent()) {
            group->addUser(new_user);
            return group;
        }
    }

    if (!insertGroup) {
        string serverIpv4 = (*serverTable)[{strIpv4Bst, content}];

        GroupUser* group = new GroupUser(strIpv4Bst,
                                        serverIpv4,
                                        from,
                                        to,
                                        content,
                                        new_user);
        groups.push_back(group);
    }

    return groups[groups.size() - 1];
}

string BigTable::Ipv4AddressToString(Ipv4Address ad)
{
  ostringstream oss;
  ad.Print(oss);
  return oss.str();
}

}
