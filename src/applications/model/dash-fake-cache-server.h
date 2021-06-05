#ifndef DASH_FAKE_CACHE_SERVER_APPLICATION_H
#define DASH_FAKE_CACHE_SERVER_APPLICATION_H


#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"

#include <map>
#include <vector>

#include "dash-fake-virtual-clientsocket.h"


#define CRLF "\r\n"

namespace ns3 {

class Address;
class RandomVariableStream;
class Socket;


class DASHFakeCacheServer : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  DASHFakeCacheServer ();
  virtual ~DASHFakeCacheServer ();

  void TxTrace(Ptr<Packet const> packet);
  void RxTrace(Ptr<Packet const> packet);

  virtual void StartApplication (void);
  virtual void StopApplication (void);


protected:
  bool ConnectionRequested (Ptr<Socket> socket, const Address& address);
  void ConnectionAccepted (Ptr<Socket> socket, const Address& address);

  uint64_t RegisterSocket(Ptr<Socket> socket);

  void FinishedCallback(uint64_t socket_id);

  std::string ImportDASHRepresentations();

protected:
  uint64_t m_bytes_recv;
  uint64_t m_bytes_sent;

  uint64_t m_last_bytes_recv;
  uint64_t m_last_bytes_sent;

private:
  std::map<Ptr<Socket>, uint64_t> m_activeSockets;
  std::map<uint64_t, DashFakeVirtualClientSocket*> m_activeClients;

  uint64_t m_lastSocketID;

  uint16_t m_port; //!< Port on which we listen for incoming packets.
  Ptr<Socket> m_socket; //!< IPv4 Socket

  std::string m_mpdDirectory;
  std::string m_mpdMetaDataFiles;
  std::string m_metaDataContentDirectory;
  std::string m_hostName;
  std::string m_remoteCDN;
  Address m_listeningAddress;

  std::map<std::string, long> m_fileSizes;
  std::string mpdFile;
};

}

#endif //DASH_FAKE_CACHE_SERVER_APPLICATION_H
