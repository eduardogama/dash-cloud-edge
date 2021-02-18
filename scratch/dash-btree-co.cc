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

#include "group-user.h"

#include "ns3/hash.h"

#include <ctime>

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("DashBTreeCloudOnly");

void onHandover(unsigned from, unsigned to, Ptr<DashController> &controller)
{
  controller->CacheJoinAssignment(from, to);
  controller->DoSendRedirect();
}

void Run(unsigned from, unsigned to, int userId, Ptr<DashController> &controller)
{
  int content = 1; //zipf(0.7, 100);

  TypeOpt result = controller->tryRequest(from, to, content, userId);

  switch (result) {
    case RedirectGroup:
      if (controller->hasToRedirect()) {
        controller->DoSendRedirect();
      }
      break;
    case Handover:
      onHandover(from, to, controller);
      break;
    case AddContainer:
      std::cout << "ENTROU onAddContainer !" << '\n';
      controller->onAddContainer(from, to, content, userId);
      break;
    default:
      cout << "[Controller Decision]Connection Accepted!" << endl;
  }

  if (controller->hasToRedirect()) {
    controller->RunController();
    controller->DoSendRedirect();
  }

}

int main (int argc, char *argv[])
{
  std::map<string, string> serverTableList;
	unsigned n_clients = 1;
  int dst_server = 7;

	std::string scenarioFiles = GetCurrentWorkingDir() + "/../../content/scenario";
	std::string requestsFile = "requests";
	std::string DashTraceFile = "report.csv";
	std::string ServerThroughputTraceFile = "server_throughput.csv";
	std::string RepresentationType = "netflix";

	std::string AdaptationLogicToUse = "dash::player::BufferBasedAdaptationLogic";

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

  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (1446));
	Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue (524288));
	Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue (524288));


  NetworkTopology network;
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
    double errRate = network.getLinks().at(i)->getPLoss();
		DoubleValue rate (errRate);
    // Ptr<RateErrorModel> em1 = CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0,Max=1.0]"), "ErrorRate", rate);
		// Ptr<RateErrorModel> em2 = CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0,Max=1.0]"), "ErrorRate", rate);

    p2p.SetDeviceAttribute("DataRate", DataRateValue( network.getLinks().at(i)->getRate() )); // Mbit/s

		// And then install devices and channels connecting our topology
		NetDeviceContainer deviceContainer;
		deviceContainer = p2p.Install(nodes.Get(network.getLinks().at(i)->getSrcId()), nodes.Get(network.getLinks().at(i)->getDstId()));
    // deviceContainer.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue (em1));
		// deviceContainer.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue (em2));

		address.Assign(deviceContainer);
		address.NewNetwork();
    netDevices.push_back(deviceContainer);
	}

  p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mb/s")); // This must not be more than the maximum throughput in 802.11n
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
  for (size_t i = 0; i < network.getNodes().size(); i++) {
    NetDeviceContainer deviceContainer;
    deviceContainer = p2p.Install (nodes.Get(i), cache_nodes.Get(i));

    address.Assign(deviceContainer);
    address.NewNetwork();
    netDevices.push_back(deviceContainer);
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


  int seedValue = time(0);
  RngSeedManager::SetSeed(seedValue);
  srand(seedValue);
  rng.seed(seedValue);
  
  vector<int> v_aps;

  for (size_t i = 0; i < network.getNodes().size(); i++) {
		Ptr<Node> node = nodes.Get(network.getNodes().at(i)->getId());
		if (Names::FindName(node).find("ap") != string::npos) {
      v_aps.push_back(network.getNodes().at(i)->getId());
    }
  }

  for (auto& ap_i : v_aps) {
    mobility.Install(nodes.Get(ap_i));
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


  // mobility.Install(clients);
  mobility.Install(nodes.Get(1));
	mobility.Install(nodes.Get(2));
	mobility.Install(nodes.Get(0));
	mobility.Install(nodes.Get(7));

  mobility.Install(cache_nodes.Get(1));
  mobility.Install(cache_nodes.Get(2));
  mobility.Install(cache_nodes.Get(0));
  mobility.Install(cache_nodes.Get(7));

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	// %%%%%%%%%%%% Set up the DASH server
	Ptr<Node> n_server = cache_nodes.Get(dst_server);

	string representationStrings = GetCurrentWorkingDir() + "/../../../dataset/netflix_vid1.csv";
	fprintf(stderr, "representations = %s\n", representationStrings.c_str ());

	string str_ipv4_server = Ipv4AddressToString(n_server->GetObject<Ipv4>()->GetAddress(1,0).GetLocal());

  for (size_t i = 7; i < cache_nodes.GetN(); i++) {
    Ptr<Node> n_mec_server = cache_nodes.Get(i);
    Ptr<Ipv4> src_ip = n_mec_server->GetObject<Ipv4> ();

    string str_ipv4_server_1 = Ipv4AddressToString(n_mec_server->GetObject<Ipv4>()->GetAddress(1,0).GetLocal());
    DASHServerHelper server(Ipv4Address::GetAny (), 80, str_ipv4_server_1,
                            "/content/mpds/", representationStrings, "/content/segments/");

    ApplicationContainer serverApps = server.Install(n_mec_server);
  	serverApps.Start (Seconds(0.0));
  	serverApps.Stop (Seconds(stopTime));
  }

	//=======================================================================================
  network.setNodeContainers(&cache_nodes);
	//=======================================================================================

  int UserId=0;

	for (size_t i = 0; i < n_clients*v_aps.size(); i++) {
    int ap_i = uniformDis();
    double t = poisson();
    
    int screenWidth = 1920;
    int screenHeight = 1080;

    stringstream mpd_baseurl;
    mpd_baseurl << "http://" << str_ipv4_server << "/content/mpds/";

    stringstream ssMPDURL;
    ssMPDURL << mpd_baseurl.str() << "vid" << 1 << ".mpd.gz";

		DASHHttpClientHelper player (ssMPDURL.str());
    player.SetAttribute("AdaptationLogic", StringValue(AdaptationLogicToUse));
    player.SetAttribute("StartUpDelay", StringValue("4"));
    player.SetAttribute("ScreenWidth", UintegerValue(screenWidth));
    player.SetAttribute("ScreenHeight", UintegerValue(screenHeight));
    player.SetAttribute("UserId", UintegerValue(UserId++));
    player.SetAttribute("AllowDownscale", BooleanValue(true));
    player.SetAttribute("AllowUpscale", BooleanValue(true));
    player.SetAttribute("MaxBufferedSeconds", StringValue("60"));

    ApplicationContainer clientApps;
    clientApps = player.Install(nodes.Get(ap_i));
    
    clientApps.Start(Seconds(t));
    clientApps.Stop(Seconds(stopTime));
    
    cout << "user id=" << nodes.Get(ap_i)->GetId() << " user ip=" << str_ipv4_server << " server=" << str_ipv4_server << " ap=" << ap_i << endl;
  }
  

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
