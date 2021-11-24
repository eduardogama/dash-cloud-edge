#ifndef CTRL_DASH_MAIN_H_
#define CTRL_DASH_MAIN_H_

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"

#include "utils.h"
#include "videos.h"

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

	void SetServerTableList(map<pair<string, int>, string> *serverTableList);
	string getServerTableList(string server, int content);

	TypeOpt tryRequest (unsigned from, unsigned to, int content, int userId);

	void RunController ();

	bool hasToRedirect ();
	void DoSendRedirect ();

	void RedirectUsers(unsigned actualNode, unsigned nextNode);
	void RedirectUsersTwo(unsigned actualNode, unsigned nextNode);

	void RTMgmtMechanism(unsigned actualNode, unsigned nextNode);
	void ILPSolution(unsigned actualNode, unsigned nextNode);

	std::string GenRandom(const int len);

	void DoRedirectUsers(unsigned i, unsigned nextNode, int content);

	void CacheJoinAssignment(unsigned from, unsigned to);
	void onAddContainer(unsigned from, unsigned to, int content, int userId);

	GroupUser* AddUserInGroup (unsigned from, unsigned to, int content, unsigned userId);
	void printGroups();

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

	map<pair<string, int>, string> *serverTableList;
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

void DashController::SetServerTableList(map<pair<string, int>, string> *serverTableList)
{
	this->serverTableList = serverTableList;
}

string DashController::getServerTableList(string server, int content)
{
	return (*serverTableList)[{server, content}];
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

			(*serverTableList)[{user->getIp(), user->getContent()}] = group->getServerIp();
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

	// int actualNode = m_group_i->getRoute().getActualStep();
	// int nextNode   = m_group_i->getRoute().getNextStep();
	//
	// if (network->canAlloc(actualNode, nextNode)) {
	// 	network->allocStream(actualNode, nextNode);
	// } else {
	// 	takeAction = AddContainer;
	// }

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

	network->SearchRoute(to, from); // From Dst (7) to Ap (3,4,5,6)

  if (!insertGroup) {
		string serverIpv4 = (*serverTableList)[{strIpv4Bst, content}];
		groups.push_back(new GroupUser(strIpv4Bst, serverIpv4, from, to, network->getRoute(), content, new_user));
	}

	return groups[groups.size() - 1];
}

void DashController::RedirectUsersTwo(unsigned actualNode, unsigned nextNode)
{
	for (unsigned i = 0; i < this->groups.size(); i++) {
		Path path   = this->groups[i]->getRoute();
		int content = this->groups[i]->getContent();

		unsigned actual = actualNode;
		unsigned next   = nextNode;

		bool findedLink = false;
		path.goStart();
		while (!path.isEndPath()) {
			if (path.getActualStep() == actual && path.getNextStep() == next) {
				findedLink = true;
				break;
			}
			path.goAhead();
		}

		if (findedLink) {
			path.goLastLink();

			actual = path.getActualStep();
			next   = path.getNextStep();

			while (!path.isStartPath() && (next != actualNode)) {

				if (hasVideoByNode(next, content)) {
					DoRedirectUsers(i, next, content);
					break;
				}
				if (VideoAssignment(next, content)) {
					string strContent = getVideoPath(content);

					Ptr<Application> app = network->getNodeContainers()->Get(next)->GetApplication(0);
			    app->GetObject<EdgeDashFakeServerApplication>()->AddVideo(strContent);

					DoRedirectUsers(i, next, content);
					break;
				}

				path.goBack();
				actual = path.getActualStep();
				next   = path.getNextStep();
			}
		}
	}

	printGroups();
}

std::string DashController::GenRandom(const int len) {
	static const char alphanum[] =
	    "0123456789"
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz";
	std::string tmp_s;
	tmp_s.reserve(len);

	for (int i = 0; i < len; ++i) {
  	tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	return tmp_s;
}

void DashController::ILPSolution(unsigned actualNode, unsigned nextNode)
{
	std::ostringstream buffer;

	string output = GenRandom(6) ;
	buffer << output << " ";
	buffer << this->groups.size() << " ";
	buffer << actualNode << " " << nextNode << " ";
	for (unsigned i = 0; i < this->groups.size(); i++) {
		buffer << this->groups[i]->getAp() << " "
					 << i + 8 << " "
					 << i << " "
					 << this->groups[i]->getUsers().size() << " ";
	}

	std:string cmd("python3.7 ../ILP-QoE/cflp-main.py " + buffer.str());
	system(cmd.c_str());

	string line;
	ifstream ilpSolution(output);

	while (getline (ilpSolution, line)) {
		std::cout << line << '\n';
		vector<string> vals = str_split(line, " ");

		int groupId = ::stoi(vals[0]);
		int serverId = ::stoi(vals[1]);
		// cout << vals[0] << ' ' << vals[1] << '\n';

		int content = this->groups[groupId]->getContent();

		if (hasVideoByNode(serverId, content)) {
			DoRedirectUsers(groupId, serverId, content);
		} else if (VideoAssignment(serverId, content)) {
			string strContent = getVideoPath(content);

			Ptr<Application> app = network->getNodeContainers()->Get(serverId)->GetApplication(0);
			app->GetObject<EdgeDashFakeServerApplication>()->AddVideo(strContent);

			DoRedirectUsers(groupId, serverId, content);
		}
	}
	ilpSolution.close();

	cmd = "rm " + output;
	system(cmd.c_str());

}

void DashController::RTMgmtMechanism(unsigned actualNode, unsigned nextNode)
{
	for (unsigned i = 0; i < this->groups.size(); i++) {
		Path path   = this->groups[i]->getRoute();
		int content = this->groups[i]->getContent();

		unsigned actual = actualNode;
		unsigned next   = nextNode;

		bool findedLink = false;
		path.goStart();
		while (!path.isEndPath()) {
			if (path.getActualStep() == actual && path.getNextStep() == next) {
				findedLink = true;
				break;
			}
			path.goAhead();
		}

		if (findedLink) {

		}
	}
	printGroups();
}

void DashController::RedirectUsers(unsigned actualNode, unsigned nextNode)
{
	for (unsigned i = 0; i < this->groups.size(); i++) {
		Path path = this->groups[i]->getRoute();

		unsigned actual = actualNode;
		unsigned next   = nextNode;

		path.goStart();
		while (!path.isEndPath()) {
			if (path.getActualStep() == actual && path.getNextStep() == next) {

				int content = this->groups[i]->getContent();
				// std::cout << content << '\n';
				// std::cout << "has Content " << content << " in Node group(" << groups[i]->getId() << "," << groups[i]->getContent() << ")=" << hasVideoByNode(next, content) << '\n';

				if (hasVideoByNode(next, content)){
					DoRedirectUsers(i, next, content);
					// getchar();
					break;
				} else {
					path.goAhead();
					actual = path.getActualStep();
					next   = path.getNextStep();
					continue;
				}
			}
			path.goAhead();
		}
	}

	printGroups();
}

void DashController::DoRedirectUsers(unsigned i, unsigned nextNode, int content)
{
	network->SearchRoute(nextNode, groups[i]->getFrom());

	std::cout << "group(" << groups[i]->getId() << "," << groups[i]->getContent() << ") = Old " << groups[i]->getRoute() << '\n';
	std::cout << "group(" << groups[i]->getId() << "," << groups[i]->getContent() << ") = New " << network->getRoute() << '\n';
	string newServerIp = getInterfaceNode(nextNode);

	groups[i]->setRoute(network->getRoute());
	groups[i]->setActualNode(nextNode);
	groups[i]->setServerIp(newServerIp);

	(*serverTableList)[{groups[i]->getId(), groups[i]->getContent()}] = newServerIp;
}

// void DashController::RedirectUsers(unsigned actualNode, unsigned nextNode)
// {
// 	vector<int> groups_i;
// 	for (unsigned i = 0; i < this->groups.size(); i++) {
// 		Path path = this->groups[i]->getRoute();
// 		path.goStart();
// 		while (!path.isEndPath()) {
// 			if (path.getActualStep() == actualNode && path.getNextStep() == nextNode) {
// 				groups_i.push_back(i);
// 				break;
// 			}
// 			path.goAhead();
// 		}
// 	}
//
// 	for (auto& g_i : groups_i) {
// 		network->SearchRoute(nextNode, groups[g_i]->getFrom());
//
// 		std::cout << "groups(" << groups[g_i]->getId() << ") = Old" << groups[g_i]->getRoute() << '\n';
// 		string newServerIp = getInterfaceNode(nextNode);
//
// 		groups[g_i]->setRoute(network->getRoute());
// 		groups[g_i]->setActualNode(nextNode);
// 		groups[g_i]->setServerIp(newServerIp);
//
// 		(*serverTableList)[groups[g_i]->getId()] = newServerIp;
// 	}
//
// 	printGroups();
// }

string DashController::getInterfaceNode(int node)
{
	Ptr<Node> nodesrc = network->getNodeContainers()->Get(node);
	Ptr<Ipv4> srcIpv4 = nodesrc->GetObject<Ipv4> ();

	return Ipv4AddressToString(srcIpv4->GetAddress(1, 0).GetLocal());
}

string DashController::getInterfaceNode(int actualNode, int nextNode)
{
	Ptr<Node> nodesrc = network->getNodeContainers()->Get(actualNode);
	Ptr<Ipv4> src_ip = nodesrc->GetObject<Ipv4> ();

	return Ipv4AddressToString(src_ip->GetAddress(1, 0).GetLocal());
}

void DashController::printGroups()
{
	for (size_t i = 0; i < this->groups.size(); i++) {
		std::cout << "Id=" << this->groups[i]->getId() << " Server=" << this->groups[i]->getServerIp();
		std::cout << " Content=" << this->groups[i]->getContent();
		std::cout << " " << this->groups[i]->getRoute() << '\n';
	}
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
