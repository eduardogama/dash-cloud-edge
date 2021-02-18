#ifndef AGGREGATION_APPLICATION_H
#define AGGREGATION_APPLICATION_H


#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"

#include "ns3/traced-callback.h"
#include "ns3/tcp-socket.h"

#include "ns3/socket.h"

#include <string>
#include <map>


namespace ns3 {

class Cluster;

class AggregationApplication : public Application
{
	public:
	    
	    static TypeId GetTypeId (void);
	    
		AggregationApplication();
		virtual ~AggregationApplication();

        void TxTrace(Ptr<Packet const> packet);
        void RxTrace(Ptr<Packet const> packet);
        
//		void MapSSIMplus();

		void Start();
		void AddUser(uint64_t user);
		
		bool isAlive();		
		
		bool ConnectionRequested (Ptr<Socket> socket, const Address& address);
		void ConnectionAccepted (Ptr<Socket> socket, const Address& address);

		void HandleIncomingData(Ptr<Socket> socket);

		void HandleReadyToTransmit(Ptr<Socket> socket, uint32_t txSize);

		void ConnectionClosedNormal (Ptr<Socket> socket);
		void ConnectionClosedError (Ptr<Socket> socket);

        /**
        * \brief Register this new socket and gets a new client ID for this socket, and register this socket
        */
        uint64_t RegisterSocket(Ptr<Socket> socket);

	private:
	
		uint64_t m_bytes_recv;
        uint64_t m_bytes_sent;
    
        std::map<Ptr<Socket> /* socket */, uint64_t /* socket id */  > m_activeSockets;

        std::map<uint64_t /* socket id */, Cluster* /* client_socket */ > m_activeClients;

        std::map<uint64_t /* socket id */, std::string /* packet buffer */ > m_activePackets;
	
		std::map<std::string, Cluster> clusters;
		
		std::string id;
		
		bool alive;
		
		Ptr<Socket> m_socket; //!< IPv4 Socket
	    std::string m_hostName;
		Address m_listeningAddress;
		
        uint16_t m_port; //!< Port on which we listen for incoming packets.
        
        uint64_t m_lastSocketID;
	
	protected:
        virtual void StartApplication (void);
        virtual void StopApplication (void);
};

class Cluster {
	public:
		Cluster();
		
		virtual ~Cluster();
	
	private:
		uint16_t id;	
};

} // namespace ns3

#endif /* AGGREGATION_APPLICATION_H */
