
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"

#include "ns3/simulator.h"

#include "http-client-dash.h"

#include <fstream>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HttpClientDashApplication");
NS_OBJECT_ENSURE_REGISTERED (HttpClientDashApplication);

TypeId
HttpClientDashApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HttpClientDashApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<HttpClientDashApplication> ()
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&HttpClientDashApplication::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets",
                   UintegerValue (80),
                   MakeUintegerAccessor (&HttpClientDashApplication::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RemoteHostName",
                   "The destinations hostname/servername",
                   StringValue ("localhost"),
                   MakeStringAccessor (&HttpClientDashApplication::m_hostName),
                   MakeStringChecker ())
    .AddAttribute("FileToRequest", "Name of the File to Request",
                   StringValue("/"),
                   MakeStringAccessor(&HttpClientDashApplication::m_fileToRequest),
                   MakeStringChecker())
    .AddAttribute("WriteOutfile", "Write the downloaded file to outfile (empty means disabled)",
                   StringValue(""),
                   MakeStringAccessor(&HttpClientDashApplication::m_outFile),
                   MakeStringChecker())
    .AddAttribute("KeepAlive", "Whether or not the connection should be re-used every time (default: false)",
                   BooleanValue(false),
                   MakeBooleanAccessor(&HttpClientDashApplication::m_keepAlive),
                   MakeBooleanChecker())
    .AddTraceSource("FileDownloadFinished", "Trace called every time a download finishes",
                   MakeTraceSourceAccessor(&HttpClientDashApplication::m_downloadFinishedTrace),
                   "bla")
    .AddTraceSource("HeaderReceived", "Trace called every time a header is received",
                   MakeTraceSourceAccessor(&HttpClientDashApplication::m_headerReceivedTrace),
                   "bla")
    .AddTraceSource("FileDownloadStarted", "Trace called every time a download starts",
                   MakeTraceSourceAccessor(&HttpClientDashApplication::m_downloadStartedTrace),
                   "bla")
    .AddTraceSource("CurrentPacketStats", "Trace current packets statistics (once per second)",
                   MakeTraceSourceAccessor(&HttpClientDashApplication::m_currentStatsTrace),
                   "bla")
  ;
  return tid;
}

HttpClientDashApplication::HttpClientDashApplication ()
{
  NS_LOG_FUNCTION (this);
  this->m_sent = 0;
  this->node_id = 0;
  this->m_socket = 0;

  this->_tmpbuffer = NULL; // init this thing

  this->m_tried_connecting = 0;
  this->m_success_connecting = 0;
  this->m_failed_connecting = 0;
}

HttpClientDashApplication::~HttpClientDashApplication()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;

  if (_tmpbuffer != NULL)
  {
    fprintf(stderr, "tmpbuffer is still not empty...\n");
    free(_tmpbuffer);
  }
}

void HttpClientDashApplication::StartApplication()
{
  NS_LOG_FUNCTION (this);

  if (this->_tmpbuffer == NULL) {
    this->_tmpbuffer = (uint8_t*) malloc(sizeof(uint8_t) * 128 * 1024);  // 128kB
  }

  // mark this app as active
  m_active = true;
  _start_time = Simulator::Now ().GetMilliSeconds ();

  fprintf(stderr, "Establishing connection (time=%f)...\n",Simulator::Now().GetSeconds());

  TryEstablishConnection();
}

void HttpClientDashApplication::StopApplication()
{
    NS_LOG_FUNCTION_NOARGS();
    fprintf(stderr, "Client(%d): Stopping...\n", this->node_id);

    m_active = false;

    if (this->_tmpbuffer != NULL) {
      free(this->_tmpbuffer);
      this->_tmpbuffer = NULL;
    }

    if (m_socket != 0 && !m_keepAlive) {
      m_socket->Close();
      m_socket = 0;
    } else {
      fprintf(stderr, "We are in stop application, but keep alive ...\n");
    }
}

void HttpClientDashApplication::TryEstablishConnection()
{
  NS_LOG_FUNCTION (this);

  m_sentGetRequest = false;
  m_finished_download = false;

  m_bytesRecv = 0;
  m_bytesSent = 0;

  if (!m_keepAlive || m_socket == 0) {

    if (m_socket == 0) {
      std::cout << "creating socket client" << '\n';
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid); //  TCP NewReno per default (according to documentation)
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true) {
        m_socket->Bind();
        m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
      } else if (Ipv6Address::IsMatchingType(m_peerAddress) == true) {
        m_socket->Bind6();
        m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));

        NS_LOG_DEBUG("Binding to Ipv6...");
      }

      m_socket->SetSendCallback (MakeCallback (&HttpClientDashApplication::OnReadySend, this));

      m_socket->SetConnectCallback (MakeCallback (&HttpClientDashApplication::ConnectionComplete, this),
                                    MakeCallback (&HttpClientDashApplication::ConnectionFailed, this));

    }
    m_socket->SetCloseCallbacks (MakeCallback (&HttpClientDashApplication::ConnectionClosedNormal, this),
                                 MakeCallback (&HttpClientDashApplication::ConnectionClosedError, this));

  } else {
    fprintf(stderr, "Keeping connection alive...\n");
    OnReadySend(m_socket, 1200);
  }
}

void HttpClientDashApplication::ConnectionComplete (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_DEBUG("Client Connection Completed!");

  fprintf(stderr, "Client successfully connected at time=%f\n", Simulator::Now().GetSeconds());
  m_success_connecting++;

  // Get ready to receive.
  socket->SetRecvCallback (MakeCallback (&HttpClientDashApplication::HandleRead, this));
}

void HttpClientDashApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  fprintf(stderr, "Client(%d): ERROR: OnConnectionFailed - failed to open connection\n", node_id);
}

void HttpClientDashApplication::ConnectionClosedNormal (Ptr<Socket> socket)
{
  if (socket == 0) {
    return;
  }

  fprintf(stderr, "Client(%d): Socket was closed normally\n", node_id);
  // socket is in CLOSE_WAIT state --> close the socket here --> socket will be in LAST_ACK state
  socket->Close();

  socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket> > (),MakeNullCallback<void, Ptr<Socket> > ());
}

void HttpClientDashApplication::ConnectionClosedError (Ptr<Socket> socket)
{
  if (socket != 0) {
    fprintf(stderr,"Client(%d): Socket was closed with an error, errno=%d; Trying to open it again...\n", node_id, socket->GetErrno());
    socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket> > (),MakeNullCallback<void, Ptr<Socket> > ());
  }

  // let's try opening the second again again in 0.5 second
  Simulator::Schedule(Seconds(0.5), &HttpClientDashApplication::TryEstablishConnection, this);
}

void HttpClientDashApplication::OnReadySend (Ptr<Socket> localSocket, uint32_t txSpace)
{
  NS_LOG_FUNCTION (this);
  if (!m_sentGetRequest) {
    m_sentGetRequest = true;
    DoSendGetRequest(localSocket, txSpace);
  }
}

bool HttpClientDashApplication::endsWith(const std::string& s, const std::string& suffix)
{
    return s.size() >= suffix.size() &&
           s.substr(s.size() - suffix.size()) == suffix;
}

vector<string> HttpClientDashApplication::split(const string& s, const string& delimiter)
{
  const bool& removeEmptyEntries = false;
  vector<string> tokens;

  for (size_t start = 0, end; start < s.length(); start = end + delimiter.length()) {
     size_t position = s.find(delimiter, start);
     end = position != string::npos ? position : s.length();

     string token = s.substr(start, end - start);
     if (!removeEmptyEntries || !token.empty()) {
       tokens.push_back(token);
     }
  }

  if (!removeEmptyEntries && (s.empty() || endsWith(s, delimiter))) {
    tokens.push_back("");
  }

  return tokens;
}

void HttpClientDashApplication::DoSendGetRequest (Ptr<Socket> localSocket, uint32_t txSpace)
{
  NS_LOG_FUNCTION (this);

  string hostname = getServerTableList(strNodeIpv4);
  if (m_hostName != hostname ) {
    fprintf(stderr, "Client(%d,%s): Old Hostname = %s new Hostname = %s\n", node_id, strNodeIpv4.c_str(), m_hostName.c_str(), hostname.c_str());

    stringstream ssValue;
    ifstream inputFile("UsersConnection");

    string line;
		while (getline(inputFile, line)) {
      vector<string> values = split(line, " ");
      string strNodeId = to_string(node_id);

      if(strNodeId == values[0]) {
        ssValue << values[0] << " " << values[1] << " " <<  values[2] << " " << values[3] << " " << hostname << endl;
      } else {
        ssValue << values[0] << " " << values[1] << " " <<  values[2] << " " << values[3] << " " << values[4] << endl;
      }
		}
    inputFile.close();

    ofstream newOutputFile;
    newOutputFile.open("UsersConnection", ios::out);
    newOutputFile << ssValue.str();
    newOutputFile.flush();
  	newOutputFile.close();

    // for (auto& server : *serverTableList) {
    //   std::cout << server.first << " " << server.second << '\n';
    // }
    // getchar();

    m_hostName = hostname;
    SetRemote(Ipv4Address(m_hostName.c_str()),80);
    SetAttribute("KeepAlive", StringValue("false"));

  } else {
    SetAttribute("KeepAlive", StringValue("true"));
  }

  m_downloadStartedTrace(this, this->m_fileToRequest);

  // Create HTTP 1.1 compatible request
  stringstream requestSS;
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

  string requestString = requestSS.str();

  NS_LOG_INFO ("CLIENT: Initiating GET request of length " << requestString.length());
  cout << "CLIENT: Initiating GET request of length " << requestString.length() << " in hostname " << m_hostName << "****" << endl;

  // copy the pointer
  uint8_t* buffer = (uint8_t*)requestString.c_str();

  Ptr<Packet> p = Create<Packet> (buffer, requestString.length());

  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);
  localSocket->Send (p);

  m_bytesSent += requestString.length();

  if (Ipv4Address::IsMatchingType (m_peerAddress)) {
    NS_LOG_INFO ("CLIENT: At time " << Simulator::Now ().GetSeconds () << "s client sent " << requestString.length() << " bytes to " <<
                 Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  } else if (Ipv6Address::IsMatchingType (m_peerAddress)) {
    NS_LOG_INFO ("CLIENT: At time " << Simulator::Now ().GetSeconds () << "s client sent " << requestString.length() << " bytes to " <<
                 Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  }

  m_is_first_packet = true;
  m_sent++;
}

void HttpClientDashApplication::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this << socket << "URL=" << m_fileToRequest);

  if (m_finished_download) {
    fprintf(stderr, "Client(%d)::HandleRead(time=%f) Client is asked to HandleRead although it should have finished already...\n", node_id, Simulator::Now().GetSeconds());
    return;
  }

  Ptr<Packet> packet;
  Address from;

  uint32_t bytes_recv_this_time = 0;

  while (true) {
    packet = socket->RecvFrom (from);

    if (!(packet) || packet->GetSize () == 0) {
      break;
    }

    packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();

    // PARSE PACKET
    size_t packet_size = packet->CopyData(_tmpbuffer, packet->GetSize());
    _tmpbuffer[packet_size] = '\0';

    bytes_recv_this_time += packet_size;

    if (m_is_first_packet) {
      m_is_first_packet = false;

      // parse header
      int status_code = 0;
      int where = ParseResponseHeader(_tmpbuffer, packet_size, &status_code, &(this->requested_content_length));
      //fprintf(stderr, "content starts at position %d, with length %d (status code %d)\n", where, requested_content_length, status_code);
      m_bytesRecv += packet_size - where;

      m_headerReceivedTrace(this, this->m_fileToRequest, requested_content_length);

      // write to file
      if (!m_outFile.empty() & m_isMpd) {
        FILE* fp = fopen(m_outFile.c_str(), "a");
        fwrite(&_tmpbuffer[where], sizeof(uint8_t), packet_size-where, fp);

        fclose(fp);
      }

    } else {
      m_bytesRecv += packet_size;

      // write to file
      if (!m_outFile.empty() & m_isMpd) {
        // open outfile to append
        FILE* fp = fopen(m_outFile.c_str(), "a");
        fwrite(_tmpbuffer, sizeof(uint8_t), packet_size, fp);

        fclose(fp);
      }
    }

    // we have received the whole file!
    if (m_bytesRecv == requested_content_length) {
      NS_LOG_DEBUG("All bytes received, this means we are done...");
      fprintf(stderr, "Client(%d) All bytes received, this means we are done...\n", node_id);
      OnFileReceived(0, requested_content_length);
      break;
    } else if (m_bytesRecv > requested_content_length) {
      fprintf(stderr, "Client(%d)::HandleRead(time=%f) Expected only %d bytes, but received already %d bytes\n",
              node_id, Simulator::Now().GetSeconds(), requested_content_length, m_bytesRecv);
    }
  }
}

uint32_t HttpClientDashApplication::ParseResponseHeader(const uint8_t* buffer, size_t len, int* realStatusCode, unsigned int* contentLength)
{
  /*
    HTTP/1.1 200 OKCRLF
    Content-Type: text/xml; charset=utf-8CRLF
    Content-Length: {len}CRLFCRLF;
  */

  fprintf(stderr, "Client(%d): Parsing Response Header of length %ld\n", node_id, len);
  const char* strbuffer = (const char*) buffer;

  // should start with a HTTP response
  if (strncmp(strbuffer, "HTTP/1.1",8) == 0) {
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

        if (p) {
          const char* strContentLength = &statusCode[p-statusCode+16];

          p = strstr(strContentLength, "\r\n");
          if (p) {
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

void HttpClientDashApplication::OnFileReceived(unsigned status, unsigned length)
{
  fprintf(stderr, "Client(%d, %f): File received\n", node_id, Simulator::Now().GetSeconds());

  m_finished_download = true;
  _finished_time = Simulator::Now().GetMilliSeconds ();

  long milliSeconds = _finished_time - _start_time;
  double seconds = ((double)milliSeconds)/ 1000.0;

  double downloadSpeed = ((double)requested_content_length)/((double)seconds);

  lastDownloadBitrate = downloadSpeed * 8.0; // do not forget to do *8, as this is a BIT-rate

  m_downloadFinishedTrace(this, this->m_fileToRequest, downloadSpeed, milliSeconds);
}

void HttpClientDashApplication::setServerTableList (std::map<std::string, std::string> *serverTableList)
{
  this->serverTableList = serverTableList;
}

string HttpClientDashApplication::getServerTableList (std::string server)
{
  return (*serverTableList)[server];
}

void HttpClientDashApplication::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void HttpClientDashApplication::SetRemote (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void HttpClientDashApplication::SetRemote (Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void HttpClientDashApplication::ForceCloseSocket()
{
  if (m_socket != 0 && m_keepAlive) {
    m_socket->Close();
  }

  if (gta_socket != 0) {
    gta_socket->Close();
  }
}


//=======================================================================================
// PARALLEL CONNECTION WITH AGGREGATOR AGENT
//=======================================================================================

void HttpClientDashApplication::AgentTryEstablishConnection()
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

    gta_socket->SetConnectCallback(MakeCallback (&HttpClientDashApplication::AgentConnectionComplete, this),
                                   MakeCallback (&HttpClientDashApplication::AgentConnectionFailed, this));
  }
}

void HttpClientDashApplication::AgentConnectionComplete (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    NS_LOG_DEBUG("Client Connection Completed with Aggregation!");
    fprintf(stderr, "Client successfully connected with DashReqServer at time=%f\n", Simulator::Now().GetSeconds());

    // Get ready to receive.
    socket->SetRecvCallback (MakeCallback (&HttpClientDashApplication::AgentHandleRead, this));
}

void HttpClientDashApplication::AgentConnectionFailed (Ptr<Socket> socket)
{
    // NS_LOG_FUNCTION (this << socket);
    fprintf(stderr, "Client(%d): ERROR: OnConnectionFailed - failed to open connection with DashReqServer\n", node_id);
//     // Well, this is not supposed to happen...
//     NS_LOG_WARN ("Client failed to open connection.");
}

void HttpClientDashApplication::AgentHandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;

  packet = socket->RecvFrom (from);
  if (!(packet)) {
    return;
  }

	uint8_t *buffer = new uint8_t[packet->GetSize ()];
	packet->CopyData(buffer, packet->GetSize ());

	string str_ip = string(buffer, buffer+packet->GetSize());

	cout << "Client(" << node_id << "," << ") --> New hostname=" << str_ip << " Old=" << m_hostName << endl;

	if (str_ip != m_hostName) {
    m_hostName = str_ip;

    SetRemote(Ipv4Address(m_hostName.c_str()), 80);
    SetAttribute("KeepAlive", StringValue("true"));

//     m_redirectRequest = true;
	}
}

// void HttpClientDashApplication::AgentDoSend (Ptr<Socket> socket, uint32_t txSpace, double qoe)
// {
//   string str_qoe = to_string(qoe);
//
// 	cout << "Client (" << node_id << "," << node_ipv4str << ") " << Simulator::Now ().GetSeconds() << " Socket AgentDoSend QoE video " << str_qoe << std::endl;
//   // getchar();
//
//   uint8_t* buffer = (uint8_t*)str_qoe.c_str();
// 	Ptr<Packet> p   = Create<Packet> (buffer, str_qoe.length());
//
//   socket->Send(p);
// }


}
