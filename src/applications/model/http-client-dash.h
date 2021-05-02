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


#ifndef HTTP_CLIENT_DASH_APPLICATION_H
#define HTTP_CLIENT_DASH_APPLICATION_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include "ns3/tcp-socket.h"

#include "ns3/ipv4.h"



#define CRLF "\r\n"

using namespace std;
namespace ns3 {


class Socket;
class Packet;


class HttpClientDashApplication : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  HttpClientDashApplication();
  virtual ~HttpClientDashApplication();

  void setServerTableList (std::map<std::string, std::string> *serverTableList);
  string getServerTableList (std::string server);

protected:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
  * \brief set the remote address and port
  * \param ip remote IPv4 address
  * \param port remote port
  */
  void SetRemote (Ipv4Address ip, uint16_t port);
  /**
  * \brief set the remote address and port
  * \param ip remote IPv6 address
  * \param port remote port
  */
  void SetRemote (Ipv6Address ip, uint16_t port);
  /**
  * \brief set the remote address and port
  * \param ip remote IP address
  * \param port remote port
  */
  void SetRemote (Address ip, uint16_t port);

  /**
  * \brief Forces closing the socket if KeepAlive was set
  */
  void ForceCloseSocket();

  void TryEstablishConnection();

  void ConnectionComplete (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);
  void ConnectionClosedNormal (Ptr<Socket> socket);
  void ConnectionClosedError (Ptr<Socket> socket);

  virtual void OnFileReceived(unsigned status, unsigned length);

  bool endsWith(const std::string& s, const std::string& suffix);
  vector<string> split(const string& s, const string& delimiter);

  //=======================================================================================
  // PARALLEL CONNECTION WITH AGGREGATOR AGENT
  //=======================================================================================

  void AgentTryEstablishConnection();

  void AgentConnectionComplete (Ptr<Socket> socket);
  void AgentConnectionFailed (Ptr<Socket> socket);
  void AgentHandleRead (Ptr<Socket> socket);

private:
  /**
  * \brief Callback from Socket when ready to send a packet
  */
  virtual void OnReadySend (Ptr<Socket> localSocket, uint32_t txSpace);

  /**
  * \brief Sending an actual packet
  */
  virtual void DoSendGetRequest (Ptr<Socket> localSocket, uint32_t txSpace);

  /**
  * \brief Handle a packet reception.
  *
  * This function is called by lower layers.
  *
  * \param socket the socket the packet was received to.
  */
  void HandleRead (Ptr<Socket> socket);

  uint32_t ParseResponseHeader (const uint8_t* buffer, size_t len, int* statusCode, unsigned int* contentLength);



protected:

  uint32_t node_id;
  string   strNodeIpv4;

  TracedCallback<Ptr<ns3::Application>, std::string> m_downloadStartedTrace;
  TracedCallback<Ptr<ns3::Application>, std::string, long> m_headerReceivedTrace;
  TracedCallback<Ptr<ns3::Application>, std::string, double, long> m_downloadFinishedTrace;
  TracedCallback<Ptr<ns3::Application>, std::string, unsigned> m_currentStatsTrace;

  std::string m_fileToRequest;
  std::string m_hostName; //!< The hostname of the destiatnion server
  std::string m_outFile;

  bool m_finished_download;
  bool m_isMpd;

  bool m_active;
  bool m_keepAlive;

  unsigned requested_content_length;

  double lastDownloadBitrate;

  map<string, string> *serverTableList;

private:
  uint8_t* _tmpbuffer;

  Ptr<Socket> m_socket; //!< Socket
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port

  uint32_t m_tried_connecting;
  uint32_t m_success_connecting;
  uint32_t m_failed_connecting;

  uint32_t m_bytesRecv; //!< Number of bytes received
  uint32_t m_bytesSent; //!< Number of bytes sent
  uint32_t m_sent; //!< Counter for sent packets

  int64_t _start_time;
  int64_t _finished_time;

  bool m_sentGetRequest;
  bool m_is_first_packet;

  /// Callbacks for tracing the packet Tx events
  TracedCallback<Ptr<const Packet> > m_txTrace;


  //=======================================================================================
  // PARALLEL CONNECTION WITH AGGREGATOR AGENT
  //=======================================================================================

  Ptr<Socket> gta_socket;
};

}

#endif //HTTP_CLIENT_DASH_APPLICATION_H
