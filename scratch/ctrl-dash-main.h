#ifndef CTRL_DASH_MAIN_H_
#define CTRL_DASH_MAIN_H_

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"

#include "utils.h"

#include "group-user.h"


using namespace ns3;
using namespace std;



enum TypeOpt {RedirectGroup, Handover, AddContainer, RequestAccepted};

enum ServerState {On, Off, Waiting, StandBy};

namespace ns3 {

typedef struct {
	int actualNode;
	int nextNode;
	string ipv4;
} Interface;

class DashController : public Application
{
public:
  DashController ();
  virtual ~DashController ();

  static TypeId GetTypeId (void);

  void Setup (NetworkTopology* network, string str_ipv4_server, Address address, uint16_t port);

	void SetServerTableList(map<string, string> *serverTableList);
	string getServerTableList(string server);


	TypeOpt tryRequest (unsigned from, unsigned to, int content, int userId);

	void RunController ();

	bool hasToRedirect ();
	void DoSendRedirect ();

	void RedirectUsers(unsigned actualNode, unsigned nextNode);

	void CacheJoinAssignment(unsigned from, unsigned to);
	void onAddContainer(unsigned from, unsigned to, int content, int userId);

	GroupUser* AddUserInGroup (unsigned from, unsigned to, int content, unsigned userId);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

	void setSendRedirect (bool sendRedirect);
	vector<int> FindCommonGroups (unsigned actualNode, unsigned nextNode);
	vector<int> getFreeGroups(vector<int>& groups_i);

	void ResourceAllocation (vector<int> &free_groups);

	bool canAlloc (vector<int>& g_i, int actualNode, int nextNode);

	string getInterfaceNode (int actualNode, int nextNode);
	string getInterfaceNode(int node);


  bool ConnectionRequested (Ptr<Socket> socket, const Address& address);
  void ConnectionAccepted (Ptr<Socket> socket, const Address& address);

  void HandleIncomingData (Ptr<Socket> socket);
  void HandleReadyToTransmit (Ptr<Socket> socket, string &video);

  void ConnectionClosedNormal (Ptr<Socket> socket);
  void ConnectionClosedError (Ptr<Socket> socket);

	void ReDoSendRedirect();

	bool sendRedirect;

  Ptr<Socket>     m_socket;
  Address         m_listeningAddress;
  uint32_t        m_packetSize;
  uint32_t        m_packetsSent;
  uint16_t 	      m_port; //!< Port on which we listen for incoming packets.
  string 					m_serverIp;

  NetworkTopology *network;

	vector<GroupUser *> groups;

  vector<Interface> m_interfaces;
  vector<Interface> m_wireless_interfaces;
	vector<string> clientList;
	vector<string> cdn_srvs;

  map<string, Ptr<Socket>> m_clientSocket;

	GroupUser* m_group_i;

	map<string, Ptr<Node>>   m_server_node;
	map<string, ServerState> m_server_state;

	map<string, string> *serverTableList;
};

NS_OBJECT_ENSURE_REGISTERED (DashController);

DashController::DashController () :
              m_socket (0),
							m_listeningAddress (),
							m_packetSize (0),
              m_packetsSent (0)
{
}

DashController::~DashController ()
{
	m_socket = 0;
}

TypeId DashController::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::DashController")
		.SetParent<Application> ()
		.SetGroupName ("Applications")
		.AddConstructor<DashController> ()
    .AddAttribute ("ListeningAddress",
                   "The listening Address for the inbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&DashController::m_listeningAddress),
                   MakeAddressChecker ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets (default: 1317).",
                   UintegerValue (1317),
                   MakeUintegerAccessor (&DashController::m_port),
                   MakeUintegerChecker<uint16_t> ())
                   ;
	return tid;
}

void DashController::Setup(NetworkTopology* network, string str_ipv4_server, Address address, uint16_t port)
{
  this->cdn_srvs.push_back(str_ipv4_server);

  this->network 					 = network;
  this->m_listeningAddress = address;
	this->m_port 		 				 = port;
  this->m_serverIp 				 = str_ipv4_server;

  for (auto& link : network->getLinks()) {

  	Ptr<Node> nsrc = network->getNodeContainers()->Get(link->getSrcId());
  	Ptr<Ipv4> ipv4src = nsrc->GetObject<Ipv4>();

  	Ptr<Node> ndest = network->getNodeContainers()->Get(link->getDstId());
  	Ptr<Ipv4> ipv4dest = ndest->GetObject<Ipv4> ();

		int32_t int_prev = -1;
		for (uint32_t l = 1; l < ipv4src->GetNInterfaces (); l++) {
			for (uint32_t l1 = 1; l1 < ipv4dest->GetNInterfaces (); l1++) {
				int_prev = ipv4dest->GetInterfaceForPrefix (ipv4src->GetAddress(l, 0).GetLocal(), ipv4src->GetAddress(l, 0).GetMask());
				if (int_prev != -1) {
					break;
				}
			}

			string ipsrc = Ipv4AddressToString (ipv4src->GetAddress(l, 0).GetLocal());

			if (int_prev != -1) {
				m_interfaces.push_back({link->getSrcId(), link->getDstId(), ipsrc});
			}
		}
	}
	// Add wireless interface from ap nodes ===========================================
	for (unsigned int i = 0; i < network->getNodes().size(); i += 1) {
		Ptr<Node> node = network->getNodeContainers()->Get(network->getNodes().at(i)->getId());

		size_t found = Names::FindName(node).find("ap");
		if (found != string::npos) {
			Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
			string ipsrc = Ipv4AddressToString(ipv4->GetAddress(2, 0).GetLocal());

			m_interfaces.push_back({network->getNodes().at(i)->getId(), network->getNodes().at(i)->getId(), ipsrc});
		}
	}

	for (auto& interface : m_interfaces) {
		cout << "Uplink Interface -> (" << interface.actualNode << "," << interface.nextNode << ") -> " << interface.ipv4 << endl;
	}
}

void DashController::SetServerTableList(map<string, string> *serverTableList)
{
	this->serverTableList = serverTableList;
}

string DashController::getServerTableList(string server)
{
	return (*serverTableList)[server];
}

void DashController::RunController ()
{
	cout << "Starting offload cache. Size cdn new servers " << m_server_node.size() << endl;

	map<string, Ptr<Node>>::iterator it = m_server_node.begin();
	for (; it != m_server_node.end(); it++) {
		string str_new_server = it->first;
		Ptr<Node> new_server 	= it->second;

		if (m_server_state[str_new_server] != On) {
			m_server_state[str_new_server] = On;
		}
	}
}

bool DashController::hasToRedirect ()
{
	return this->sendRedirect;
}

void DashController::DoSendRedirect ()
{
	for (auto& group : groups) {
		for (auto& user : group->getUsers()) {
			cout << "[DashController] User Ip = "  << user->getIp() << " Server = " << group->getServerIp()
				 		<< " user size = " << group->getUsers().size() << endl;

			string serverIp = group->getServerIp();

			(*serverTableList)[user->getIp()] = group->getServerIp();
			if (m_clientSocket[user->getIp()] == 0) {
				clientList.push_back(user->getIp());
				continue;
			}

			HandleReadyToTransmit(m_clientSocket[user->getIp()], serverIp);
		}
	}

	setSendRedirect(false);
}

void DashController::setSendRedirect(bool sendRedirect)
{
	this->sendRedirect = sendRedirect;
}

TypeOpt DashController::tryRequest(unsigned from, unsigned to, int content, int userId)
{
	TypeOpt takeAction = RequestAccepted;
	m_group_i = DashController::AddUserInGroup(from, to, content, userId);

	int actualNode = m_group_i->getRoute().getActualStep();
	int nextNode   = m_group_i->getRoute().getNextStep();

	if (network->canAlloc(actualNode, nextNode)) {
		network->allocStream(actualNode, nextNode);
	} else {
		takeAction = AddContainer;
	}

	return takeAction;
}

vector<int> DashController::getFreeGroups(vector<int>& groups_i)
{
	vector<int> free_groups_index	(groups.size(), 0);

	for (auto& i : groups_i) {
		free_groups_index[i] = 1;
	}

	return free_groups_index;
}

void DashController::onAddContainer(unsigned from, unsigned to, int content, int userId)
{
	int actualNode = m_group_i->getRoute().getActualStep();
	int nextNode   = m_group_i->getRoute().getNextStep();
	network->ResetCapacityLink(actualNode, nextNode);

	vector<int> groups_i 					= DashController::FindCommonGroups(actualNode, nextNode);
	vector<int> free_groups_index = DashController::getFreeGroups(groups_i);

	DashController::ResourceAllocation(free_groups_index);
	DashController::setSendRedirect(true);
}

void DashController::RedirectUsers(unsigned actualNode, unsigned nextNode)
{
	vector<int> groups_i;

	for (unsigned i = 0; i < this->groups.size(); i++) {
		Path path = this->groups[i]->getRoute();
		path.goStart();
		while (!path.isEndPath()) {
			if (path.getActualStep() == actualNode && path.getNextStep() == nextNode) {
				groups_i.push_back(i);
			}
			path.goAhead();
		}
	}

	for (auto& g_i : groups_i) {
		network->SearchRoute(nextNode, groups[g_i]->getFrom());

		// std::cout << "groups(" << groups[g_i]->getId() << ") = " << network->getRoute() << '\n';
		// getchar();
		string newServerIp = getInterfaceNode(nextNode);

		groups[g_i]->setRoute(network->getRoute());
		groups[g_i]->setActualNode(actualNode);
		groups[g_i]->setServerIp(newServerIp);

		(*serverTableList)[groups[g_i]->getId()] = newServerIp;
	}
}

string DashController::getInterfaceNode(int node)
{
	Ptr<Node> nodesrc = network->getNodeContainers()->Get(node);
	Ptr<Ipv4> srcIpv4 = nodesrc->GetObject<Ipv4> ();

	return Ipv4AddressToString(srcIpv4->GetAddress(1, 0).GetLocal());
}

void DashController::ResourceAllocation (vector<int>& free_groups)
{
	for (size_t i = 0; i < free_groups.size(); i++) {
		if (!free_groups[i]) {
			continue;
		}

		int actualNode = groups[i]->getRoute().getActualStep();
		int nextNode   = groups[i]->getRoute().getNextStep();

		vector<int> groups_i = DashController::FindCommonGroups(actualNode, nextNode);

		if ((groups_i.size() > 0) & network->canAllocGroups(groups, groups_i, actualNode, nextNode)) {
			for (auto& g_i : groups_i) {
				free_groups[g_i] = 0;

				string new_serverIp = getInterfaceNode(actualNode, nextNode);
				network->SearchRoute(actualNode, groups[g_i]->getFrom());

				groups[g_i]->setRoute(network->getRoute());
				groups[g_i]->setActualNode(actualNode);
				groups[g_i]->setServerIp(new_serverIp);

				cout << "Resource allocated, ip=" << new_serverIp << endl;
				cout << groups[g_i]->getRoute() << endl;

				for (unsigned j = 0; j < groups[g_i]->getUsers().size(); j++) {
					network->allocStream(actualNode, nextNode);
				}
			}

			string str_ip  		 = groups[i]->getServerIp();
			ServerState state  = Off;
			Ptr<Node> new_node = network->getNodeContainers()->Get(actualNode);

			m_server_node.insert( pair<string, Ptr<Node>>(str_ip, new_node) );
			m_server_state.insert( pair<string, ServerState>(str_ip, state) );

		} else if (groups[i]->getRoute().isEndPath()) {
			Ptr<Node> node = network->getNodeContainers()->Get(actualNode);
			string new_serverIp = getInterfaceNode(actualNode, nextNode);
			network->SearchRoute(actualNode, groups[i]->getFrom());

			groups[i]->setRoute(network->getRoute());
			groups[i]->setActualNode(actualNode);
			groups[i]->setServerIp(new_serverIp);

			string str_ip  		 = groups[i]->getServerIp();
			ServerState state  = Off;
			Ptr<Node> new_node = network->getNodeContainers()->Get(actualNode);

			m_server_node.insert( pair<string, Ptr<Node>>(str_ip, new_node) );
			m_server_state.insert( pair<string, ServerState>(str_ip, state) );
		} else {
			groups[i]->getRoute().goAhead();
			DashController::ResourceAllocation(free_groups);
		}
	}
}

void DashController::CacheJoinAssignment(unsigned from, unsigned to)
{
}

vector<int> DashController::FindCommonGroups (unsigned actualNode, unsigned nextNode)
{
	vector<int> groups_i;

	for (unsigned i = 0; i < groups.size(); i++) {
		Path path = groups[i]->getRoute();
		path.goStart();
		while (!path.isEndPath()) {
			if (path.getActualStep() == actualNode && path.getNextStep() == nextNode) {
				groups_i.push_back(i);
			}
			path.goAhead();
		}
	}

	return groups_i;
}

GroupUser* DashController::AddUserInGroup(unsigned from, unsigned to, int content, unsigned userId)
{
	Ptr<Node> user = network->getClientContainers()->Get(userId);
	Ptr<Ipv4> ipv4src = user->GetObject<Ipv4>();

	string str_ipv4src = Ipv4AddressToString(ipv4src->GetAddress(1,0).GetLocal());
	string str_ipv4bst = Ipv4AddressToString(ipv4src->GetAddress(1,0).GetBroadcast());

	EndUser *new_user = new EndUser(user->GetId(), str_ipv4src, content);

	vector<string> str_addr = str_split(str_ipv4bst, ".");
	bool insert_group = false;
  for (auto& group : groups) {
    vector<string> group_addr = str_split(group->getId(), ".");

		bool same_group = true;
		for (unsigned i = 0; i < str_addr.size(); i++) {
			if (group_addr[i] != str_addr[i]) {
				same_group = false;
				break;
			}
		}

		if (same_group /*and cont == it->getContent()*/) {
    	group->addUser(new_user);
			(*serverTableList)[str_ipv4bst] = group->getServerIp();
			return group;
		}
  }

  if (!insert_group) {
		network->SearchRoute(to, from);
		groups.push_back(new GroupUser(str_ipv4bst, m_serverIp, from, to, network->getRoute(), new_user));
	}


	return groups[groups.size() - 1];
}

string DashController::getInterfaceNode(int actualNode, int nextNode)
{
	Ptr<Node> nodesrc = network->getNodeContainers()->Get(actualNode);
	Ptr<Ipv4> src_ip = nodesrc->GetObject<Ipv4> ();

	return Ipv4AddressToString(src_ip->GetAddress(1, 0).GetLocal());
}

void DashController::StartApplication (void)
{
  cout << "Starting Dash Controller" << endl;

  // m_running = true;
	// m_packetsSent = 0;
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

	m_socket->Listen ();

  NS_ASSERT (m_socket != 0);

  // And make sure to handle requests and accepted connections
  m_socket->SetAcceptCallback (MakeCallback(&DashController::ConnectionRequested, this),
                                  MakeCallback(&DashController::ConnectionAccepted, this));
}

void DashController::StopApplication (void)
{
}

bool DashController::ConnectionRequested (Ptr<Socket> socket, const Address& address)
{
  cout << Simulator::Now () << " Socket = " << socket << " Server: ConnectionRequested" << endl;
  return true;
}

void DashController::ConnectionAccepted (Ptr<Socket> socket, const Address& address)
{
  InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (address);

  cout << "DashController(" << socket << ") " << Simulator::Now ()
      << " Successful socket creation Connection Accepted From " << iaddr.GetIpv4 ()
      << " port: " << iaddr.GetPort ()<< endl;

  m_clientSocket[Ipv4AddressToString(iaddr.GetIpv4())] = socket;

  socket->SetRecvCallback (MakeCallback (&DashController::HandleIncomingData, this));

  socket->SetCloseCallbacks(MakeCallback (&DashController::ConnectionClosedNormal, this),
                            MakeCallback (&DashController::ConnectionClosedError,  this));
}

void DashController::HandleIncomingData(Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;

  packet = socket->RecvFrom (from);

  if (!(packet)) {
      return;
  }

  uint8_t *buffer = new uint8_t[packet->GetSize ()];
  packet->CopyData(buffer, packet->GetSize ());
  string str_s = string(buffer, buffer+packet->GetSize());

	std::ostringstream oss;
	socket->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal().Print(oss);
  cout << "DashController : HandleIncomingData - connection with DashController received " << str_s.c_str() << endl;

	for (auto& s : m_clientSocket) {
		Ptr<Socket> skt = s.second;
		if (skt == socket) {
			cout << "[DashController : HandleIncomingData] " << s.first << endl;
		}
	}

	// DashController::computeQoEGroup(qoe);
  // HandleReadyToTransmit(socket, str_s);
}

void DashController::HandleReadyToTransmit(Ptr<Socket> socket, string &requestString)
{
	cout << "DashController " << Simulator::Now () << " Socket HandleReadyToTransmit (DashController Class) From "
		 << socket << endl << "Server Ip = " << requestString << endl;
  uint8_t* buffer = (uint8_t*)requestString.c_str();
	Ptr<Packet> pkt = Create<Packet> (buffer, requestString.length());
  socket->Send (pkt);
}

void DashController::ConnectionClosedNormal (Ptr<Socket> socket)
{}

void DashController::ConnectionClosedError (Ptr<Socket> socket)
{}

}

#endif // CTRL_DASH_MAIN_H_
