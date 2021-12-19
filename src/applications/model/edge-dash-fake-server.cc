/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2015 Christian Kreuzberger, Alpen-Adria-Universitaet Klagenfurt
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Christian Kreuzberger <christian.kreuzberger@itec.aau.at>
//

// ns3 - HTTP Server Application class

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

#include "edge-dash-fake-server.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EdgeDashFakeServerApplication");

NS_OBJECT_ENSURE_REGISTERED (EdgeDashFakeServerApplication);




TypeId
EdgeDashFakeServerApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EdgeDashFakeServerApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<EdgeDashFakeServerApplication> ()
    .AddAttribute ("ListeningAddress",
                   "The listening Address for the inbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&EdgeDashFakeServerApplication::m_listeningAddress),
                   MakeAddressChecker ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets (default: 80).",
                   UintegerValue (80),
                   MakeUintegerAccessor (&EdgeDashFakeServerApplication::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute("MPDDirectory", "The directory which EdgeDashFakeServerApplication fakes to serves the MPD files",
                   StringValue("/"),
                   MakeStringAccessor(&EdgeDashFakeServerApplication::m_mpdDirectory),
                   MakeStringChecker())
    .AddAttribute("RepresentationsMetaDataFiles", "The meta data csv file(s) containing the videos and representations this server will serve",
                   StringValue("./representations.csv"),
                   MakeStringAccessor(&EdgeDashFakeServerApplication::m_mpdMetaDataFiles),
                   MakeStringChecker())
    .AddAttribute("RepresentationsSegmentsDirectory", "The directory that virtual segments of representations are served",
                   StringValue("/"),
                   MakeStringAccessor(&EdgeDashFakeServerApplication::m_metaDataContentDirectory),
                   MakeStringChecker())
    .AddAttribute("Hostname", "The (virtual) hostname of this server",
                   StringValue("localhost"),
                   MakeStringAccessor(&EdgeDashFakeServerApplication::m_hostName),
                   MakeStringChecker())
    .AddAttribute ("Capacity", "Capacity Server.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&EdgeDashFakeServerApplication::m_capacity),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("StartCapacity", "Start Capacity Server.",
                  UintegerValue (0),
                  MakeUintegerAccessor (&EdgeDashFakeServerApplication::assignedVideos),
                  MakeUintegerChecker<uint16_t> ())
    .AddTraceSource("ThroughputTracer", "Trace Throughput statistics of this server",
                      MakeTraceSourceAccessor(&EdgeDashFakeServerApplication::m_throughputTrace), "bla")
                    ;
  ;
  return tid;
}


EdgeDashFakeServerApplication::EdgeDashFakeServerApplication ()
{
  NS_LOG_FUNCTION (this);
}

EdgeDashFakeServerApplication::~EdgeDashFakeServerApplication()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}

void
EdgeDashFakeServerApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

std::string EdgeDashFakeServerApplication::ImportDASHRepresentations (std::string mpdMetaDataFilename, int video_id) /* mpd string */
{
  NS_LOG_FUNCTION(mpdMetaDataFilename << video_id);
// read m_mpdMetaDataFiles and fill m_fileSizes
  std::ifstream infile(mpdMetaDataFilename.c_str());
  if (!infile.is_open())
  {
    NS_LOG_ERROR("Error opening " << mpdMetaDataFilename);
    return "";
  }

  int segment_duration = 0;
  int number_of_segments = 0;

  std::string line;

  // get first line: segmentDuration
  std::getline(infile,line);
  std::string prefix("segmentDuration=");
  if (!line.compare(0, prefix.size(), prefix))
    segment_duration = atoi(line.substr(prefix.size()).c_str());

  std::getline(infile,line);
  prefix = "numberOfSegments=";
  if (!line.compare(0, prefix.size(), prefix))
    number_of_segments = atoi(line.substr(prefix.size()).c_str());

  std::stringstream mpdData;

  mpdData << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
          << "<MPD xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
          << "xmlns=\"urn:mpeg:DASH:schema:MPD:2011\" xsi:schemaLocation=\"urn:mpeg:DASH:schema:MPD:2011\"" << std::endl
          << "profiles=\"urn:mpeg:dash:profile:isoff-main:2011\" type=\"static\"" << std::endl;


  int totalVideoDuration = number_of_segments * segment_duration;

  int videoDurationInSeconds = totalVideoDuration % 60;
  totalVideoDuration -= videoDurationInSeconds;
  totalVideoDuration = totalVideoDuration / 60;
  int videoDurationInMinutes = totalVideoDuration % 60;
  totalVideoDuration -= videoDurationInMinutes;
  int videoDurationInHours = totalVideoDuration / 60;


  mpdData << "mediaPresentationDuration=\"PT" << videoDurationInHours << "H" <<
          videoDurationInMinutes << "M" <<
          videoDurationInSeconds << "S\" ";
  mpdData << "minBufferTime=\"PT2.0S\">" << std::endl;
  mpdData << "<BaseURL>http://" << m_hostName << m_metaDataContentDirectory  << "vid" << video_id << "/</BaseURL>" << std::endl
          << "<Period start=\"PT0S\">" << std::endl << "<AdaptationSet bitstreamSwitching=\"true\">" << std::endl;

  // get header and ignore
  std::getline(infile,line); // reprId,screenWidth,screenHeight,bitrate

  while (std::getline(infile,line))
  {
    if (line.length() > 2) // line must not be empty
    {
      size_t pos1 = line.find(",");
      if (pos1 != std::string::npos)
      {
        std::string repr_id = line.substr(0, pos1);
        line = line.substr(pos1+1);

        pos1 = line.find(",");
        if (pos1 != std::string::npos)
        {
          std::string repr_width = line.substr(0, pos1);
          line = line.substr(pos1+1);

          pos1 = line.find(",");

          if (pos1 != std::string::npos)
          {
            std::string repr_height = line.substr(0, pos1);
            std::string repr_bitrate = line.substr(pos1+1);

            int iBitrate = atoi(repr_bitrate.c_str()); // read bitrate in kilobit/s

            fprintf(stderr, "Representation ID = %s, height = %s, bitrate = %s\n", repr_id.c_str(), repr_height.c_str(), repr_bitrate.c_str());
            mpdData << "<Representation id=\"" << repr_id << "\" codecs=\"avc1\" mimeType=\"video/mp4\"" <<
                 " width=\"" << repr_width << "\" height=\"" << repr_height << "\" startWithSAP=\"1\" bandwidth=\"" << (iBitrate*1000) << "\">" << std::endl;
            mpdData << "<SegmentList duration=\"" << segment_duration << "\">" << std::endl;


            long iSegmentSize = (double)iBitrate/8.0 * (double)segment_duration * 1024; // in byte

            for (int i = 0; i < number_of_segments; i++)
            {
              std::ostringstream segmentFileName;
              segmentFileName << "vid" << video_id << "/repr_" << repr_id << "_seg_" << i << ".264";

              m_fileSizes[m_metaDataContentDirectory + segmentFileName.str()] = iSegmentSize;

              m_virtualFiles.push_back(m_metaDataContentDirectory + segmentFileName.str());
              mpdData << "<SegmentURL media=\"" <<  "repr_" << repr_id << "_seg_" << i << ".264" << "\"/> " << std::endl;
              //fprintf(stderr, "SegmentName=%s\n", (m_metaDataContentDirectory + segmentFileName.str()).c_str());
            }

            mpdData << "</SegmentList>" << std::endl << "</Representation>" << std::endl;

          }
        }
      }
    }
  }

  mpdData << "</AdaptationSet></Period></MPD>" << std::endl;

  infile.close();

  return mpdData.str();
}

void EdgeDashFakeServerApplication::ReportStats()
{
  uint64_t bytes_recv = m_bytes_recv - m_last_bytes_recv;
  uint64_t bytes_sent = m_bytes_sent - m_last_bytes_sent;

  m_throughputTrace(this, bytes_sent, bytes_recv, m_activeClients.size());

  m_last_bytes_recv = m_bytes_recv;
  m_last_bytes_sent = m_bytes_sent;

  if (m_active) {
    m_reportStatsTimer = Simulator::Schedule(Seconds(1.0), &EdgeDashFakeServerApplication::ReportStats, this);
  }
}

// this traces the acutal packet size, including header etc...
void EdgeDashFakeServerApplication::TxTrace(Ptr<Packet const> packet)
{
  m_bytes_sent += packet->GetSize();
}

// this traces the acutal packet size, including header etc...

void EdgeDashFakeServerApplication::RxTrace(Ptr<Packet const> packet)
{
  m_bytes_recv += packet->GetSize();
}

void EdgeDashFakeServerApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  m_lastSocketID = 1;

  m_active = true;

  Ptr<NetDevice> netdevice = GetNode()->GetDevice(0);
  netdevice->TraceConnectWithoutContext("PhyTxEnd", MakeCallback(&EdgeDashFakeServerApplication::TxTrace, this));
  netdevice->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&EdgeDashFakeServerApplication::RxTrace, this));

  m_last_bytes_recv = 0;
  m_last_bytes_sent = 0;

  m_bytes_recv = 0;
  m_bytes_sent = 0;

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

    if (Ipv4Address::IsMatchingType(m_listeningAddress) == true)
    {
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

  // Listen for incoming connections
  m_socket->Listen();
  NS_ASSERT (m_socket != 0);

  // And make sure to handle requests and accepted connections
  m_socket->SetAcceptCallback (MakeCallback(&EdgeDashFakeServerApplication::ConnectionRequested, this),
      MakeCallback(&EdgeDashFakeServerApplication::ConnectionAccepted, this)
  );
}

void EdgeDashFakeServerApplication::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) {
    m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }
}

void EdgeDashFakeServerApplication::AddVideo(std::string video)
{
  int pos1 = video.find("vid") + 3;
  int pos2 = video.find(".csv");
  int video_id = std::stoi(video.substr(pos1, pos2 - pos1));

  std::string mmm = video;
  std::string mpdData = ImportDASHRepresentations(mmm, video_id);

  // compress
  std::string compressedMpdData = zlib_compress_string(mpdData);
  fprintf(stderr, "Size of compressed = %ld, uncompressed = %ld\n", compressedMpdData.length(), mpdData.length());

  std::stringstream SSMpdFilename;

  SSMpdFilename << m_mpdDirectory << "vid" << video_id << ".mpd.gz";

  m_fileSizes[SSMpdFilename.str()] = compressedMpdData.size();

  fprintf(stderr, "Adding '%s' to m_fileSizes with size %ld\n", SSMpdFilename.str().c_str(), compressedMpdData.size());

  m_mpdFileContents[SSMpdFilename.str()] = compressedMpdData;

  ReportStats();
}

void EdgeDashFakeServerApplication::OnReadySend(Ptr<Socket> socket, unsigned int txSize)
{
  fprintf(stderr, "Server says it is ready to send something now...\n");
}

bool EdgeDashFakeServerApplication::ConnectionRequested (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  NS_LOG_DEBUG (Simulator::Now () << " Socket = " << socket << " " << " Server: ConnectionRequested");
  return true;
}

void EdgeDashFakeServerApplication::ConnectionAccepted (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  fprintf(stderr, "DASH Fake Server(%s): Connection Accepted!\n", m_hostName.c_str());

  uint64_t socket_id = RegisterSocket(socket);

  m_activeClients[socket_id] = new HttpServerFakeVirtualClientSocket(socket_id, "/", m_fileSizes, m_virtualFiles, m_mpdFileContents,
                  MakeCallback(&EdgeDashFakeServerApplication::FinishedCallback, this));

  NS_LOG_DEBUG (socket << " " << Simulator::Now () << " Successful socket id : " << socket_id << " Connection Accepted From " << address);

  // set callbacks for this socket to be in HttpServerFakeClientSocket class
  socket->SetSendCallback (MakeCallback (&HttpServerFakeVirtualClientSocket::HandleReadyToTransmit, m_activeClients[socket_id]));
  socket->SetRecvCallback (MakeCallback (&HttpServerFakeVirtualClientSocket::HandleIncomingData, m_activeClients[socket_id]));


  socket->TraceConnectWithoutContext ("State",
    MakeCallback(&HttpServerFakeVirtualClientSocket::LogStateChange, m_activeClients[socket_id]));

    socket->SetCloseCallbacks (MakeCallback (&HttpServerFakeVirtualClientSocket::ConnectionClosedNormal, m_activeClients[socket_id]),
                             MakeCallback (&HttpServerFakeVirtualClientSocket::ConnectionClosedError,  m_activeClients[socket_id]));
}

void EdgeDashFakeServerApplication::FinishedCallback (uint64_t socket_id)
{
  // create timer to finish this, because if we do it in here, we will crash the app
  Simulator::Schedule(Seconds(1.0), &EdgeDashFakeServerApplication::DoFinishSocket, this, socket_id);
  // EdgeDashFakeServerApplication::DoFinishSocket(socket_id);
}

void EdgeDashFakeServerApplication::DoFinishSocket(uint64_t socket_id)
{
  if (m_activeClients.find(socket_id) != m_activeClients.end()) {
    // std::cout << "ENTROU DoFinishSocket\n";
    // getchar();

    HttpServerFakeClientSocket* tmp = m_activeClients[socket_id];
    m_activeClients.erase(socket_id);
    delete tmp; // TODO: CHECK
  }
}

uint64_t EdgeDashFakeServerApplication::RegisterSocket (Ptr<Socket> socket)
{
    this->m_activeSockets[socket] = this->m_lastSocketID;
    return this->m_lastSocketID++;
}

string EdgeDashFakeServerApplication::getVideoPath(int idVideo)
{
    return "content/representations/vid" + to_string(idVideo) + ".csv";
}

string EdgeDashFakeServerApplication::OutputVideos(int start, int n)
{
    string representationStrings = "";
    for (int i = start; i <= n; i++) {
        representationStrings += "content/representations/vid" + to_string(i) + ".csv";
        if (i < n) {
            representationStrings += ",";
        }
    }

    return representationStrings;
}

bool EdgeDashFakeServerApplication::hasVideo(int content)
{
    for (auto& videoId : contentVideos) {
        if (videoId == content) {
            return true;
        }
    }
    return false;
}

bool EdgeDashFakeServerApplication::VideoAssignment(int content)
{
    if (assignedVideos < m_capacity) {
        assignedVideos++;
        BindVideos(content, content);

        return true;
    }

    return false;
}

void EdgeDashFakeServerApplication::AddCapacityNode(int capacity)
{
    m_capacity = capacity;
    assignedVideos = 0;
}

void EdgeDashFakeServerApplication::BindVideos(int start, unsigned contentN)
{
    for (size_t i = start; i <= contentN; i++) {
        contentVideos.push_back(i);
    }
}

}
