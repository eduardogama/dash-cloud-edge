#include "aggregation.h"

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/string.h"
#include "ns3/pointer.h"


#include <stdio.h>
#include <stdlib.h>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AggregationApplication");

NS_OBJECT_ENSURE_REGISTERED (AggregationApplication);

//AggregationApplication::AggregationApplication(uint64_t socket_id,
//    std::string contentDir,
//    std::map<std::string /* filename */, long /* file size */>& fileSizes,
//    std::vector<std::string /* filename */>& fakeFiles,
//    std::map<std::string,std::string>& /* virtual file host */ virtualHostedFiles,
//    Callback<void, uint64_t> finished_callback)
//{

TypeId
AggregationApplication::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::AggregationApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<AggregationApplication> ()
	.AddAttribute ("ListeningAddress",
                   "The listening Address for the inbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&AggregationApplication::m_listeningAddress),
                   MakeAddressChecker ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets (default: 80).",
                   UintegerValue (80),
                   MakeUintegerAccessor (&AggregationApplication::m_port),
                   MakeUintegerChecker<uint16_t> ())
	.AddAttribute("Hostname", "The (virtual) hostname of this server",
                   StringValue("localhost"),
                   MakeStringAccessor(&AggregationApplication::m_hostName),
                   MakeStringChecker())
                   ;
    return tid;
}

AggregationApplication::AggregationApplication() : alive(false)
{
	NS_LOG_FUNCTION (this);
}

AggregationApplication::~AggregationApplication()
{
	//	NS_LOG_FUNCTION (this);
}

// this traces the acutal packet size, including header etc...
void AggregationApplication::TxTrace(Ptr<Packet const> packet)
{
    m_bytes_sent += packet->GetSize();
}

// this traces the acutal packet size, including header etc...
void AggregationApplication::RxTrace(Ptr<Packet const> packet)
{
    m_bytes_recv += packet->GetSize();
}

void AggregationApplication::StartApplication (void)
{
	NS_LOG_FUNCTION (this);
	
    // trace Physical TX and RX after it has been done (End)
    Ptr<NetDevice> netdevice = GetNode()->GetDevice(0);
    netdevice->TraceConnectWithoutContext("PhyTxEnd", MakeCallback(&AggregationApplication::TxTrace, this));
    netdevice->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&AggregationApplication::RxTrace, this));
    
    if(AggregationApplication::isAlive())
	{
		return;
	}
	
    if (m_socket == 0)
    {
        TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");

        m_socket = Socket::CreateSocket(GetNode(), tid);

        // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
        if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
        m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
        {
            fprintf(stderr,"Using BulkSend with an incompatible socket type. "
                          "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                          "In other words, use TCP instead of UDP.\n");
        }

        if (Ipv4Address::IsMatchingType(m_listeningAddress) == true)
        {
            InetSocketAddress local = InetSocketAddress (Ipv4Address::ConvertFrom(m_listeningAddress), m_port);
            std::cout << "Listening on Ipv4 " << Ipv4Address::ConvertFrom(m_listeningAddress) << ":" << m_port << std::endl;
            m_socket->Bind (local);
        } else if (Ipv6Address::IsMatchingType(m_listeningAddress) == true)
        {
            Inet6SocketAddress local6 = Inet6SocketAddress (Ipv6Address::ConvertFrom(m_listeningAddress), m_port);
            std::cout << "Listening on Ipv6 " << Ipv6Address::ConvertFrom(m_listeningAddress) << std::endl;
            m_socket->Bind (local6);
        } else {
            std::cout << "Not sure what type the m_listeningaddress is... " << m_listeningAddress << std::endl;
        }
    }

    // Listen for incoming connections
    m_socket->Listen();
    NS_ASSERT (m_socket != 0);

    // And make sure to handle requests and accepted connections
    m_socket->SetAcceptCallback (MakeCallback(&AggregationApplication::ConnectionRequested, this),
        MakeCallback(&AggregationApplication::ConnectionAccepted, this)
    );
	
	fprintf(stderr, "GTA Dash AggregationApplication Created!\n");
}

void AggregationApplication::StopApplication()
{
    NS_LOG_FUNCTION (this);

//    m_active = false;

//    Simulator::Cancel(m_reportStatsTimer);

    if (m_socket != 0)
    {
        m_socket->Close ();
        m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void AggregationApplication::Start()
{	
	this->alive = true;
}

void AggregationApplication::AddUser(uint64_t user)
{

}

bool AggregationApplication::isAlive()
{
	return this->alive;
}

bool AggregationApplication::ConnectionRequested (Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION (this << socket << address);
    NS_LOG_DEBUG (Simulator::Now () << " Socket = " << socket << " " << " Server: ConnectionRequested");
 
    return true;
}

void AggregationApplication::ConnectionAccepted (Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION (this << socket << address);

	uint64_t socket_id = RegisterSocket(socket);

	AggregationApplication::AddUser(socket_id);

    NS_LOG_DEBUG (socket << " " << Simulator::Now () << " Successful socket creation (AggregationApplication Class) Connection Accepted From " << address);

    // set callbacks for this socket to be in HttpServerFakeClientSocket class
    socket->SetSendCallback (MakeCallback (&AggregationApplication::HandleReadyToTransmit, this));
    socket->SetRecvCallback (MakeCallback (&AggregationApplication::HandleIncomingData, this));

//    socket->TraceConnectWithoutContext ("State", MakeCallback(&AggregationApplication::LogStateChange, m_activeClients[socket_id]));

    /*
    socket->TraceConnectWithoutContext ("CongestionWindow",
    MakeCallback(&HttpServerFakeClientSocket::LogCwndChange, m_activeClients[socket_id]));
    */

    socket->SetCloseCallbacks (MakeCallback (&AggregationApplication::ConnectionClosedNormal, this),
                             MakeCallback (&AggregationApplication::ConnectionClosedError,  this));
}

void AggregationApplication::HandleIncomingData(Ptr<Socket> socket)
{
}

void AggregationApplication::HandleReadyToTransmit(Ptr<Socket> socket, uint32_t txSize)
{

}

void AggregationApplication::ConnectionClosedNormal (Ptr<Socket> socket)
{

}

void AggregationApplication::ConnectionClosedError (Ptr<Socket> socket)
{

}

uint64_t AggregationApplication::RegisterSocket(Ptr<Socket> socket)
{
	this->m_activeSockets[socket] = this->m_lastSocketID;
	
	return this->m_lastSocketID++;
}

Cluster::Cluster()
{

}

Cluster::~Cluster()
{

}

}
