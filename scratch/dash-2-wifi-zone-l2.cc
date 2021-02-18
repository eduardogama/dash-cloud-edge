/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Eduardo S. Gama <eduardogama72@gmail.com>
 */

#include <ctime>
#include <cmath>
#include <iomanip>
#include <cstring>

#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"

#include "ns3/mobility-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-helper.h"

#include "ns3/applications-module.h"
#include "ns3/dashplayer-tracer.h"

#include "ns3/olsr-helper.h"

#include <iostream>

#include <fstream>
#include <string>
#include <sstream>

using namespace ns3;
using namespace std;

extern vector<vector<int> > readNxNMatrix (std::string adj_mat_file_name);
extern vector<vector<double> > readCordinatesFile (std::string node_coordinates_file_name);
extern void printCoordinateArray (const char* description, vector<vector<double> > coord_array);
extern void printMatrix (const char* description, vector<vector<int> > array);

extern vector<std::string> split(const std::string& str, const std::string& delim);
extern void store(uint32_t &tNodes, uint32_t &layer, uint32_t &users, uint16_t &seed, std::string &str);
extern void calcAvg(uint32_t &tNodes, uint32_t &layer, uint32_t &users);

uint32_t tRate;
double tStalls;
double tStalls_time;

std::string dir;

NS_LOG_COMPONENT_DEFINE("DashTwoWifiZonel2");

class Experiment {
    public:
        Experiment();
        ~Experiment();
        
        bool CreateDir(string name);
        string getDirectory();
        void setDirectory();
    private:
        string dir;
};

Experiment::Experiment(){
}
Experiment::~Experiment(){
}
bool Experiment::CreateDir(string name) {
    dir = name;
    return system(string(string("mkdir -p ") + name).c_str()) != -1;
}

string Experiment::getDirectory(){
    return dir;
}

void setDirectory(string _dir){
    dir = _dir;
}

int main(int argc, char *argv[]) {
    uint16_t seed = 0;

    tStalls      = 0;
    tStalls_time = 0;
    tRate        = 0;

    bool tracing         = false;
    uint32_t n_nodes     = 3;
    uint32_t users       = 1;
    uint32_t layer       = 0;
    double stopTime      = 100.0;
    string window        = "10s";
    string AdaptationLogicToUse = "dash::player::RateAndBufferBasedAdaptationLogic"; //dash::player::BufferBasedAdaptationLogic


    LogComponentEnable ("DashTwoWifiZonel2", LOG_LEVEL_ALL);

    CommandLine cmd;
    cmd.AddValue("tracing", "Flag to enable/disable tracing", tracing);
    cmd.AddValue("users", "The number of concurrent videos", users);
    cmd.AddValue("nodes", "The number of nodes.", n_nodes);
    cmd.AddValue("layer", "The layer where the nodes support the cloud provisioning resquest segments clients", layer);
    cmd.AddValue("stopTime", "The time when the clients will stop requesting segments", stopTime);
    cmd.AddValue("seed", "Seed experiment.", seed);
    cmd.AddValue("AdaptationLogicToUse", "Adaptation Logic to Use.", AdaptationLogicToUse);
    
    cmd.Parse(argc, argv);
    
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1400));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));

    // ---------- Network Setup ------------------------------------------------
    NS_LOG_INFO("Create nodes."); // Explicitly create the nodes required by the topology (shown above).

    NodeContainer nodes;   // Declare nodes objects
    nodes.Create (4);
    
    // ---------- Wired Network Setup ------------------------------------------
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
//    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    
    NodeContainer n02 = NodeContainer (nodes.Get (0), nodes.Get (2));
    NodeContainer n12 = NodeContainer (nodes.Get (1), nodes.Get (2));
    NodeContainer n23 = NodeContainer (nodes.Get (2), nodes.Get (3));
    
    NetDeviceContainer n02_dev = pointToPoint.Install (n02);
    NetDeviceContainer n12_dev = pointToPoint.Install (n12);
    NetDeviceContainer n23_dev = pointToPoint.Install (n23);

   
    stringstream gstream; //gstream << 4; string g=gstream.str(); 
    // ---------- End Network Setup --------------------------------------------
    
    // ---------- Network WiFi Users Setup -------------------------------------
    NS_LOG_INFO ("Create Links Between Nodes.");
    // AP node to connect host the users 
    NodeContainer wifi_n1;
    wifi_n1.Create(users);

    NodeContainer wifi_n2;
    wifi_n2.Create(users);

    NodeContainer ap_n1 = nodes.Get (0);
    NodeContainer ap_n2 = nodes.Get (1);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
            
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
    NetDeviceContainer sta_dev1 = wifi.Install(phy, mac, wifi_n1);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
    NetDeviceContainer ap_dev1 =  wifi.Install(phy, mac, ap_n1);


    WifiHelper wifi2;
    WifiMacHelper mac2;
    phy.SetChannel(channel.Create());
    Ssid ssid2 = Ssid("ns-3-ssid-2");
            
    wifi2.SetRemoteStationManager("ns3::AarfWifiManager");

    mac2.SetType("ns3::StaWifiMac", "Ssid", SsidValue (ssid2), "ActiveProbing", BooleanValue (false));
    NetDeviceContainer sta_dev2 = wifi2.Install(phy, mac2, wifi_n2);

    mac2.SetType("ns3::ApWifiMac", "Ssid", SsidValue (ssid2));
    NetDeviceContainer ap_dev2 =  wifi2.Install(phy, mac2, ap_n2);

    // ---------- Mobility Model Setup -------------------------------------
    MobilityHelper mobility;    
    Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();    
    
    // Initialize the coordinates and allocate the coordinates in each network node
    int aux = 360/(users);
    int dist = 8;
    
    for (size_t i = 0; i < 360; i += aux) {
        double cosseno = cos(i);
        double seno    = sin(i);
        positionAlloc->Add(Vector(dist*cosseno, dist*seno, 0)); // node1 -- starting very far away

        cout << "(x - y) = " << dist*cosseno << " - " << dist*seno << '\n';
    }
    for (size_t i = 0; i < 360; i += aux) {
        double cosseno = cos(i);
        double seno    = sin(i);
        positionAlloc->Add(Vector(dist*cosseno + 30, dist*seno, 0)); // node1 -- starting very far away

        cout << "(x - y) = " << dist*cosseno << " - " << dist*seno << '\n';
    }
    
    positionAlloc->Add(Vector(0, 0, 0));   // Ap 1
    positionAlloc->Add(Vector(30, 0, 0));   // Ap 2
    
    positionAlloc->Add(Vector(15, 15, 0)); // node2
    positionAlloc->Add(Vector(15, 30, 0)); // node3

    mobility.SetPositionAllocator(positionAlloc);    
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        
    mobility.Install(wifi_n1);
    mobility.Install(wifi_n2);
    mobility.Install(ap_n1);
    mobility.Install(ap_n2);
    mobility.Install(nodes.Get(2));
    mobility.Install(nodes.Get(3));
    
    
    // ---------- End Mobility Model Setup -------------------------------------

    InternetStackHelper internet;

    internet.Install(nodes.Get(3));    
    internet.Install(nodes.Get(2));    
    
    internet.Install(ap_n1);
    internet.Install(wifi_n1);
    
    internet.Install(ap_n2);
    internet.Install(wifi_n2);

    Ipv4AddressHelper ipv4_n;
    ipv4_n.SetBase("10.1.1.0", "255.255.255.0");

    ipv4_n.Assign(n02_dev);
    ipv4_n.Assign(n12_dev);
    ipv4_n.Assign(n23_dev);
    ipv4_n.NewNetwork();

    ipv4_n.Assign(ap_dev1);
    ipv4_n.Assign(sta_dev1);
    ipv4_n.NewNetwork();
    
    ipv4_n.Assign(ap_dev2);
    ipv4_n.Assign(sta_dev2);
   // ---------- End Network WiFi Users Setup ---------------------------------
    
    // ---------- Internet Stack Setup -------------------------------------------
    NS_LOG_INFO ("Install Internet Stack to Nodes.");    
    for (size_t i = 0; i < users; i++) {
        Ptr<Node> n = wifi_n1.Get(i);
        Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
        cout << "WiFi User Node " << i << " --> " << ipv4->GetAddress(1,0) << '\n';
    }
    for (size_t i = 0; i < users; i++) {
        Ptr<Node> n = wifi_n2.Get(i);
        Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
        cout << "WiFi User Node " << i << " --> " << ipv4->GetAddress(1,0) << '\n';
    }
    // ---------- End Internet Stack Setup -------------------------------------------
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // ---------- Create Dash Server Setup -------------------------------------------
    // CLOUD NODE ----------------
    Ptr<Node> n = nodes.Get(2);
    Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();    
    gstream << ipv4->GetAddress(1,0).GetLocal();

    NS_LOG_INFO("Create Applications.");
    string srv_ip = gstream.str();
    gstream.str("");
    
    cout << "Ap Wifi Node --> " << srv_ip << '\n';
    
    string representationStrings = "/home/eduardo/dataset/netflix_vid1.csv";
    fprintf(stderr, "representations = %s\n", representationStrings.c_str ());

    DASHServerHelper server (Ipv4Address::GetAny (), 80,  srv_ip,
                           "/content/mpds/", representationStrings, "/content/segments/");

    ApplicationContainer serverApp;
    serverApp = server.Install(nodes.Get(2));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(stopTime));

    // ---------- End Create Dash Server Setup --------------------------------------

    // AGGREGATION APP -------------------
    cout << "Ap Wifi Node --> " << srv_ip << '\n';
    AggregationHelper aggregationHelper(Ipv4Address::GetAny (), 1317, srv_ip);
    
    ApplicationContainer aggregationApp;
    aggregationApp = aggregationHelper.Install(nodes.Get(2));
    
    aggregationApp.Start(Seconds(0.0));
    aggregationApp.Stop(Seconds(stopTime));
    
    // ---------- Network Seed Setup ------------------------------------------------
    SeedManager::SetRun(time(0));
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
    // ---------- End Network Seed Setup --------------------------------------------

    // ---------- Create Dash client Setup ------------------------------------------
    vector<DASHHttpClientHelper> clients;
    vector<ApplicationContainer> clientApps;

    double rdm = 0.0;

    for (uint32_t user = 0; user < users; user++) {
        stringstream mpd_baseurl;

        mpd_baseurl << "http://" << srv_ip << "/content/mpds/";

        int videoId = 0;
        int screenWidth = 1920, screenHeight = 1080;

        stringstream ssMPDURL;
        ssMPDURL << mpd_baseurl.str () << "vid" << videoId+1 << ".mpd.gz";

        DASHHttpClientHelper player(ssMPDURL.str ());
        player.SetAttribute("AdaptationLogic", StringValue(AdaptationLogicToUse));
        player.SetAttribute("StartUpDelay", StringValue("4"));
        player.SetAttribute("ScreenWidth", UintegerValue(screenWidth));
        player.SetAttribute("ScreenHeight", UintegerValue(screenHeight));
        player.SetAttribute("UserId", UintegerValue(user));
        player.SetAttribute("AllowDownscale", BooleanValue(true));
        player.SetAttribute("AllowUpscale", BooleanValue(true));
        player.SetAttribute("MaxBufferedSeconds", StringValue("60"));

        rdm += uv->GetValue();

        ApplicationContainer clientApp = player.Install(wifi_n1.Get(user));
        clientApp.Start(Seconds(0.25 + rdm));
        clientApp.Stop(Seconds(stopTime));

        clients.push_back(player);
        clientApps.push_back(clientApp);
    }
    
    for (uint32_t user = 0; user < users; user++) {
        stringstream mpd_baseurl;

        mpd_baseurl << "http://" << srv_ip << "/content/mpds/";

        int videoId = 0;
        int screenWidth = 1920, screenHeight = 1080;

        stringstream ssMPDURL;
        ssMPDURL << mpd_baseurl.str () << "vid" << videoId+1 << ".mpd.gz";

        DASHHttpClientHelper player(ssMPDURL.str ());
        player.SetAttribute("AdaptationLogic", StringValue(AdaptationLogicToUse));
        player.SetAttribute("StartUpDelay", StringValue("4"));
        player.SetAttribute("ScreenWidth", UintegerValue(screenWidth));
        player.SetAttribute("ScreenHeight", UintegerValue(screenHeight));
        player.SetAttribute("UserId", UintegerValue(user+users));
        player.SetAttribute("AllowDownscale", BooleanValue(true));
        player.SetAttribute("AllowUpscale", BooleanValue(true));
        player.SetAttribute("MaxBufferedSeconds", StringValue("60"));

        rdm += uv->GetValue();

        ApplicationContainer clientApp = player.Install(wifi_n2.Get(user));
        clientApp.Start(Seconds(0.25 + rdm));
        clientApp.Stop(Seconds(stopTime));

        clients.push_back(player);
        clientApps.push_back(clientApp);
    }
    // ---------- End Create Dash client Setup -------------------------------------------

    Experiment exp;

    gstream << "dash-1-zone-same-dist-" << n_nodes << "-" << users << "-" << layer;
    if (!exp.CreateDir(gstream.str())) {
        printf("Error creating directory!n");
        exit(1);
    }
    gstream.str("");

    Simulator::Stop(Seconds(stopTime));


    gstream << exp.getDirectory() << "/output-dash-" << seed << ".csv";
    DASHPlayerTracer::InstallAll(gstream.str());
    gstream.str("");

    gstream << exp.getDirectory() << "/output-throughput-" << seed << ".csv";
    NodeThroughputTracer::InstallAll(gstream.str());
    gstream.str("");

    AnimationInterface anim(exp.getDirectory() + std::string("/topology.netanim"));

    // Flow Monitor -----------------------------------------------------------
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    monitor->Start (Seconds (0)); monitor->Stop (Seconds (stopTime));
    
    AsciiTraceHelper ascii;
//    pointToPoint.EnablePcapAll ("myfirst");
    phy.EnablePcapAll ("dash-wifi-l2");
    

    // Now, do the actual simulation.
    NS_LOG_INFO("Run Simulation.");

    Simulator::Run();

    monitor->SerializeToXmlFile (exp.getDirectory() + string("/netflow.flowmon"), true, true);

    Simulator::Destroy();
    DASHPlayerTracer::Destroy();

    return 0;
}

// ---------- Function Definitions -------------------------------------------

void calcAvg(uint32_t &tNodes, uint32_t &layer, uint32_t &users) {
    fstream file;

    ostringstream arq;   // stream used for the conversion

    arq << dir << "/" << "Bt_total_" << tNodes << "_" << users << "_" << layer << ".txt";

    file.open(arq.str().c_str(),fstream::out | fstream::app);


    locale mylocale("");
    file.imbue( mylocale );

    file << tStalls/(users) << " " << tStalls_time/(users) << " " << tRate/(users) << endl;

    file.close();
}

void store(uint32_t &tNodes, uint32_t &layer, uint32_t &users, uint16_t &seed, std::string &str) {
    fstream file;

    ostringstream arq;   // stream used for the conversion
    arq << dir << "/" << "Bt_PerUser_" << seed << "_" << tNodes << "_" << users << "_" << layer << ".txt";

    file.open(arq.str().c_str(),fstream::out | fstream::app);

    locale mylocale("");
    file.imbue( mylocale );

    //PB per user
    file << str << endl;
    file.close();

    vector<string> tokens = split(str, " ");
}

vector<std::string> split(const std::string& str, const std::string& delim) {
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do {
        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());

    return tokens;
}

vector<vector<int> > readNxNMatrix (std::string adj_mat_file_name) {
    ifstream adj_mat_file;
    adj_mat_file.open (adj_mat_file_name.c_str (), ios::in);
    if (adj_mat_file.fail ()) {
        NS_FATAL_ERROR ("File " << adj_mat_file_name.c_str () << " not found");
    }
    vector<vector<int> > array;
    int i = 0;
    int n_nodes = 0;

    while (!adj_mat_file.eof ()) {
        string line;
        getline (adj_mat_file, line);
        if (line == "") {
            NS_LOG_WARN ("WARNING: Ignoring blank row in the array: " << i);
            break;
        }

        istringstream iss (line);
        int element;
        vector<int> row;
        int j = 0;

        while (iss >> element) {
            row.push_back (element);
            j++;
        }

        if (i == 0) {
            n_nodes = j;
        }

        if (j != n_nodes ) {
            NS_LOG_ERROR ("ERROR: Number of elements in line " << i << ": " << j << " not equal to number of elements in line 0: " << n_nodes);
            NS_FATAL_ERROR ("ERROR: The number of rows is not equal to the number of columns! in the adjacency matrix");
        } else {
            array.push_back (row);
        }
        i++;
    }

    if (i != n_nodes) {
        NS_LOG_ERROR ("There are " << i << " rows and " << n_nodes << " columns.");
        NS_FATAL_ERROR ("ERROR: The number of rows is not equal to the number of columns! in the adjacency matrix");
    }

    adj_mat_file.close ();
    return array;
}

vector<vector<double> > readCordinatesFile (std::string node_coordinates_file_name) {
    ifstream node_coordinates_file;
    node_coordinates_file.open (node_coordinates_file_name.c_str (), ios::in);
    if (node_coordinates_file.fail ()) {
        NS_FATAL_ERROR ("File " << node_coordinates_file_name.c_str () << " not found");
    }
    vector<vector<double> > coord_array;
    int m = 0;

    while (!node_coordinates_file.eof ()) {
        string line;
        getline (node_coordinates_file, line);

        if (line == "") {
            NS_LOG_WARN ("WARNING: Ignoring blank row: " << m);
            break;
        }

        istringstream iss (line);
        double coordinate;
        vector<double> row;
        int n = 0;
        while (iss >> coordinate) {
            row.push_back (coordinate);
            n++;
        }

        if (n != 2) {
            NS_LOG_ERROR ("ERROR: Number of elements at line#" << m << " is "  << n << " which is not equal to 2 for node coordinates file");
            exit (1);
        } else {
            coord_array.push_back (row);
        }
        m++;
    }
    node_coordinates_file.close ();
    return coord_array;
}

void printMatrix (const char* description, vector<vector<int> > array) {
    cout << "**** Start " << description << "********" << endl;
    for (size_t m = 0; m < array.size (); m++) {
        for (size_t n = 0; n < array[m].size (); n++) {
            cout << array[m][n] << ' ';
        }
        cout << endl;
    }
    cout << "**** End " << description << "********" << endl;
}

void printCoordinateArray (const char* description, vector<vector<double> > coord_array) {
    cout << "**** Start " << description << "********" << endl;
    for (size_t m = 0; m < coord_array.size (); m++) {
        for (size_t n = 0; n < coord_array[m].size (); n++) {
            cout << coord_array[m][n] << ' ';
        }
        cout << endl;
    }
    cout << "**** End " << description << "********" << endl;
}

// ---------- End of Function Definitions ------------------------------------
