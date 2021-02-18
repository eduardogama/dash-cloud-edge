
#ifndef FEMTOCELL_HH_
#define FEMTOCELL_HH_

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/config-store-module.h>
#include <ns3/buildings-module.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/applications-module.h>
#include <ns3/log.h>
#include <iomanip>
#include <ios>
#include <string>
#include <sstream>
#include <vector>

using namespace ns3;

bool AreOverlapping (Box a, Box b)
{
	return !((a.xMin > b.xMax) || (b.xMin > a.xMax) || (a.yMin > b.yMax) || (b.yMin > a.yMax));
}

/**
 * Class that takes care of installing blocks of the
 * buildings in a given area. Buildings are installed in pairs
 * as in dual stripe scenario.
 */
class FemtocellBlockAllocator
{
	public:
		/**
		* Constructor
		* \param area the total area
		* \param nApartmentsX the number of apartments in the X direction
		* \param nFloors the number of floors
		*/
		FemtocellBlockAllocator (Box area, uint32_t nApartmentsX, uint32_t nFloors);
		/**
		* Function that creates building blocks.
		* \param n the number of blocks to create
		*/
		void Create (uint32_t n);
		/// Create function
		void Create ();

	private:
		/**
		* Function that checks if the box area is overlapping with some of previously created building blocks.
		* \param box the area to check
		* \returns true if there is an overlap
		*/
		bool OverlapsWithAnyPrevious (Box box);
		Box m_area; ///< Area
		uint32_t m_nApartmentsX; ///< X apartments
		uint32_t m_nFloors; ///< number of floors
		std::list<Box> m_previousBlocks; ///< previous bocks
		double m_xSize; ///< X size
		double m_ySize; ///< Y size
		Ptr<UniformRandomVariable> m_xMinVar; ///< X minimum variance
		Ptr<UniformRandomVariable> m_yMinVar; ///< Y minimum variance
};



FemtocellBlockAllocator::FemtocellBlockAllocator (Box area, uint32_t nApartmentsX, uint32_t nFloors)
    : m_area (area),
      m_nApartmentsX (nApartmentsX),
      m_nFloors (nFloors),
      m_xSize (nApartmentsX * 10 + 20),
      m_ySize (70)
{
	m_xMinVar = CreateObject<UniformRandomVariable> ();
	m_xMinVar->SetAttribute ("Min", DoubleValue (area.xMin));
	m_xMinVar->SetAttribute ("Max", DoubleValue (area.xMax - m_xSize));
	m_yMinVar = CreateObject<UniformRandomVariable> ();
	m_yMinVar->SetAttribute ("Min", DoubleValue (area.yMin));
	m_yMinVar->SetAttribute ("Max", DoubleValue (area.yMax - m_ySize));
}

void FemtocellBlockAllocator::Create (uint32_t n)
{
	for (uint32_t i = 0; i < n; ++i)
	{
		Create ();
	}
}

void FemtocellBlockAllocator::Create ()
{

	Box box;
	uint32_t attempt = 0;
	do
	{
//		NS_ASSERT_MSG (attempt < 100, "Too many failed attempts to position apartment block. Too many blocks? Too small area?");
		box.xMin = m_xMinVar->GetValue ();
		box.xMax = box.xMin + m_xSize;
		box.yMin = m_yMinVar->GetValue ();
		box.yMax = box.yMin + m_ySize;
		++attempt;
	} while (OverlapsWithAnyPrevious(box));

//	NS_LOG_LOGIC ("allocated non overlapping block " << box);
	m_previousBlocks.push_back (box);
	Ptr<GridBuildingAllocator> gridBuildingAllocator;
	gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
	gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (1));
	gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (10 * m_nApartmentsX));
	gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (10 * 2));
	gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (10));
	gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (10));
	gridBuildingAllocator->SetAttribute ("Height", DoubleValue (3 * m_nFloors));
	gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (m_nApartmentsX));
	gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (2));
	gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (m_nFloors));
	gridBuildingAllocator->SetAttribute ("MinX", DoubleValue (box.xMin + 10));
	gridBuildingAllocator->SetAttribute ("MinY", DoubleValue (box.yMin + 10));
	gridBuildingAllocator->Create (2);
}

bool FemtocellBlockAllocator::OverlapsWithAnyPrevious (Box box)
{
	for (std::list<Box>::iterator it = m_previousBlocks.begin (); it != m_previousBlocks.end (); ++it)
	{
		if (AreOverlapping (*it, box))
		{
			return true;
		}
	}
	return false;
}

void PrintGnuplottableBuildingListToFile (std::string filename)
{
	std::ofstream outFile;
	outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
	if (!outFile.is_open ())
	{
//		NS_LOG_ERROR ("Can't open file " << filename);
		return;
	}
	uint32_t index = 0;
	for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
	{
		++index;
		Box box = (*it)->GetBoundaries ();
		outFile << "set object " << index << " rect from " << box.xMin << "," << box.yMin << " to "
				<< box.xMax << "," << box.yMax << " front fs empty " << std::endl;
	}
}

void
PrintGnuplottableUeListToFile (std::string filename)
{
	std::ofstream outFile;
	outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
	if (!outFile.is_open ())
	{
//		NS_LOG_ERROR ("Can't open file " << filename);
		return;
	}
	for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
	{
		Ptr<Node> node = *it;
		int nDevs = node->GetNDevices ();
		for (int j = 0; j < nDevs; j++)
		{
			Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject<LteUeNetDevice> ();
			if (uedev)
			{
				Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
				outFile << "set label \"" << uedev->GetImsi () << "\" at " << pos.x << "," << pos.y
						<< " left font \"Helvetica,4\" textcolor rgb \"grey\" front point pt 1 ps 0.3 lc rgb \"grey\" offset 0,0"
						<< std::endl;
			}
		}
	}
}

void
PrintGnuplottableEnbListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
//      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteEnbNetDevice> enbdev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
          if (enbdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << enbdev->GetCellId ()
                      << "\" at "<< pos.x << "," << pos.y
                      << " left font \"Helvetica,4\" textcolor rgb \"white\" front  point pt 2 ps 0.3 lc rgb \"white\" offset 0,0"
                      << std::endl;
            }
        }
    }
}


static ns3::GlobalValue g_nBlocks ("nBlocks", "Number of femtocell blocks", ns3::UintegerValue (1),
                                   ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue
    g_nApartmentsX ("nApartmentsX", "Number of apartments along the X axis in a femtocell block",
                    ns3::UintegerValue (10), ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_nFloors ("nFloors", "Number of floors", ns3::UintegerValue (1),
                                   ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_nMacroEnbSites ("nMacroEnbSites", "How many macro sites there are",
                                          ns3::UintegerValue (3),
                                          ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue
    g_nMacroEnbSitesX ("nMacroEnbSitesX",
                       "(minimum) number of sites along the X-axis of the hex grid",
                       ns3::UintegerValue (1), ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_interSiteDistance ("interSiteDistance",
                                             "min distance between two nearby macro cell sites",
                                             ns3::DoubleValue (500),
                                             ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue
    g_areaMarginFactor ("areaMarginFactor",
                        "how much the UE area extends outside the macrocell grid, "
                        "expressed as fraction of the interSiteDistance",
                        ns3::DoubleValue (0.5), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_macroUeDensity ("macroUeDensity",
                                          "How many macrocell UEs there are per square meter",
                                          ns3::DoubleValue (0.00002),
                                          ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_homeEnbDeploymentRatio ("homeEnbDeploymentRatio",
                                                  "The HeNB deployment ratio as per 3GPP R4-092042",
                                                  ns3::DoubleValue (0.2),
                                                  ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_homeEnbActivationRatio ("homeEnbActivationRatio",
                                                  "The HeNB activation ratio as per 3GPP R4-092042",
                                                  ns3::DoubleValue (0.5),
                                                  ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue
    g_homeUesHomeEnbRatio ("homeUesHomeEnbRatio",
                           "How many (on average) home UEs per HeNB there are in the simulation",
                           ns3::DoubleValue (1.0), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_macroEnbTxPowerDbm ("macroEnbTxPowerDbm",
                                              "TX power [dBm] used by macro eNBs",
                                              ns3::DoubleValue (46.0),
                                              ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_homeEnbTxPowerDbm ("homeEnbTxPowerDbm", "TX power [dBm] used by HeNBs",
                                             ns3::DoubleValue (20.0),
                                             ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_macroEnbDlEarfcn ("macroEnbDlEarfcn", "DL EARFCN used by macro eNBs",
                                            ns3::UintegerValue (100),
                                            ns3::MakeUintegerChecker<uint16_t> ());
static ns3::GlobalValue g_homeEnbDlEarfcn ("homeEnbDlEarfcn", "DL EARFCN used by HeNBs",
                                           ns3::UintegerValue (100),
                                           ns3::MakeUintegerChecker<uint16_t> ());
static ns3::GlobalValue g_macroEnbBandwidth ("macroEnbBandwidth",
                                             "bandwidth [num RBs] used by macro eNBs",
                                             ns3::UintegerValue (25),
                                             ns3::MakeUintegerChecker<uint16_t> ());
static ns3::GlobalValue g_homeEnbBandwidth ("homeEnbBandwidth", "bandwidth [num RBs] used by HeNBs",
                                            ns3::UintegerValue (25),
                                            ns3::MakeUintegerChecker<uint16_t> ());
static ns3::GlobalValue g_simTime ("simTime", "Total duration of the simulation [s]",
                                   ns3::DoubleValue (0.25), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue
    g_generateRem ("generateRem",
                   "if true, will generate a REM and then abort the simulation;"
                   "if false, will run the simulation normally (without generating any REM)",
                   ns3::BooleanValue (false), ns3::MakeBooleanChecker ());
static ns3::GlobalValue
    g_remRbId ("remRbId",
               "Resource Block Id of Data Channel, for which REM will be generated;"
               "default value is -1, what means REM will be averaged from all RBs of "
               "Control Channel",
               ns3::IntegerValue (-1), MakeIntegerChecker<int32_t> ());
static ns3::GlobalValue
    g_epc ("epc",
           "If true, will setup the EPC to simulate an end-to-end topology, "
           "with real IP applications over PDCP and RLC UM (or RLC AM by changing "
           "the default value of EpsBearerToRlcMapping e.g. to RLC_AM_ALWAYS). "
           "If false, only the LTE radio access will be simulated with RLC SM. ",
           ns3::BooleanValue (false), ns3::MakeBooleanChecker ());
static ns3::GlobalValue
    g_epcDl ("epcDl",
             "if true, will activate data flows in the downlink when EPC is being used. "
             "If false, downlink flows won't be activated. "
             "If EPC is not used, this parameter will be ignored.",
             ns3::BooleanValue (true), ns3::MakeBooleanChecker ());
static ns3::GlobalValue
    g_epcUl ("epcUl",
             "if true, will activate data flows in the uplink when EPC is being used. "
             "If false, uplink flows won't be activated. "
             "If EPC is not used, this parameter will be ignored.",
             ns3::BooleanValue (true), ns3::MakeBooleanChecker ());
static ns3::GlobalValue
    g_useUdp ("useUdp",
              "if true, the UdpClient application will be used. "
              "Otherwise, the BulkSend application will be used over a TCP connection. "
              "If EPC is not used, this parameter will be ignored.",
              ns3::BooleanValue (true), ns3::MakeBooleanChecker ());
static ns3::GlobalValue g_useDash ("useDash",
                                   "if true, the DashClient application will be used. "
                                   "If EPC is not used, this parameter will be ignored.",
                                   ns3::BooleanValue (true), ns3::MakeBooleanChecker ());
static ns3::GlobalValue g_fadingTrace ("fadingTrace",
                                       "The path of the fading trace (by default no fading trace "
                                       "is loaded, i.e., fading is not considered)",
                                       ns3::StringValue (""), ns3::MakeStringChecker ());
static ns3::GlobalValue g_numBearersPerUe ("numBearersPerUe",
                                           "How many bearers per UE there are in the simulation",
                                           ns3::UintegerValue (1),
                                           ns3::MakeUintegerChecker<uint16_t> ());
static ns3::GlobalValue g_srsPeriodicity ("srsPeriodicity",
                                          "SRS Periodicity (has to be at least "
                                          "greater than the number of UEs per eNB)",
                                          ns3::UintegerValue (80),
                                          ns3::MakeUintegerChecker<uint16_t> ());
static ns3::GlobalValue
    g_outdoorUeMinSpeed ("outdoorUeMinSpeed",
                         "Minimum speed value of macro UE with random waypoint model [m/s].",
                         ns3::DoubleValue (0.0), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue
    g_outdoorUeMaxSpeed ("outdoorUeMaxSpeed",
                         "Maximum speed value of macro UE with random waypoint model [m/s].",
                         ns3::DoubleValue (0.0), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue
    g_targetDt ("targetDt",
                "The target time difference between receiving and playing a frame. [s].",
                ns3::DoubleValue (35.0), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_window ("window", "The window for measuring the average throughput. [s].",
                                  ns3::DoubleValue (10.0), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue
    g_algorithms ("algorithms",
                  "The adaptation algorithms used. It can be a comma seperated list of"
                  "protocolos, such as 'ns3::FdashClient,ns3::OsmpClient'."
                  "You may find the list of available algorithms in src/dash/model/algorithms",
                  ns3::StringValue ("ns3::FdashClient"), ns3::MakeStringChecker ());
static ns3::GlobalValue g_bufferSpace ("bufferSpace",
                                       "The space in bytes that is used for buffering the video",
                                       ns3::UintegerValue (30000000),
                                       MakeUintegerChecker<uint32_t> ());


#endif // FEMTOCELL_HH_
