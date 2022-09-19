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

// ns3 - HTTP Client Application class


#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "http-client.h"



namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HttpClientApplication");

NS_OBJECT_ENSURE_REGISTERED (HttpClientApplication);

TypeId
HttpClientApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HttpClientApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<HttpClientApplication> ()
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&HttpClientApplication::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets",
                   UintegerValue (80),
                   MakeUintegerAccessor (&HttpClientApplication::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RemoteHostName",
                   "The destinations hostname/servername",
                   StringValue ("localhost"),
                   MakeStringAccessor (&HttpClientApplication::m_hostName),
                   MakeStringChecker ())
    .AddAttribute("FileToRequest", "Name of the File to Request",
                   StringValue("/"),
                   MakeStringAccessor(&HttpClientApplication::m_fileToRequest),
                   MakeStringChecker())
    .AddAttribute("WriteOutfile", "Write the downloaded file to outfile (empty means disabled)",
                   StringValue(""),
                   MakeStringAccessor(&HttpClientApplication::m_outFile),
                   MakeStringChecker())
    .AddAttribute("KeepAlive", "Whether or not the connection should be re-used every time (default: false)",
                   BooleanValue(false),
                   MakeBooleanAccessor(&HttpClientApplication::m_keepAlive),
                   MakeBooleanChecker())
    .AddTraceSource("FileDownloadFinished", "Trace called every time a download finishes",
                   MakeTraceSourceAccessor(&HttpClientApplication::m_downloadFinishedTrace),
                   "bla")
    .AddTraceSource("HeaderReceived", "Trace called every time a header is received",
                   MakeTraceSourceAccessor(&HttpClientApplication::m_headerReceivedTrace),
                   "bla")
    .AddTraceSource("FileDownloadStarted", "Trace called every time a download starts",
                   MakeTraceSourceAccessor(&HttpClientApplication::m_downloadStartedTrace),
                   "bla")
    .AddTraceSource("CurrentPacketStats", "Trace current packets statistics (once per second)",
                   MakeTraceSourceAccessor(&HttpClientApplication::m_currentStatsTrace),
                   "bla")
  ;
  return tid;
}

HttpClientApplication::HttpClientApplication ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  node_id = 0;
  m_socket = 0;

  _tmpbuffer = NULL; // init this thing

  m_tried_connecting = 0;
  m_success_connecting = 0;
  m_failed_connecting = 0;
}

HttpClientApplication::~HttpClientApplication()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;

  if (_tmpbuffer != NULL)
  {
    fprintf(stderr, "tmpbuffer is still not empty...\n");
    free(_tmpbuffer);
  }
}

void
HttpClientApplication::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
HttpClientApplication::SetRemote (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void HttpClientApplication::SetRemote (Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void HttpClientApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void HttpClientApplication::LogStateChange(const ns3::TcpSocket::TcpStates_t old_state, const ns3::TcpSocket::TcpStates_t new_state)
{
  NS_LOG_DEBUG("HttpClient(" << this << "): Socket State Change " << ns3::TcpSocket::TcpStateName[old_state] << " -> "
      << ns3::TcpSocket::TcpStateName[new_state]);

  fprintf(stderr, "Client(%d): Time=%f Socket %s -> %s\n", node_id, Simulator::Now().GetSeconds(), ns3::TcpSocket::TcpStateName[old_state], ns3::TcpSocket::TcpStateName[new_state]);
}

void HttpClientApplication::LogCwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
  fprintf(stderr, "Client(%d): Cwnd Changed %d -> %d\n", node_id, oldCwnd, newCwnd);
  this->cur_cwnd = newCwnd;
}

void HttpClientApplication::TryEstablishConnection (void)
{
  NS_LOG_FUNCTION (this);

  do_cancel_socket = false;


  m_sentGetRequest = false;
  m_finished_download = false;

  m_bytesRecv = 0;
  m_bytesSent = 0;

  if (!m_keepAlive || m_socket == 0)
  {
    m_tried_connecting++;

    if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid); //  TCP NewReno per default (according to documentation)
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
      {
        m_socket->Bind();
        m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
//        int errno = m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
//        NS_LOG_DEBUG("Binding to Ipv4:" << Ipv4Address::ConvertFrom(m_peerAddress) << ":" << m_peerPort << ", errno=" << errno);
      }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
      {
        m_socket->Bind6();
        m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));

        NS_LOG_DEBUG("Binding to Ipv6...");
      }

      //m_socket->ShutdownRecv ();
      m_socket->SetConnectCallback (MakeCallback (&HttpClientApplication::ConnectionComplete, this),
                                MakeCallback (&HttpClientApplication::ConnectionFailed, this));
    } else {
      fprintf(stderr, "Client(%d): ERROR: m_socket != 0\n", node_id);
    }

    m_socket->SetSendCallback (MakeCallback (&HttpClientApplication::OnReadySend, this));

    m_socket->SetCloseCallbacks (MakeCallback (&HttpClientApplication::ConnectionClosedNormal, this),
                              MakeCallback (&HttpClientApplication::ConnectionClosedError, this));

    m_socket->TraceConnectWithoutContext ("State",
      MakeCallback(&HttpClientApplication::LogStateChange, this));

    // UNCOMMENT in case you want CWND tracing on client (not really needed, you do not trace the CWND on the client)
    /*
    m_socket->TraceConnectWithoutContext ("CongestionWindow",
      MakeCallback(&HttpClientApplication::LogCwndChange, this));
    */

    fprintf(stderr, "Waiting for reply from server...\n");

  } else {
    fprintf(stderr, "Keeping connection alive...\n");
    OnReadySend(m_socket, 1200);
  }
}

void HttpClientApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  // only re-init tmpbuffer if it is null
  if (_tmpbuffer == NULL) {
    _tmpbuffer = (uint8_t*)malloc(sizeof(uint8_t)* 128*1024); // 128 kB
  }


  // mark this app as active
  m_active = true;
  _start_time = Simulator::Now().GetMilliSeconds();

  // Create OutFile
  if (!m_outFile.empty())
  {
    fprintf(stderr, "Client(%d): Creating outfile %s\n", node_id, m_outFile.c_str());
    // (re)create outfile
    FILE* fp = fopen(m_outFile.c_str(), "w");
    fclose(fp);
  }

  fprintf(stderr, "Establishing connection (time=%f)...\n",Simulator::Now().GetSeconds());
  AgentTryEstablishConnection();
  TryEstablishConnection();

  m_lastStatsReportedBytesRecv = 0;
  m_lastStatsReportedBytesSent = 0;
  // start stats reporter
  //ReportStats();
}

void HttpClientApplication::ReportStats()
{
  unsigned int bytes_recv_last_timespan = m_bytesRecv - m_lastStatsReportedBytesRecv;
  unsigned int bytes_sent_last_timespan = m_bytesSent - m_lastStatsReportedBytesSent;


  m_currentStatsTrace(this, this->m_fileToRequest, bytes_recv_last_timespan);


  std::stringstream cwnd_trace_filename;
  cwnd_trace_filename << "traces/cwnd_" << node_id << ".csv";

  FILE* fp = fopen(cwnd_trace_filename.str().c_str(), "a");

  fprintf(fp, "Client(%d):Time=%f,bytes_recv=%d,bytes_sent=%d,cwnd=%d;finished_download=%d-%ld;tried_connect=%d;failed_connect=%d;success_connect=%d\n",
      node_id, Simulator::Now().GetSeconds(), bytes_recv_last_timespan, bytes_sent_last_timespan, cur_cwnd, m_finished_download, _finished_time, m_tried_connecting, m_failed_connecting, m_success_connecting);

  fclose(fp);

  m_lastStatsReportedBytesRecv = m_bytesRecv;
  m_lastStatsReportedBytesSent = m_bytesSent;

//  if (m_active == true) {
//    // schedule again
//    //m_reportStatsEvent = Simulator::Schedule(Seconds(0.1), &HttpClientApplication::ReportStats, this);
//  }
}

void HttpClientApplication::ConnectionClosedNormal (Ptr<Socket> socket)
{
  fprintf(stderr, "Client(%d): Socket was closed normally\n", node_id);
  // socket is in CLOSE_WAIT state --> close the socket here --> socket will be in LAST_ACK state
  socket->Close();
  socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket> > (),MakeNullCallback<void, Ptr<Socket> > ());
}

void HttpClientApplication::ConnectionClosedError (Ptr<Socket> socket)
{
  fprintf(stderr,"Client(%d): Socket was closed with an error, errno=%d; Trying to open it again...\n", node_id, socket->GetErrno());
  socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket> > (),MakeNullCallback<void, Ptr<Socket> > ());

  // let's try opening the second again again in 0.5 second
  Simulator::Schedule(Seconds(0.5), &HttpClientApplication::TryEstablishConnection, this);
}

void HttpClientApplication::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (_tmpbuffer != NULL) {
    free(_tmpbuffer);
    _tmpbuffer = NULL;
  }

  m_active = false;

  if (m_socket != 0 && !m_keepAlive) {
    fprintf(stderr, "Client(%d): Socket is still open, closing it...\n", node_id);
    m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    m_socket = 0;
  } else {
    fprintf(stderr, "We are in stop application, but keeping alive...\n");
  }

  if (!Simulator::IsExpired(m_reportStatsEvent)) {
    Simulator::Cancel (m_reportStatsEvent);
  }

}

void HttpClientApplication::ConnectionComplete (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_DEBUG("Client Connection Completed!");

  fprintf(stderr, "Client successfully connected at time=%f\n", Simulator::Now().GetSeconds());

  m_success_connecting++;


  // Get ready to receive.
  socket->SetRecvCallback (MakeCallback (&HttpClientApplication::HandleRead, this));
}

void HttpClientApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  fprintf(stderr, "Client(%d): ERROR: OnConnectionFailed - failed to open connection\n", node_id);

  m_failed_connecting++;

  // Well, this is not supposed to happen...
  NS_LOG_WARN ("Client failed to open connection.");
}

void HttpClientApplication::OnReadySend (Ptr<Socket> localSocket, uint32_t txSpace)
{
  NS_LOG_FUNCTION (this);
  fprintf(stderr, "HttpClientApp::OnReadySend()\n");
  if (!m_sentGetRequest) {
    m_sentGetRequest = true;
    DoSendGetRequest(localSocket, txSpace);
  }
}

void HttpClientApplication::DoSendGetRequest (Ptr<Socket> localSocket, uint32_t txSpace)
{
  NS_LOG_FUNCTION (this);

  m_downloadStartedTrace(this, this->m_fileToRequest);

  // Create HTTP 1.1 compatible request
  std::stringstream requestSS;
  fprintf(stderr, "Client(%d, %f): Executing  'GET %s'\n", node_id, Simulator::Now().GetSeconds(), m_fileToRequest.c_str());
  requestSS << "GET " << m_fileToRequest << " HTTP/1.1" << CRLF;
  requestSS << "Host: " << m_hostName << CRLF;
  //requestSS << "Pragma: no-cache" << CRLF;
  //requestSS << "Cache-Control: no-cache" << CRLF;
  requestSS << "Accept: text/html,application/xml" << CRLF;
  requestSS << "User-Agent: ns-3 (applications/model/http-client.cc)" << CRLF;
  requestSS << "Accept-Encoding: identity" << CRLF; // no compression, gzip, etc... allowed

  if (m_keepAlive) {
    requestSS << "Connection: keep-alive" << CRLF;
  }


  // empty line, to indicate that this is the end of the request (without a body)
  requestSS << CRLF;


  m_is_first_packet = true;


  std::string requestString = requestSS.str();
  //fprintf(stderr, "Creating Request String:\n%s\n------------\n", requestString.c_str());


  NS_LOG_INFO("CLIENT: Initiating GET request of length " << requestString.length());

  // copy the pointer
  uint8_t* buffer = (uint8_t*)requestString.c_str();

  Ptr<Packet> p = Create<Packet> (buffer, requestString.length());


  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);
  localSocket->Send (p);

  m_bytesSent += requestString.length();

  ++m_sent;

  if (Ipv4Address::IsMatchingType (m_peerAddress)) {
    NS_LOG_INFO ("CLIENT: At time " << Simulator::Now ().GetSeconds () << "s client sent " << requestString.length() << " bytes to " <<
                 Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  } else if (Ipv6Address::IsMatchingType (m_peerAddress)) {
    NS_LOG_INFO ("CLIENT: At time " << Simulator::Now ().GetSeconds () << "s client sent " << requestString.length() << " bytes to " <<
                 Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  }

  //m_socket->ShutdownSend();
}

void HttpClientApplication::CancelDownload()
{
  // next time the client receives something, we cancel the socket
  do_cancel_socket = true;
}

uint32_t HttpClientApplication::ParseResponseHeader(const uint8_t* buffer, size_t len, int* realStatusCode, unsigned int* contentLength)
{
  /*
  HTTP/1.1 200 OKCRLF
  Content-Type: text/xml; charset=utf-8CRLF
  Content-Length: {len}CRLFCRLF;
  */

  fprintf(stderr, "Client(%d): Parsing Response Header of length %ld\n", node_id, len);
  const char* strbuffer = (const char*) buffer;

  //fprintf(stderr, "header=\n%s\n", buffer);

  // should start with a HTTP response
  if (strncmp(strbuffer, "HTTP/1.1",8) == 0)
  {
    const char* statusCode = &strbuffer[9];
    // find next space
    char needle[2];
    needle[0] = ' ';
    needle[1] = '\0';
    const char *p = strstr(statusCode, (const char*)needle);
    if (p) {
      int pos = p - statusCode;

      char actualStatusCode[4];

      strncpy(actualStatusCode, statusCode,pos);
      actualStatusCode[3] = '\0';

      int iStatusCode = atoi(actualStatusCode);

      if (iStatusCode == 404) {
        fprintf(stderr, "Client(%d): ParseHeader: Status Code 404, not found!\n", node_id);
      } else {
        // find Content-Length
        char needle2[17];
        strcpy(needle2, "Content-Length: ");
        needle2[16] = '\0';
        p = strstr(statusCode, needle2);

        if (p)
        {
          const char* strContentLength = &statusCode[p-statusCode+16];

          p = strstr(strContentLength, "\r\n");
          if (p)
          {
            pos = p - strContentLength;
            char actualContentLength[12];
            strncpy(actualContentLength,strContentLength,pos);
            unsigned int iActualContentLength = atoi(actualContentLength);


            // find the end of the header, where the body content begins
            p = strstr(strbuffer, "\r\n\r\n");

            if (p) {
              // done!
              *realStatusCode = iStatusCode;
              *contentLength = iActualContentLength;

              pos = p - strbuffer;
              return pos+4; // +4 to skip CRLFCRLF
            } else {
              fprintf(stderr, "ERROR: could not find where body begins\n");
            }
          } else {
            fprintf(stderr, "ERROR: No CRLF found?!?\n");
          }

        } else {
          fprintf(stderr, "ERROR: Server did not reply Content-Length Header field\n");
        }
      }

    } else {
      fprintf(stderr, "Invalid HTTP Response, %s\n", strbuffer);
    }

  } else {
    fprintf(stderr, "Not sure what this response header means\n");
    fprintf(stderr, "Result=%s\n", strbuffer);
  }


  return 0;
}


void HttpClientApplication::OnFileReceived(unsigned status, unsigned length)
{
  if (!m_active)
    return;

  fprintf(stderr, "Client(%d, %f): File received\n", node_id, Simulator::Now().GetSeconds());

  m_finished_download = true;
  _finished_time = Simulator::Now().GetMilliSeconds ();


  long milliSeconds = _finished_time - _start_time;
  double seconds = ((double)milliSeconds)/ 1000.0;

  double downloadSpeed = ((double)requested_content_length)/((double)seconds);

  lastDownloadBitrate = downloadSpeed * 8.0; // do not forget to do *8, as this is a BIT-rate

  m_downloadFinishedTrace(this, this->m_fileToRequest, downloadSpeed, milliSeconds);
}

void HttpClientApplication::ForceCloseSocket()
{
  if (m_socket != 0)
  {
    if (m_keepAlive)
    {
      m_socket->Close();
    }
  }
}

void HttpClientApplication::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this << socket << "URL=" << m_fileToRequest);
  std::cout << "entrou aqui IncomingDataFromServer" << '\n';
  std::cout << m_is_first_packet << '\n';
  std::cout << m_bytesRecv << " " << requested_content_length << '\n';

  if (m_finished_download) {
    fprintf(stderr, "Client(%d)::HandleRead(time=%f) Client is asked to HandleRead although it should have finished already...\n", node_id, Simulator::Now().GetSeconds());
    return;
  }

  Ptr<Packet> packet;
  Address from;

  uint32_t bytes_recv_this_time = 0;

  while (true)
  {
    packet = socket->RecvFrom (from);

    if (!(packet))
    {
      break;
    }


    packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();
    // PARSE PACKET
    size_t packet_size = packet->CopyData(_tmpbuffer, packet->GetSize());
    _tmpbuffer[packet_size] = '\0';

    bytes_recv_this_time += packet_size;

    if (m_is_first_packet)
    {
      m_is_first_packet = false;
      // parse header
      int status_code = 0;
      int where = ParseResponseHeader(_tmpbuffer, packet_size, &status_code, &(this->requested_content_length));
      //fprintf(stderr, "content starts at position %d, with length %d (status code %d)\n", where, requested_content_length, status_code);
      m_bytesRecv += packet_size - where;

      m_headerReceivedTrace(this, this->m_fileToRequest, requested_content_length);

      // write to file
      // if (!m_outFile.empty()) {
      //   // open outfile to append
      //   FILE* fp = fopen(m_outFile.c_str(), "a");
      //
      //   fwrite(&_tmpbuffer[where], sizeof(uint8_t), packet_size-where, fp);
      //
      //   fclose(fp);
      // }

    } else {
      m_bytesRecv += packet_size;

      // write to file
      // if (!m_outFile.empty()) {
      //   // open outfile to append
      //   FILE* fp = fopen(m_outFile.c_str(), "a");
      //
      //   fwrite(_tmpbuffer, sizeof(uint8_t), packet_size, fp);
      //
      //   fclose(fp);
      // }
    }


    // we have received the whole file!
    if (m_bytesRecv == requested_content_length) {
      NS_LOG_DEBUG("All bytes received, this means we are done...");
      OnFileReceived(0, requested_content_length);
      break;
    } else if (m_bytesRecv > requested_content_length) {
      fprintf(stderr, "Client(%d)::HandleRead(time=%f) Expected only %d bytes, but received already %d bytes\n",
        node_id, Simulator::Now().GetSeconds(), requested_content_length, m_bytesRecv);
    }

  }

  //fprintf(stderr, "Client(%d)::HandleRead(time=%f) handled %d bytes this time\n", node_id, Simulator::Now().GetSeconds(), bytes_recv_this_time);
}


//=======================================================================================
// PARALLEL CONNECTION WITH AGGREGATOR AGENT
//=======================================================================================

void HttpClientApplication::AgentTryEstablishConnection()
{
  NS_LOG_FUNCTION (this);

  if (gta_socket == 0) {
    TypeId tid = TypeId::LookupByName("ns3::TcpSocketFactory");
    gta_socket = Socket::CreateSocket(GetNode (), tid); //  TCP NewReno per default (according to documentation)

    if (Ipv4Address::IsMatchingType(m_peerAddress) == true) {
      gta_socket->Bind();
      gta_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), 1317));

      NS_LOG_DEBUG("Binding to Ipv4:" << Ipv4Address::ConvertFrom(m_peerAddress) << ":" << 1317);
    } else if (Ipv6Address::IsMatchingType(m_peerAddress) == true) {
      gta_socket->Bind6();
      gta_socket->Connect(Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), 1317));

      NS_LOG_DEBUG("Binding to Ipv6...");
    }

    gta_socket->SetConnectCallback(MakeCallback (&HttpClientApplication::AgentConnectionComplete, this),
                                   MakeCallback (&HttpClientApplication::AgentConnectionFailed, this));
  }
}

void HttpClientApplication::AgentConnectionComplete (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    NS_LOG_DEBUG("Client Connection Completed with Aggregation!");

    // fprintf(stderr, "Client successfully connected with DashReqServer at time=%f\n", Simulator::Now().GetSeconds());

    // Get ready to receive.
    socket->SetRecvCallback (MakeCallback (&HttpClientApplication::AgentHandleRead, this));

//    AgentDoSend(socket, 0);
}

void HttpClientApplication::AgentConnectionFailed (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    fprintf(stderr, "Client(%d): ERROR: OnConnectionFailed - failed to open connection with DashReqServer\n", node_id);

    // Well, this is not supposed to happen...
    NS_LOG_WARN ("Client failed to open connection.");
}

void HttpClientApplication::AgentDoSend (Ptr<Socket> socket, uint32_t txSpace, double qoe)
{
  std::string str_qoe = std::to_string(qoe);

	std::cout << "Client (" << node_id << "," << strNodeIpv4 << ") " << Simulator::Now ().GetSeconds() << " Socket AgentDoSend QoE video " << str_qoe << std::endl;
  // getchar();

  uint8_t* buffer = (uint8_t*)str_qoe.c_str();

	Ptr<Packet> p = Create<Packet> (buffer, str_qoe.length());

  socket->Send(p);
}

void HttpClientApplication::AgentHandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;

  packet = socket->RecvFrom (from);

  if (!(packet)) {
    return;
  }

	uint8_t *buffer = new uint8_t[packet->GetSize ()];
	packet->CopyData(buffer, packet->GetSize ());

	std::string str_ip = std::string(buffer, buffer+packet->GetSize());

	std::cout << "Client(" << node_id << "," << ")--> New hostname=" << str_ip << " Old=" << m_hostName << std::endl;
	if (str_ip != m_hostName) {
    m_hostName = str_ip;

    SetRemote(Ipv4Address(m_hostName.c_str()), 80);
    SetAttribute("KeepAlive", StringValue("true"));

    m_redirectRequest = true;
	}

}

} // Namespace ns3
