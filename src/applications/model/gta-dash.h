#ifndef GTA_DASH
#define GTA_DASH


#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include "ns3/tcp-socket.h"


#include <string>


namespace ns3 {

class GTADash : public Application
{
	public:
	    
	    static TypeId GetTypeId (void);
	    
		GTADash();
		virtual ~GTADash();


		void MapSSIMplus();
		

	private:
		std::string id;
		
};

} // namespace ns3

#endif /* GTA_DASH */
