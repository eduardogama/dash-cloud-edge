#include "gta-dash.h"

#include "ns3/log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


namespace ns3 {


NS_LOG_COMPONENT_DEFINE ("GTADash");

NS_OBJECT_ENSURE_REGISTERED (GTADash);


TypeId GTADash::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::GTADash")
        .SetParent<Application> ()
        .SetGroupName("Applications")
        .AddConstructor<GTADash> ();
        
        return tid;
}

GTADash::GTADash()
{
	NS_LOG_FUNCTION (this);

    fprintf(stderr, "GTA Dash Client Created!\n");
}

GTADash::~GTADash()
{
	NS_LOG_FUNCTION (this);
}

}

