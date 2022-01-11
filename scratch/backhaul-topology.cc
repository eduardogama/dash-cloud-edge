#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include <ns3/mobility-module.h>
#include <ns3/buildings-module.h>

#include <map>
#include <list>

#include "utils.h"


using namespace std;
using namespace ns3;


extern string GetCurrentWorkingDir(void);

NS_LOG_COMPONENT_DEFINE ("BackhaulTopologyScenario");


static string Ipv4AddressToString (Ipv4Address ad) {
	ostringstream oss;
	ad.Print (oss);
	return oss.str ();
}

bool AreOverlapping (Box a, Box b) {
    return !((a.xMin > b.xMax) || (b.xMin > a.xMax) || (a.yMin > b.yMax) || (b.yMin > a.yMax));
}

class FemtocellBlockAllocator
{
public:
    FemtocellBlockAllocator (Box area, uint32_t nApartmentsX, uint32_t nFloors);

    void Create(uint32_t n);
    void Create();

private:
    bool OverlapsWithAnyPrevious (Box box);
    Box m_area; ///< Area
    uint32_t m_nApartmentsX; ///< X apartments
    uint32_t m_nFloors; ///< number of floors
    list<Box> m_previousBlocks; ///< previous bocks
    double m_xSize; ///< X size
    double m_ySize; ///< Y size
    Ptr<UniformRandomVariable> m_xMinVar; ///< X minimum variance
    Ptr<UniformRandomVariable> m_yMinVar; ///< Y minimum variance
};

FemtocellBlockAllocator::FemtocellBlockAllocator (Box area, uint32_t nApartmentsX, uint32_t nFloors)
  : m_area (area),
    m_nApartmentsX (nApartmentsX),
    m_nFloors (nFloors),
    m_xSize (nApartmentsX*10 + 20),
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
        Create ();
}

void FemtocellBlockAllocator::Create ()
{
    Box box;
    uint32_t attempt = 0;
    do
    {
        NS_ASSERT_MSG (attempt < 100, "Too many failed attempts to position apartment block. Too many blocks? Too small area?");
        box.xMin = m_xMinVar->GetValue ();
        box.xMax = box.xMin + m_xSize;
        box.yMin = m_yMinVar->GetValue ();
        box.yMax = box.yMin + m_ySize;
        ++attempt;
    }
    while (OverlapsWithAnyPrevious (box));

    NS_LOG_LOGIC ("allocated non overlapping block " << box);
    m_previousBlocks.push_back (box);
    Ptr<GridBuildingAllocator>  gridBuildingAllocator;
    gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
    gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (1));
    gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (10*m_nApartmentsX));
    gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (10*2));
    gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (10));
    gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (10));
    gridBuildingAllocator->SetAttribute ("Height", DoubleValue (3*m_nFloors));
    gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (m_nApartmentsX));
    gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (2));
    gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (m_nFloors));
    gridBuildingAllocator->SetAttribute ("MinX", DoubleValue (box.xMin + 10));
    gridBuildingAllocator->SetAttribute ("MinY", DoubleValue (box.yMin + 10));
    gridBuildingAllocator->Create (2);
}

bool FemtocellBlockAllocator::OverlapsWithAnyPrevious (Box box)
{
  for (std::list<Box>::iterator it = m_previousBlocks.begin (); it != m_previousBlocks.end (); ++it) {
      if (AreOverlapping (*it, box)) {
          return true;
        }
    }
  return false;
}


int main(int argc, char *argv[])
{
    NetworkTopology network;
    string RepresentationType = "netflix";
    string hasAlgorithm = "hybrid";
    unsigned dst_server = 15;
    unsigned n_clients = 1;
    unsigned numberOfAps = 0;
    int stopTime = 30;
    int seed = 0;


    CommandLine cmd;

    //default parameters
    cmd.AddValue("RepresentationType", "Input representation type name", RepresentationType);
    cmd.AddValue("stopTime", "The time when the clients will stop requesting segments", stopTime);
    cmd.AddValue("HASLogic", "Adaptation Logic to Use.", hasAlgorithm);
    cmd.AddValue("seed", "Seed experiment.", seed);
    cmd.AddValue("Client", "Number of clients per AP.", n_clients);

    cmd.Parse(argc, argv);


    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1600));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));

    ReadTopology("content/scenario/backhaul_link", "content/scenario/backhaul_nodes", network);

    NS_LOG_INFO("Create Nodes");

    string AdaptationLogicToUse = has_algorithm(hasAlgorithm);
    string dir = CreateDir("../btree-" + hasAlgorithm + "-" + to_string(n_clients) + "-" + to_string(seed));
    string filePath = dir + "/Troughput_" + to_string(seed) + "_";

    AdaptationLogicToUse = "dash::player::" + AdaptationLogicToUse;


    NodeContainer nodes; // Declare nodes objects
    NodeContainer clients;
    map<pair<string,int>, string> serverTableList;

    nodes.Create(network.getNodes().size());

    Ptr<Monitoring> monitor = CreateObject<Monitoring>();
    monitor->setStepsTime(2);
    monitor->setToRedirect(true);
    monitor->setPathFile(filePath);
    monitor->setNodes(&nodes);

    Ptr<BigTable> bigtable = CreateObject<BigTable>();
    bigtable->setClientContainers(&clients);
    bigtable->setServerTable(&serverTableList);

    Ptr<ControllerMain> ctrlapp = CreateObject<ControllerMain>();
    nodes.Get(dst_server)->AddApplication(ctrlapp);
    ctrlapp->Setup(Ipv4Address::GetAny(), 1317, "ILPSolution");
    ctrlapp->setNodeContainers(&nodes);
    ctrlapp->SetStartTime(Seconds(0.0));
    ctrlapp->SetStopTime(Seconds(20.));

    monitor->setController(ctrlapp);
    ctrlapp->setBigTable(bigtable);
    ctrlapp->setServerTable(&serverTableList);

    cout << "Node size = " << network.getNodes().size() << endl;
    for (unsigned i = 0; i < network.getNodes().size(); i += 1) {
        ostringstream ss;
        ss << network.getNodes().at(i)->getId();
        Names::Add(network.getNodes().at(i)->getType() + ss.str(), nodes.Get(network.getNodes().at(i)->getId()));
        cout << "Node name " << i << " = " << network.getNodes().at(i)->getType() << endl;
    }

    // Later we add IP Addresses
    NS_LOG_INFO("Assign IP Addresses.");
    InternetStackHelper internet;

    fprintf(stderr, "Installing Internet Stack\n");
    // Now add ip/tcp stack to all nodes.
    internet.Install(nodes);

    // create p2p links
    Ipv4AddressHelper address;
    address.SetBase ("10.0.0.0", "255.255.255.0");
    PointToPointHelper p2p;

    for (unsigned int i = 0; i < network.getLinks().size(); i += 1) {
        int srcnode = network.getLinks().at(i)->getSrcId();
        int dstnode = network.getLinks().at(i)->getDstId();

        double datarate = network.getLinks().at(i)->getRate();

        p2p.SetDeviceAttribute("DataRate", DataRateValue(datarate)); // Mbit/s

        // And then install devices and channels connecting our topology
        NetDeviceContainer deviceContainer;
        deviceContainer = p2p.Install(nodes.Get(srcnode), nodes.Get(dstnode));

        address.Assign(deviceContainer);
        address.NewNetwork();

        Ptr<Ipv4> srcipv4 = nodes.Get(srcnode)->GetObject<Ipv4>();
        Ptr<Ipv4> dstipv4 = nodes.Get(dstnode)->GetObject<Ipv4>();

        int same_bcst = -1;
        for (size_t i_ipv4 = 1; i_ipv4 < dstipv4->GetNInterfaces(); i_ipv4++) {
            same_bcst = srcipv4->GetInterfaceForPrefix(dstipv4->GetAddress(i_ipv4, 0).GetLocal(), dstipv4->GetAddress(i_ipv4, 0).GetMask());
            if (same_bcst != -1) {
                break;
            }
        }

        string stripv4 = Ipv4AddressToString(srcipv4->GetAddress(same_bcst, 0).GetBroadcast());

        monitor->setLinkMap(stripv4, 0);
        monitor->setLinkCapacityMap(stripv4, srcnode, dstnode, datarate/1000000);
        monitor->CreateFile(stripv4, srcnode, dstnode);
    }

    //Store IP adresses
    string addr_file = dir + "/addresses";
    ofstream out_addr_file(addr_file.c_str());
    for (unsigned int i = 0; i < nodes.GetN(); i++) {
        Ptr<Node> n = nodes.Get(i);
        Ptr<Ipv4> ipv4 = n->GetObject<Ipv4>();
        for (uint32_t l = 1; l < ipv4->GetNInterfaces(); l++) {
            out_addr_file << i << " " << ipv4->GetAddress(l, 0).GetLocal() << endl;
        }
    }
    out_addr_file.flush();
    out_addr_file.close();

    // %%%%%%%%%%%% Set up the Mobility Position
    uint32_t nMacroEnbSites = 5;
    uint32_t nMacroEnbSitesX = 2;

    double interSiteDistance = 500;
    double areaMarginFactor = 0.5;
    double ueZ = 1.5;

    Box macroUeBox;
    if (nMacroEnbSites > 0) {
        uint32_t currentSite = nMacroEnbSites - 1;
        uint32_t biRowIndex = (currentSite / (nMacroEnbSitesX + nMacroEnbSitesX + 1));
        uint32_t biRowRemainder = currentSite % (nMacroEnbSitesX + nMacroEnbSitesX + 1);
        uint32_t rowIndex = biRowIndex*2 + 1;

        if (biRowRemainder >= nMacroEnbSitesX) {
            ++rowIndex;
        }

        uint32_t nMacroEnbSitesY = rowIndex;

        NS_LOG_LOGIC ("nMacroEnbSitesY = " << nMacroEnbSitesY);

        macroUeBox = Box (-areaMarginFactor*interSiteDistance,
                          (nMacroEnbSitesX + areaMarginFactor)*interSiteDistance,
                          -areaMarginFactor*interSiteDistance,
                          (nMacroEnbSitesY -1)*interSiteDistance*sqrt (0.75) + areaMarginFactor*interSiteDistance,
                          ueZ, ueZ);
    }

    uint32_t nApartmentsX = 10;
    uint32_t nFloors = 1;
    uint32_t nBlocks = 1;
    FemtocellBlockAllocator blockAllocator (macroUeBox, nApartmentsX, nFloors);
    blockAllocator.Create (nBlocks);

    return 0;
}
