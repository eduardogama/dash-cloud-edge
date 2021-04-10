#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/dashplayer-tracer.h"
#include "ns3/node-throughput-tracer.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

#include "ns3/uinteger.h"
#include "ns3/netanim-module.h"

#include <map>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>

#include "ctrl-dash-main.h"
#include "utils.h"

#include "node-statistics.h"
#include "group-user.h"
#include "ns3/hash.h"

#include <ctime>

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("DashBTreeLinkCapacity");

int main (int argc, char *argv[])
{
  NetworkTopology network;
  std::map<string, string> serverTableList;
	unsigned n_ap = 0, n_clients = 1;
  int dst_server = 7;

  //Register packet receptions to calculate throughput
  NodeStatistics eCtrl = NodeStatistics(&network, 2, "./");

	string scenarioFiles = GetCurrentWorkingDir() + "/../content/scenario";
	string requestsFile = "requests";
	string DashTraceFile = "report.csv";
	string ServerThroughputTraceFile = "server_throughput.csv";
	string RepresentationType = "netflix";
	string AdaptationLogicToUse = "dash::player::RateAndBufferBasedAdaptationLogic";

	int stopTime = 30;
	int seed = 0;
	CommandLine cmd;

	//default parameters
	cmd.AddValue("DashTraceFile", "Filename of the DASH traces", DashTraceFile);
	cmd.AddValue("RepresentationType", "Input representation type name", RepresentationType);
  cmd.AddValue("stopTime", "The time when the clients will stop requesting segments", stopTime);
  cmd.AddValue("AdaptationLogicToUse", "Adaptation Logic to Use.", AdaptationLogicToUse);
  cmd.AddValue("seed", "Seed experiment.", seed);
  cmd.AddValue("Client", "Number of clients per AP.", n_clients);

	cmd.Parse (argc, argv);

  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (1600));

  ReadTopology(scenarioFiles + "/btree_l3_link", scenarioFiles + "/btree_l3_nodes", network);

	NS_LOG_INFO ("Create Nodes");

	NodeContainer nodes; // Declare nodes objects
  NodeContainer cache_nodes;
	nodes.Create(network.getNodes().size());
  cache_nodes.Create(network.getNodes().size());

	cout << "Node size = " << network.getNodes().size() << endl;
	for (unsigned int i = 0; i < network.getNodes().size(); i += 1) {
		ostringstream ss;
		ss << network.getNodes().at(i)->getId();
		Names::Add(network.getNodes().at(i)->getType() + ss.str(), nodes.Get( network.getNodes().at(i)->getId() ));
		cout << "Node name " << i << " = " << network.getNodes().at(i)->getType() << endl;
	}

	// Later we add IP Addresses
	NS_LOG_INFO("Assign IP Addresses.");
	InternetStackHelper internet;

	fprintf(stderr, "Installing Internet Stack\n");
	// Now add ip/tcp stack to all nodes.
	internet.Install(nodes);
  internet.Install(cache_nodes);

	// create p2p links
	vector<NetDeviceContainer> netDevices;
	Ipv4AddressHelper address;
	address.SetBase ("10.0.0.0", "255.255.255.0");
	PointToPointHelper p2p;

	for (unsigned int i = 0; i < network.getLinks().size(); i += 1) {
    int srcnode = network.getLinks().at(i)->getSrcId();
    int dstnode = network.getLinks().at(i)->getDstId();

    p2p.SetDeviceAttribute("DataRate", DataRateValue( network.getLinks().at(i)->getRate() )); // Mbit/s

		// And then install devices and channels connecting our topology
		NetDeviceContainer deviceContainer;
		deviceContainer = p2p.Install(nodes.Get(srcnode), nodes.Get(dstnode));

		address.Assign(deviceContainer);
		address.NewNetwork();
    netDevices.push_back(deviceContainer);


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
    eCtrl.setLinkMap(stripv4, 0);
    eCtrl.createTroughputFile(stripv4, srcnode, dstnode);
	}

	//Store IP adresses
	std::string addr_file = "addresses";
	ofstream out_addr_file(addr_file.c_str());
	for (unsigned int i = 0; i < cache_nodes.GetN(); i++) {
		Ptr<Node> n = cache_nodes.Get(i);
		Ptr<Ipv4> ipv4 = n->GetObject<Ipv4>();
		for (uint32_t l = 1; l < ipv4->GetNInterfaces(); l++) {
			out_addr_file << i <<  " " << ipv4->GetAddress(l, 0).GetLocal() << endl;
		}
	}
	out_addr_file.flush();
	out_addr_file.close();

	// %%%%%%%%%%%% Set up the Mobility Position
  MobilityHelper mobility;
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                              "MinX", DoubleValue (0),
                              "MinY", DoubleValue (0),
                              "DeltaX", DoubleValue (100.0),
                              "DeltaY", DoubleValue (100.0),
                              "GridWidth", UintegerValue (400),
		                          "LayoutType", StringValue ("RowFirst"));
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();

  map<int, NodeContainer> map_aps;
  NodeContainer clients;

  for (size_t i = 0; i < network.getNodes().size(); i++) {
		Ptr<Node> node = nodes.Get(network.getNodes().at(i)->getId());
		if (Names::FindName(node).find("ap") != string::npos) {
      map_aps.insert ( std::pair<int, NodeContainer>(network.getNodes().at(i)->getId(), NodeContainer()) );
    }
  }

  for (size_t i_client = 0; i_client < n_clients; i_client++) {
    for (auto& ap : map_aps) {
      Ptr<Node> node_client = CreateObject<Node> ();

      clients.Add(node_client);
      ap.second.Add(node_client);
    }
  }


  int seedValue = time(0);
  RngSeedManager::SetSeed(seedValue);
  srand(seedValue);
  rng.seed(seedValue);

  for (auto& ap : map_aps) {
    int ap_i = ap.first;
    NodeContainer& node_clients = ap.second;

    mobility.Install(nodes.Get(ap_i));

    Ptr<MobilityModel> mob = nodes.Get(ap_i)->GetObject<MobilityModel>();

    double rng_client = 360.0/node_clients.GetN();
    double x = mob->GetPosition().x, y = mob->GetPosition().y;

    unsigned client_i=0;
    for (size_t j = 0; j < 360 && client_i < node_clients.GetN(); j += rng_client) {
      double cosseno = cos(j);
      double seno    = sin(j);
      positionAlloc->Add(Vector(5*cosseno + x , 5*seno + y, 0));
      client_i++;
    }
  }

  positionAlloc->Add(Vector(50, 15, 0));   // Ap 0
	positionAlloc->Add(Vector(250, 15, 0));  // Ap 1
	positionAlloc->Add(Vector(150, 30, 0)); // router 2
	positionAlloc->Add(Vector(150, 45, 0)); // router 3

  positionAlloc->Add(Vector(50, 15, 0));   // Ap 0
	positionAlloc->Add(Vector(250, 15, 0));  // Ap 1
	positionAlloc->Add(Vector(150, 30, 0)); // router 2
	positionAlloc->Add(Vector(150, 45, 0)); // router 3


  mobility.SetPositionAllocator(positionAlloc);
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  for (auto& ap : map_aps) {
      NodeContainer& node_clients = ap.second;
      mobility.Install(node_clients);
  }
  // mobility.Install(clients);
  mobility.Install(nodes.Get(1));
	mobility.Install(nodes.Get(2));
	mobility.Install(nodes.Get(0));
	mobility.Install(nodes.Get(7));


  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper 	  phy     = YansWifiPhyHelper::Default();

  for (auto& ap : map_aps) {
    int ap_i = ap.first;
    NodeContainer& node_clients = ap.second;

    Ptr<Node> node = nodes.Get(ap_i);

    internet.Install(node_clients);

    WifiHelper wifi;
    WifiMacHelper mac;
    phy.SetChannel(channel.Create());

    ostringstream ss;
    ss << "ns-3-ssid-" << ++n_ap;
    Ssid ssid = Ssid(ss.str());

    wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    wifi.SetStandard (WIFI_PHY_STANDARD_80211g);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
    NetDeviceContainer ap_dev =  wifi.Install(phy, mac, node);

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
    NetDeviceContainer sta_dev = wifi.Install(phy, mac, node_clients);

    address.Assign(ap_dev);
    address.Assign(sta_dev);
    address.NewNetwork();
  }

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	// %%%%%%%%%%%% Set up the DASH server
	Ptr<Node> n_server = nodes.Get(dst_server);

	string representationStrings = GetCurrentWorkingDir() + "/../content/representations/netflix_vid1.csv";
	fprintf(stderr, "representations = %s\n", representationStrings.c_str ());

	string str_ipv4_server = Ipv4AddressToString(n_server->GetObject<Ipv4>()->GetAddress(1,0).GetLocal());

  DASHServerHelper server(Ipv4Address::GetAny (), 80, str_ipv4_server,
                          "/content/mpds/", representationStrings, "/content/segments/");

  ApplicationContainer serverApps = server.Install(n_server);
	serverApps.Start (Seconds(0.0));
	serverApps.Stop (Seconds(stopTime));

  //=======================================================================================
  network.setNodeContainers(&nodes);
  network.setClientContainers(&clients);

  Ptr<DashController> controller = CreateObject<DashController> ();
  n_server->AddApplication(controller);

  controller->Setup(&network, str_ipv4_server, Ipv4Address::GetAny (), 1317);
  controller->SetServerTableList(&serverTableList);
  controller->SetStartTime(Seconds(0.0));
  controller->SetStopTime(Seconds(stopTime));
	//=======================================================================================

  vector<double> startTime;

  int UserId=0;
  for (auto& ap : map_aps) {
    NodeContainer& node_clients = ap.second;

    for (size_t j = 0; j < node_clients.GetN(); j++) {
      int final_client = 100 * ap.first + UserId++;
      double t = poisson();

      int screenWidth = 1920;
      int screenHeight = 1080;

      stringstream mpd_baseurl;
      mpd_baseurl << "http://" << str_ipv4_server << "/content/mpds/";

      stringstream ssMPDURL;
      ssMPDURL << mpd_baseurl.str() << "vid" << 1 << ".mpd.gz";

  		DASHHttpClientHelper player(ssMPDURL.str());
      player.SetAttribute("AdaptationLogic", StringValue(AdaptationLogicToUse));
      player.SetAttribute("StartUpDelay", StringValue("4"));
      player.SetAttribute("ScreenWidth", UintegerValue(screenWidth));
      player.SetAttribute("ScreenHeight", UintegerValue(screenHeight));
      player.SetAttribute("UserId", UintegerValue(final_client));
      player.SetAttribute("AllowDownscale", BooleanValue(true));
      player.SetAttribute("AllowUpscale", BooleanValue(true));
      player.SetAttribute("MaxBufferedSeconds", StringValue("60"));

      ApplicationContainer clientApps;
      clientApps = player.Install(node_clients.Get(j));

      Ptr<Application> app = node_clients.Get(j)->GetApplication(0);
      app->GetObject<HttpClientDashApplication> ()->setServerTableList(&serverTableList);

      string str_ipv4_client = Ipv4AddressToString(node_clients.Get(j)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal());
      serverTableList[str_ipv4_client] = str_ipv4_server;

      startTime.push_back(t);
  	}
  }

  for (size_t i = 0; i < clients.GetN(); i++) {
    Ptr<Application> app = clients.Get(i)->GetApplication(0);

    for (auto& ap : map_aps) {
      NodeContainer& node_clients = ap.second;

      bool finded = false;
      for (size_t j = 0; j < node_clients.GetN(); j++) {
        if (node_clients.Get(j)->GetId() == clients.Get(i)->GetId()) {
          finded = true;
          break;
        }
      }
      if (finded) {
        string str_ipv4_client = Ipv4AddressToString(clients.Get(i)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal());

        cout << "user id=" << clients.Get(i)->GetId() << " user ip=" << str_ipv4_client << " server=" << str_ipv4_server << " ap=" << ap.first << endl;
        break;
      }
    }

    app->SetStartTime(Seconds(startTime[i]));
    app->SetStopTime(Seconds(stopTime));
  }

  Config::Connect("/NodeList/0/DeviceList/*/$ns3::PointToPointNetDevice/MacRx",
                  MakeCallback (&NodeStatistics::RateCallback, &eCtrl));
  Config::Connect("/NodeList/1/DeviceList/*/$ns3::PointToPointNetDevice/MacRx",
                  MakeCallback (&NodeStatistics::RateCallback, &eCtrl));
  Config::Connect("/NodeList/2/DeviceList/*/$ns3::PointToPointNetDevice/MacRx",
                  MakeCallback (&NodeStatistics::RateCallback, &eCtrl));
  Config::Connect("/NodeList/3/DeviceList/*/$ns3::PointToPointNetDevice/MacRx",
                  MakeCallback (&NodeStatistics::RateCallback, &eCtrl));
  Config::Connect("/NodeList/4/DeviceList/*/$ns3::PointToPointNetDevice/MacRx",
                  MakeCallback (&NodeStatistics::RateCallback, &eCtrl));
  Config::Connect("/NodeList/5/DeviceList/*/$ns3::PointToPointNetDevice/MacRx",
                  MakeCallback (&NodeStatistics::RateCallback, &eCtrl));
  Config::Connect("/NodeList/6/DeviceList/*/$ns3::PointToPointNetDevice/MacRx",
                  MakeCallback (&NodeStatistics::RateCallback, &eCtrl));
  Config::Connect("/NodeList/7/DeviceList/*/$ns3::PointToPointNetDevice/MacRx",
                  MakeCallback (&NodeStatistics::RateCallback, &eCtrl));

  Simulator::Schedule(Seconds(0), &NodeStatistics::CalculateThroughput, &eCtrl);


	// %%%%%%%%%%%% sort out the simulation
	string dir = CreateDir("../dash-multi-layer");
	AnimationInterface anim(dir + string("/topology.netanim"));

	DASHPlayerTracer::InstallAll(dir + string("/topology-") + to_string(seed) + string(".csv"));

	Simulator::Stop(Seconds(stopTime));
	Simulator::Run();
	Simulator::Destroy();

	DASHPlayerTracer::Destroy();

	return 0;
}
