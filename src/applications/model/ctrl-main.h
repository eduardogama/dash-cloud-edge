#ifndef CONTROLLER_MAIN_H
#define CONTROLLER_MAIN_H


#include <functional>

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"

#include "ns3/node-container.h"
#include "ns3/internet-module.h"

#include "bigtable.h"


using namespace std;

namespace ns3 {

class ControllerMain : public Application
{
public:
    static TypeId GetTypeId();

    ControllerMain();
    virtual ~ControllerMain();

    void Setup(Address address, uint16_t port, string optAlgo);

    bool OptimizerComponent(int actualNode, int nextNode);
    bool ILPSolution(int actualNode, int nextNode);
    bool QoSGreedy(int actualNode, int nextNode);

    void setBigTable(Ptr<BigTable> bigtable);

    void setNodeContainers(NodeContainer *nodeContainers);
    NodeContainer *getNodeContainers(void);

    void setServerTable(map<pair<string, int>, string> *serverTable);
    string getServerTable(string server, int content);

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    bool ConnectionRequested(Ptr<Socket> socket, const Address& address);
    void ConnectionAccepted(Ptr<Socket> socket, const Address& address);

    void HandleIncomingData(Ptr<Socket> socket);

    void ConnectionClosedNormal(Ptr<Socket> socket);
    void ConnectionClosedError(Ptr<Socket> socket);

    string Ipv4AddressToString(Ipv4Address ad);

    string getInterfaceNode(int node);
    void DoRedirectUsers(unsigned i, unsigned nextNode, int content);

private:
    Address     m_listeningAddress;
    uint16_t    m_port; //!< Port on which we listen for incoming packets.
    Ptr<Socket> m_socket;

    Ptr<BigTable> bigtable;

    map<string, Ptr<Socket>> m_clientSocket;

    NodeContainer *nodeContainers;

    map<pair<string, int>, string> *serverTable;

    function<bool(int, int)> optimizerSol;
    // auto& optimizerSol;
};

}

#endif // CONTROLLER_MAIN_H
