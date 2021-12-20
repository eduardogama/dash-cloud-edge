#include "ctrl-main.h"

#include "ns3/log.h"
#include "ns3/string.h"

#include <iostream>

#include "edge-dash-fake-server.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ControllerMain");

NS_OBJECT_ENSURE_REGISTERED(ControllerMain);


TypeId ControllerMain::GetTypeId()
{
  static TypeId tid = TypeId("ns3::ControllerMain")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<ControllerMain> ()
    .AddAttribute ("ListeningAddress",
                   "The listening Address for the inbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&ControllerMain::m_listeningAddress),
                   MakeAddressChecker ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets (default: 1317).",
                   UintegerValue (1317),
                   MakeUintegerAccessor (&ControllerMain::m_port),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

ControllerMain::ControllerMain()
{
  NS_LOG_FUNCTION (this);
}

ControllerMain::~ControllerMain()
{
  NS_LOG_FUNCTION (this);
}

void ControllerMain::Setup(Address address, uint16_t port)
{
    this->m_listeningAddress = address;
    this->m_port = port;
}

bool ControllerMain::OptimizerComponent(int actualNode, int nextNode)
{
    return ILPSolution(actualNode, nextNode);
}

bool ControllerMain::ILPSolution(int actualNode, int nextNode)
{
    ostringstream buffer;
    vector<GroupUser *> groups = bigtable->getGroups();

    buffer << "python3.7 ../ILP-QoE/cflp-2-main.py ";
    buffer << groups.size() << " ";
	buffer << actualNode << " " << nextNode << " ";
    for (unsigned i = 0; i < groups.size(); i++) {
		buffer << groups[i]->getAp() << " "
            << i + 8 << " "
            << i << " "
            << groups[i]->getUsers().size() << " ";
    }

    string output = GenRandom(6);
    buffer <<  "> " << output;

    string cmd(buffer.str());
	system(cmd.c_str());

    string line;
    ifstream ilpSolution(output);

    while (getline (ilpSolution, line)) {
        std::cout << line << '\n';
        vector<string> vals = Split(line, " ");

        int groupId = ::stoi(vals[0]);
        int serverId = ::stoi(vals[1]);

        int content = groups[groupId]->getContent();

        Ptr<Application> app = getNodeContainers()->Get(serverId)->GetApplication(0);
        Ptr<EdgeDashFakeServerApplication> edgenode = app->GetObject<EdgeDashFakeServerApplication>();

        if (edgenode->hasVideo(content)) {
            DoRedirectUsers(groupId, serverId, content);
        } else if (edgenode->VideoAssignment(content)) {
            string strContent = edgenode->getVideoPath(content);

            edgenode->AddVideo(strContent);

            DoRedirectUsers(groupId, serverId, content);
        }
    }
    ilpSolution.close();

	cmd = "rm " + output;
	system(cmd.c_str());

    return true;
}
//5 0 1	3 8 0 5	4 9 1 1	5 10 2 1 3 11 3 2	6 12 4 3
void ControllerMain::DoRedirectUsers(unsigned i, unsigned nextNode, int content)
{
    vector<GroupUser *> groups = bigtable->getGroups();

    cout << "group(" << groups[i]->getId() << "," << groups[i]->getContent() << '\n';
    cout << "group(" << groups[i]->getId() << "," << groups[i]->getContent() << '\n';
    string newServerIp = getInterfaceNode(nextNode);

    groups[i]->setActualNode(nextNode);
    groups[i]->setServerIp(newServerIp);

    (*serverTable)[{groups[i]->getId(), groups[i]->getContent()}] = newServerIp;
}

string ControllerMain::getInterfaceNode(int node)
{
    Ptr<Node> nodesrc = getNodeContainers()->Get(node);
    Ptr<Ipv4> srcIpv4 = nodesrc->GetObject<Ipv4> ();

    return Ipv4AddressToString(srcIpv4->GetAddress(1, 0).GetLocal());
}

void ControllerMain::setServerTable(map<pair<string, int>, string> *serverTable)
{
    this->serverTable = serverTable;
}

string ControllerMain::getServerTable(string server, int content)
{
    return (*serverTable)[{server, content}];
}

void ControllerMain::setBigTable(Ptr<BigTable> bigtable)
{
    this->bigtable = bigtable;
}

void ControllerMain::setNodeContainers(NodeContainer *nodeContainers)
{
    this->nodeContainers = nodeContainers;
}

NodeContainer *ControllerMain::getNodeContainers(void)
{
    return this->nodeContainers;
}

void ControllerMain::StartApplication()
{
    cout << "Starting Dash Controller" << endl;

    if (m_socket == 0) {
        TypeId tid = TypeId::LookupByName("ns3::TcpSocketFactory");

        m_socket = Socket::CreateSocket(GetNode(), tid);

        // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
        if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
            m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
        {
            fprintf (stderr,"Using BulkSend with an incompatible socket type. "
                            "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                            "In other words, use TCP instead of UDP.\n");
        }

        if (Ipv4Address::IsMatchingType (m_listeningAddress) == true)
        {
            InetSocketAddress local = InetSocketAddress (Ipv4Address::ConvertFrom (m_listeningAddress), m_port);
            cout << "Listening on Ipv4 " << Ipv4Address::ConvertFrom (m_listeningAddress) << ":" << m_port << endl;
            m_socket->Bind (local);
        } else if (Ipv6Address::IsMatchingType (m_listeningAddress) == true)
        {
            Inet6SocketAddress local6 = Inet6SocketAddress (Ipv6Address::ConvertFrom (m_listeningAddress), m_port);
            cout << "Listening on Ipv6 " << Ipv6Address::ConvertFrom (m_listeningAddress) << endl;
            m_socket->Bind (local6);
        } else {
            cout << "Not sure what type the m_listeningaddress is... " << m_listeningAddress << endl;
        }
    }

    m_socket->Listen();

    NS_ASSERT(m_socket != 0);

    // And make sure to handle requests and accepted connections
    m_socket->SetAcceptCallback(MakeCallback(&ControllerMain::ConnectionRequested, this),
        MakeCallback(&ControllerMain::ConnectionAccepted, this));
}

void ControllerMain::StopApplication()
{
    NS_LOG_FUNCTION (this);
}

bool ControllerMain::ConnectionRequested (Ptr<Socket> socket, const Address& address)
{
    cout << Simulator::Now () << " Socket = " << socket << " Server: ConnectionRequested" << endl;
    return true;
}

void ControllerMain::ConnectionAccepted (Ptr<Socket> socket, const Address& address)
{
    InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (address);

    cout << "ControllerMain(" << socket << ") " << Simulator::Now ()
        << " Successful socket creation Connection Accepted From " << iaddr.GetIpv4 ()
        << " port: " << iaddr.GetPort ()<< endl;

    m_clientSocket[Ipv4AddressToString(iaddr.GetIpv4())] = socket;

    socket->SetRecvCallback (MakeCallback (&ControllerMain::HandleIncomingData, this));

    socket->SetCloseCallbacks(MakeCallback (&ControllerMain::ConnectionClosedNormal, this),
        MakeCallback (&ControllerMain::ConnectionClosedError,  this));
}

void ControllerMain::HandleIncomingData(Ptr<Socket> socket)
{

}

string ControllerMain::Ipv4AddressToString (Ipv4Address ad)
{
    ostringstream oss;
    ad.Print(oss);
    return oss.str();
}

string ControllerMain::GenRandom(const int len)
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}

void ControllerMain::ConnectionClosedNormal (Ptr<Socket> socket)
{}

void ControllerMain::ConnectionClosedError (Ptr<Socket> socket)
{}


}
