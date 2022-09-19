#include <fstream>

#include "dash-fake-virtual-clientsocket.h"

#define CRLF "\r\n"

namespace ns3 {

DashFakeVirtualClientSocket::DashFakeVirtualClientSocket(std::map<std::string, long>& fileSizes,
  std::string& mpdFile,
  uint64_t socket_id,
  Ptr<Socket> socket_usr,
  Ptr<Node> node,
  std::string m_remoteCDN,
  std::string m_hostName,
  Callback<void, uint64_t> finished_callback) :
  m_fileSizes(fileSizes),
  mpdFile(mpdFile),
  m_socket_usr(socket_usr),
  node(node),
  m_remoteCDN(m_remoteCDN),
  m_hostName(m_hostName),
  m_peerPort(80)
{
  this->m_socket_id = socket_id;
  this->m_finished_callback = finished_callback;
  this->_tmpbuffer = (uint8_t*)malloc(sizeof(uint8_t)* 128*1024); // 128 kB

  TryEstablishConnection();
}

DashFakeVirtualClientSocket::~DashFakeVirtualClientSocket()
{
  fprintf(stderr, "Server(%ld): Destructing Client Socket(%ld)...\n", m_socket_id, m_socket_id);
  this->m_bytesToTransmit.clear();
}

void DashFakeVirtualClientSocket::TryEstablishConnection()
{
  bytes_recv = 0;
  bytes_sent = 0;

  m_peerAddress = Ipv4Address(m_remoteCDN.c_str());

  if (this->m_socket_svr == 0) {
    TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
    m_socket_svr = Socket::CreateSocket (node, tid); //  TCP NewReno per default (according to documentation)

    if (Ipv4Address::IsMatchingType(m_peerAddress) == true) {
      this->m_socket_svr->Bind();
      this->m_socket_svr->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
    }

    // this->m_socket_svr->SetSendCallback (MakeCallback(&DashFakeVirtualClientSocket::HandleReadyToTransmit, this));
    this->m_socket_svr->SetConnectCallback (MakeCallback (&DashFakeVirtualClientSocket::ConnectionComplete, this),
                              MakeCallback (&DashFakeVirtualClientSocket::ConnectionFailed, this));
  } else {
    fprintf(stderr, "CacheServerClient(%d): ERROR: m_socket != 0\n", this->node->GetId());
  }
  fprintf(stderr, "Waiting for reply from server...\n");
}

void DashFakeVirtualClientSocket::ConnectionComplete(Ptr<Socket> socket)
{
  fprintf(stderr, "Cache-Server successfully connected \n");

  m_currentBytesTx = 0;
  m_totalBytesToTx = 0;

  socket->SetRecvCallback (MakeCallback(&DashFakeVirtualClientSocket::IncomingDataFromServer, this));
}

void DashFakeVirtualClientSocket::ConnectionFailed(Ptr<Socket> socket)
{

}

void DashFakeVirtualClientSocket::IncomingDataFromServer(Ptr<Socket> socket)
{
  std::cout << "entrou aqui IncomingDataFromServer" << '\n';
  std::cout << m_is_first_packet << '\n';
  std::cout << bytes_recv_from_server << " " << requested_content_length << '\n';
  Ptr<Packet> packet;
  Address from;

  uint32_t bytes_recv_this_time = 0;

  while (true)
  {
    packet = socket->RecvFrom (from);
    if (!(packet)) {
      break;
    }

    packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();

    size_t packet_size = packet->CopyData(_tmpbuffer, packet->GetSize());
    _tmpbuffer[packet_size] = '\0';

    bytes_recv_this_time += packet_size;
    if (m_is_first_packet) {
      m_is_first_packet = false;
      int status_code = 0;

      int where = ParseResponseHeader(_tmpbuffer, packet_size, &status_code, &(requested_content_length));

      bytes_recv_from_server += packet_size - where;
    } else {
      bytes_recv_from_server += packet_size;
    }
    std::cout << bytes_recv_from_server << " " << requested_content_length << '\n';

    if (bytes_recv_from_server == requested_content_length) {
      std::cout << "CacheServer: File "<< this->m_fileToRequest << " received with size " << requested_content_length << '\n';
      this->m_fileSizes[this->m_fileToRequest] = requested_content_length;

      FinishedIncomingDataFromServer(m_socket_usr, m_activeRecvString);
    }
  }
}

void DashFakeVirtualClientSocket::FinishedIncomingDataFromServer(Ptr<Socket> socket, std::string data)
{
  fprintf(stderr, "CacheServer responding chunk %s to the server\n", this->m_fileToRequest.c_str());

  long filesize = GetFileSize(this->m_fileToRequest);

  // Create HTTP 1.1 compatible request
  std::stringstream replySS;
  replySS << "HTTP/1.1 200 OK" << CRLF; // OR HTTP/1.1 404 Not Found
  replySS << "Content-Type: text/xml; charset=utf-8" << CRLF; // e.g., when sending the MPD
  replySS << "Content-Length: " << filesize << CRLF;
  replySS << CRLF;

  std::string replyString = replySS.str();

  // this->m_fileToRequest = filename;
  m_is_first_packet = true;

  uint8_t* buffer = (uint8_t*)replyString.c_str();
  AddBytesToTransmit(buffer, replyString.length());

  fprintf(stderr, "VirtualCacheServer(%ld): Generating virtual payload with size %ld ...\n", (long int) node->GetId(), filesize);

  // now append the virtual payload data
  uint8_t tmp[4096];
  for (int i = 0; i < 4096; i++) {
    tmp[i] = (uint8_t)rand();
  }

  int cnt = 0;
  while (cnt < filesize) {
    if (cnt + 4096 < filesize) {
      AddBytesToTransmit(tmp, 4096);
    } else {
      AddBytesToTransmit(tmp, filesize - cnt);
    }
    cnt += 4096;
  }

  std::cout << m_currentBytesTx << " " << m_totalBytesToTx << " " << filesize << " " << m_bytesToTransmit.size() << '\n';
  // getchar();
  //tOMORROW IMPLEMENT HandleReadyToTransmit
  m_currentBytesTx = 0;
  m_totalBytesToTx = m_bytesToTransmit.size();
  while (m_currentBytesTx < m_totalBytesToTx)
  {
    uint32_t remainingBytes = m_totalBytesToTx - m_currentBytesTx;

    if (remainingBytes > 2860)
      remainingBytes = 2860;

    remainingBytes = std::min(remainingBytes, m_socket_usr->GetTxAvailable ());

    Ptr<Packet> replyPacket;

    uint8_t* buffer = (uint8_t*) &((this->m_bytesToTransmit)[m_currentBytesTx]);
    replyPacket = Create<Packet> (buffer, remainingBytes);

    int amountSent;
    amountSent = m_socket_usr->Send (replyPacket);
    if (amountSent <= 0) {
      return;
    }

    m_currentBytesTx += amountSent;
  }

}

void DashFakeVirtualClientSocket::IncomingDataFromUser(Ptr<Socket> socket)
{
  if (this->m_bytesToTransmit.size() != 0) {
    this->m_bytesToTransmit.clear();
  }

  Ptr<Packet> packet;
  Address from;

  m_activeRecvString = "";

  while (packet = socket->RecvFrom (from))
  {
    packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();

    // PARSE PACKET
    size_t packet_size = packet->CopyData(_tmpbuffer, packet->GetSize());
    _tmpbuffer[packet_size] = '\0';

    // check if packet in m_activePackets
    if (bytes_recv == 0) {
      m_activeRecvString = std::string((char*)_tmpbuffer);
      bytes_recv += packet_size;
    } else {
      m_activeRecvString += std::string((char*)_tmpbuffer);
      bytes_recv += packet_size;
    }

    if (string_ends_width(m_activeRecvString, std::string(CRLF))) {
      m_currentBytesTx = 0;
      m_totalBytesToTx = 0;
      break;
    }
  }

  FinishedIncomingDataFromUser(socket, m_activeRecvString);
}

void DashFakeVirtualClientSocket::FinishedIncomingDataFromUser(Ptr<Socket> socket, std::string data)
{
  fprintf(stderr, "VirtualCacheServer(%ld)::FinishedIncomingDataFromUser(socket,data=str(%ld))\n", m_socket_id, data.length());
  // getchar();
  std::string filename = ParseHTTPHeader(data);

  if (m_fileSizes.find(filename) != m_fileSizes.end()) {

    fprintf(stderr, "CacheServer(%ld): Opening \n", m_socket_id);

    long filesize = GetFileSize(filename);

    // Create a proper header
    std::stringstream replySS;
    replySS << "HTTP/1.1 200 OK" << CRLF; // OR HTTP/1.1 404 Not Found
    replySS << "Content-Type: text/xml; charset=utf-8" << CRLF; // e.g., when sending the MPD
    replySS << "Content-Length: " << filesize << CRLF;
    replySS << CRLF;

    // fprintf(stderr, "Replying with header:\n%s\n", replySS.str().c_str());

    std::string replyString = replySS.str();
    uint8_t* buffer = (uint8_t*)replyString.c_str();
    AddBytesToTransmit(buffer,replyString.length());

    // now append the virtual payload data
    uint8_t tmp[4096];

    if (string_ends_width(filename, ".mpd.gz")) {
      // std::string bytes_memory = mpdFile;
      AddBytesToTransmit((const uint8_t*)mpdFile.c_str(),mpdFile.length());
    } else {

      fprintf(stderr, "VirtualCacheServer(%ld): Generating virtual payload with size %ld ...\n", (long int) node->GetId(), filesize);

      for (int i = 0; i < 4096; i++) {
        tmp[i] = (uint8_t)rand();
      }

      int cnt = 0;

      while (cnt < filesize) {
        if (cnt + 4096 < filesize) {
          AddBytesToTransmit(tmp, 4096);
        } else {
          AddBytesToTransmit(tmp, filesize - cnt);
        }
        cnt += 4096;
      }
    }

    HandleReadyToTransmitFromUser(socket, socket->GetTxAvailable());
  } else {
    std::cout << "Cache requesting " << filename << " to the servr\n" << std::endl;
    bytes_recv_from_server = 0;

    this->m_fileToRequest = filename;
    m_is_first_packet = true;

    std::stringstream requestSS;
    requestSS << "GET " << m_fileToRequest << " HTTP/1.1" << CRLF;
    requestSS << "Host: " << m_hostName << CRLF;
    requestSS << "Accept: text/html,application/xml" << CRLF;
    requestSS << "User-Agent: ns-3 (applications/model/http-client.cc)" << CRLF;
    requestSS << "Accept-Encoding: identity" << CRLF; // no compression, gzip, etc... allowed
    requestSS << "Connection: keep-alive" << CRLF << CRLF;

    std::string requestString = requestSS.str();

    uint8_t* buffer = (uint8_t*)requestString.c_str();
    Ptr<Packet> p = Create<Packet> (buffer, requestString.length());

    m_socket_svr->Send (p);
  }
}

void DashFakeVirtualClientSocket::HandleReadyToTransmitFromUser(Ptr<Socket> socket, uint32_t txSize)
{
  std::cout << "HandleReadyToTransmit func from Cache Server " << m_currentBytesTx
            << " " <<  m_totalBytesToTx << '\n';

  while (m_currentBytesTx < m_totalBytesToTx) {

    uint32_t remainingBytes = m_totalBytesToTx - m_currentBytesTx;
    remainingBytes = std::min(remainingBytes, socket->GetTxAvailable ());

    Ptr<Packet> replyPacket;

    _tmpbuffer = (uint8_t*) &((this->m_bytesToTransmit)[m_currentBytesTx]);
    replyPacket = Create<Packet> (_tmpbuffer, remainingBytes);

    int amountSent = socket->Send (replyPacket);

    if(amountSent <= 0) {
      break;
    }
    m_currentBytesTx += amountSent;
  }

  std::cout << "Total Bytes " << m_currentBytesTx << " Transmitted " << '\n';
  this->m_bytesToTransmit.clear();
}

void DashFakeVirtualClientSocket::AskSegmentForServer(std::string data)
{
  fprintf(stderr, "Cache requesting to the servr\n");

  m_is_first_packet = true;

  uint8_t* buffer = (uint8_t*)data.c_str();
  Ptr<Packet> p = Create<Packet> (buffer, data.length());

  m_socket_svr->Send (p);
}

std::string DashFakeVirtualClientSocket::ParseHTTPHeader(std::string data)
{
  const char* cBuffer = data.c_str();

  char needle1[5];
  strcpy(needle1, "GET ");
  needle1[4] = '\0';

  char needle2[2];
  needle2[0] = ' ';
  needle2[1] = '\0';

  // find the name that was requested
  const char* p1 = strstr(cBuffer, needle1);
  int pos1 = p1-cBuffer+4;
  const char* p2 = strstr(& (cBuffer[pos1]), needle2);


  int pos2 = p2 - &cBuffer[pos1];

  char actualFileName[256];

  strncpy(actualFileName, &cBuffer[pos1], pos2);
  actualFileName[pos2] = '\0';

  std::string sFilename(actualFileName);

  if (data.find("Connection: keep-alive") != std::string::npos) {
    this->m_keep_alive = true;
  }

  return sFilename;
}

uint32_t DashFakeVirtualClientSocket::ParseResponseHeader(const uint8_t* buffer, size_t len, int* realStatusCode, unsigned int* contentLength)
{
  /*
  HTTP/1.1 200 OKCRLF
  Content-Type: text/xml; charset=utf-8CRLF
  Content-Length: {len}CRLFCRLF;
  */

  fprintf(stderr, "CacheServerClient(%d): Parsing Response Header of length %ld\n", node->GetId(), len);
  const char* strbuffer = (const char*) buffer;

  //fprintf(stderr, "header=\n%s\n", buffer);

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
        fprintf(stderr, "CacheServerClient(%d): ParseHeader: Status Code 404, not found!\n", node->GetId());
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
    fprintf(stderr, "[CacheServer] Not sure what this response header means\n");
    fprintf(stderr, "[CacheServer] Result=%s\n", strbuffer);
  }

  return 0;
}

void DashFakeVirtualClientSocket::AddBytesToTransmit(const uint8_t* buffer, uint32_t size)
{
  std::copy(buffer, buffer+size, std::back_inserter(this->m_bytesToTransmit));
  this->m_totalBytesToTx += size;
}

long DashFakeVirtualClientSocket::GetFileSize(std::string filename)
{
  return m_fileSizes[filename];
}

}
