#include <fstream>

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

#include "dash-fake-cache-server.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("DASHFakeCacheServer");

NS_OBJECT_ENSURE_REGISTERED (DASHFakeCacheServer);


TypeId DASHFakeCacheServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DASHFakeCacheServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<DASHFakeCacheServer> ()
    .AddAttribute ("ListeningAddress",
                   "The listening Address for the inbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&DASHFakeCacheServer::m_listeningAddress),
                   MakeAddressChecker ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets (default: 80).",
                   UintegerValue (80),
                   MakeUintegerAccessor (&DASHFakeCacheServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute("MPDDirectory", "The directory which DASHFakeCacheServer fakes to serves the MPD files",
                   StringValue("/"),
                   MakeStringAccessor(&DASHFakeCacheServer::m_mpdDirectory),
                   MakeStringChecker())
    .AddAttribute("RepresentationsMetaDataFiles", "The meta data csv file(s) containing the videos and representations this server will serve",
                   StringValue("./representations.csv"),
                   MakeStringAccessor(&DASHFakeCacheServer::m_mpdMetaDataFiles),
                   MakeStringChecker())
    .AddAttribute("RepresentationsSegmentsDirectory", "The directory that virtual segments of representations are served",
                   StringValue("/"),
                   MakeStringAccessor(&DASHFakeCacheServer::m_metaDataContentDirectory),
                   MakeStringChecker())
    .AddAttribute("Hostname", "The (virtual) hostname of this server",
                   StringValue("localhost"),
                   MakeStringAccessor(&DASHFakeCacheServer::m_hostName),
                   MakeStringChecker())
    .AddAttribute("RemoteCDN", "The (virtual) RemoteCDN of this server",
                   StringValue("localhost"),
                   MakeStringAccessor(&DASHFakeCacheServer::m_remoteCDN),
                   MakeStringChecker())
                    ;
  ;
  return tid;
}

DASHFakeCacheServer::DASHFakeCacheServer ()
{
  NS_LOG_FUNCTION (this);
}

DASHFakeCacheServer::~DASHFakeCacheServer()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}

// this traces the acutal packet size, including header etc...
void DASHFakeCacheServer::TxTrace(Ptr<Packet const> packet)
{
  m_bytes_sent += packet->GetSize();
}

// this traces the acutal packet size, including header etc...

void DASHFakeCacheServer::RxTrace(Ptr<Packet const> packet)
{
  m_bytes_recv += packet->GetSize();
}

void DASHFakeCacheServer::StartApplication (void)
{
  NS_LOG_FUNCTION(this);

  Ptr<NetDevice> netdevice = GetNode()->GetDevice(0);
  netdevice->TraceConnectWithoutContext("PhyTxEnd", MakeCallback(&DASHFakeCacheServer::TxTrace, this));
  netdevice->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&DASHFakeCacheServer::RxTrace, this));

  m_bytes_recv = 0;
  m_bytes_sent = 0;

  m_last_bytes_recv = 0;
  m_last_bytes_sent = 0;

  if (m_socket == 0)
  {
    TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
    m_socket = Socket::CreateSocket (GetNode (), tid);

    // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
    if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM
        && m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
    {
      NS_FATAL_ERROR ("Using BulkSend with an incompatible socket type. "
                      "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                      "In other words, use TCP instead of UDP.");
    }

    if (Ipv4Address::IsMatchingType(m_listeningAddress) == true) {
      InetSocketAddress local = InetSocketAddress (Ipv4Address::ConvertFrom(m_listeningAddress), m_port);
      NS_LOG_INFO("Listening on Ipv4 " << Ipv4Address::ConvertFrom(m_listeningAddress) << ":" << m_port);
      m_socket->Bind (local);
    } else if (Ipv6Address::IsMatchingType(m_listeningAddress) == true) {
      Inet6SocketAddress local6 = Inet6SocketAddress (Ipv6Address::ConvertFrom(m_listeningAddress), m_port);
      NS_LOG_INFO("Listening on Ipv6 " << Ipv6Address::ConvertFrom(m_listeningAddress));
      m_socket->Bind (local6);
    } else {
      NS_LOG_ERROR("Not sure what type the m_listeningaddress is... " << m_listeningAddress);
    }
  }

  m_socket->Listen();
  NS_ASSERT(m_socket != 0);

  // And make sure to handle requests and accepted connections
  m_socket->SetAcceptCallback (MakeCallback(&DASHFakeCacheServer::ConnectionRequested, this),
      MakeCallback(&DASHFakeCacheServer::ConnectionAccepted, this)
  );

  this->mpdFile = zlib_compress_string(ImportDASHRepresentations());
  this->m_fileSizes["content/mpds/vid1.mpd.gz"] = this->mpdFile.size();
}

void DASHFakeCacheServer::StopApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) {
    m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }
}

bool DASHFakeCacheServer::ConnectionRequested (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  NS_LOG_DEBUG (Simulator::Now () << " Socket = " << socket << " " << " Server: ConnectionRequested");
  
  return true;
}

void DASHFakeCacheServer::ConnectionAccepted (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  fprintf(stderr, "DASH Fake Cache Server: Connection Accepted!\n");
  uint64_t socket_id = RegisterSocket(socket);

  m_activeClients[socket_id] = new DashFakeVirtualClientSocket(m_fileSizes,
                  mpdFile, socket_id, socket, GetNode(), m_remoteCDN,
                  MakeCallback(&DASHFakeCacheServer::FinishedCallback, this));

  // set callbacks for this socket to be in HttpServerFakeClientSocket class
  // socket->SetSendCallback(MakeCallback(&DashFakeVirtualClientSocket::HandleReadyToTransmit, m_activeClients[socket_id]));
  socket->SetRecvCallback(MakeCallback(&DashFakeVirtualClientSocket::IncomingDataFromUser, m_activeClients[socket_id]));

  NS_LOG_DEBUG (socket << " " << Simulator::Now () << " Successful socket id : " << socket_id << " Connection Accepted From " << address);
}

uint64_t DASHFakeCacheServer::RegisterSocket (Ptr<Socket> socket)
{
  this->m_activeSockets[socket] = this->m_lastSocketID;
  return this->m_lastSocketID++;
}

void DASHFakeCacheServer::FinishedCallback(uint64_t socket_id)
{
  // create timer to finish this, because if we do it in here, we will crash the app
  // Simulator::Schedule(Seconds(1.0), &DASHFakeServerApplication::DoFinishSocket, this, socket_id);
  std::cout << "CacheServer running FinishedCallback function" << '\n';
}

std::string DASHFakeCacheServer::ImportDASHRepresentations()
{
  std::ifstream infile("video.mpd");

  if (!infile.is_open()) {
    std::cout << "Error Opening video.mpd" << '\n';
    return "";
  }

  std::stringstream mpdData;
  std::string line;

  while (std::getline(infile,line))
    mpdData << line << '\n';
  infile.close();

  return mpdData.str();
}

}
