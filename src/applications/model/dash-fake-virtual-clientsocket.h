#ifndef DASH_FAKE_VIRTUAL_CLIENTSOCKET
#define DASH_FAKE_VIRTUAL_CLIENTSOCKET

#include "ns3/callback.h"
#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/string.h"
#include "ns3/tcp-socket.h"

#include <map>
#include <vector>
#include <stdio.h>


namespace ns3
{
class Socket;
class Address;


class DashFakeVirtualClientSocket
{
public:
  DashFakeVirtualClientSocket(std::map<std::string, long>& fileSizes, std::string& mpdFile, uint64_t socket_id, Ptr<Socket> socket, Ptr< Node > node, std::string m_remoteCDN, Callback<void, uint64_t> finished_callback);
  virtual ~DashFakeVirtualClientSocket();

  void TryEstablishConnection();

  void ConnectionComplete(Ptr<Socket> socket);
  void ConnectionFailed(Ptr<Socket> socket);

  void HandleReadyToTransmit(Ptr<Socket> socket, uint32_t txSize);
  void HandleReadyToTransmitFromUser(Ptr<Socket> socket, uint32_t txSize);
  void HandleIncomingData(Ptr<Socket> socket, Address from, Ptr<Packet> packet);

  void IncomingDataFromServer(Ptr<Socket> socket);
  void IncomingDataFromUser(Ptr<Socket> socket);

  void FinishedIncomingDataFromServer(Ptr<Socket> socket, std::string data);
  void FinishedIncomingDataFromUser(Ptr<Socket> socket, std::string data);

  long GetFileSize(std::string filename);
  std::string ParseHTTPHeader(std::string data);
  uint32_t ParseResponseHeader(const uint8_t* buffer, size_t len, int* realStatusCode, unsigned int* contentLength);

  void AskSegmentForServer(std::string data);

  void AddBytesToTransmit(const uint8_t* buffer, uint32_t size);

  std::string ImportDASHRepresentations();

protected:
  Callback<void, uint64_t> m_finished_callback;

protected:
  std::string m_outFile;
  uint8_t* _tmpbuffer;

  uint64_t m_socket_id;

  uint32_t bytes_recv;
  uint32_t bytes_sent;

  uint32_t bytes_recv_from_server;
  uint32_t bytes_sent_from_server;

  uint32_t m_totalBytesToTx;
  uint32_t m_currentBytesTx;

  bool m_is_shutdown;
  bool m_is_first_packet;

  Ptr<Node> node;
  Ptr<Socket> m_socket_svr;
  Ptr<Socket> m_socket_usr;
  Address m_peerAddress;
  uint16_t m_peerPort;

  std::vector<uint8_t> m_bytesToTransmit;


  std::string m_fileToRequest;
  std::string m_remoteCDN;
  std::string m_activeRecvString;

  std::map<std::string,long>& m_fileSizes;
  std::string& mpdFile;

  unsigned requested_content_length;
  bool m_keep_alive;
};

}

#endif //DASH_FAKE_VIRTUAL_CLIENTSOCKET
