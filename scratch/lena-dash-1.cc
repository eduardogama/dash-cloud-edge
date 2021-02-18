/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 *
 * Modified by Eduardo S. Gama
 * Running DASH service in LTE+EPC networks
 *
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/rng-seed-manager.h"
//#include "ns3/visualizer.h"
#include "ns3/netanim-module.h"


#include "femtocell.h"

//#include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("LenaDashOne");

int
main (int argc, char *argv[])
{
    uint16_t numberOfNodes = 2;
    double simTime = 100.0;
    double distance = 60.0;
    std::string AdaptationLogicToUse = "dash::player::RateAndBufferBasedAdaptationLogic";
    
    // Command line arguments
    CommandLine cmd;
    cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
    cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
    cmd.AddValue("distance", "Distance between eNBs [m]", distance);
    
    cmd.Parse(argc, argv);
    
    // the scenario parameters get their values from the global attributes defined above
    UintegerValue uintegerValue;
    IntegerValue integerValue;
    DoubleValue doubleValue;
    BooleanValue booleanValue;
    StringValue stringValue;
    GlobalValue::GetValueByName ("nBlocks", uintegerValue);
    uint32_t nBlocks = uintegerValue.Get ();
    GlobalValue::GetValueByName ("nApartmentsX", uintegerValue);
    uint32_t nApartmentsX = uintegerValue.Get ();
    GlobalValue::GetValueByName ("nFloors", uintegerValue);
    uint32_t nFloors = uintegerValue.Get ();
    GlobalValue::GetValueByName ("nMacroEnbSites", uintegerValue);
    uint32_t nMacroEnbSites = uintegerValue.Get ();
    GlobalValue::GetValueByName ("nMacroEnbSitesX", uintegerValue);
    uint32_t nMacroEnbSitesX = uintegerValue.Get ();
    GlobalValue::GetValueByName ("interSiteDistance", doubleValue);
    double interSiteDistance = doubleValue.Get ();
    GlobalValue::GetValueByName ("areaMarginFactor", doubleValue);
    double areaMarginFactor = doubleValue.Get ();
    GlobalValue::GetValueByName ("macroUeDensity", doubleValue);
    double macroUeDensity = doubleValue.Get ();
    GlobalValue::GetValueByName ("homeEnbDeploymentRatio", doubleValue);
    double homeEnbDeploymentRatio = doubleValue.Get ();
    GlobalValue::GetValueByName ("homeEnbActivationRatio", doubleValue);
    double homeEnbActivationRatio = doubleValue.Get ();
    GlobalValue::GetValueByName ("homeUesHomeEnbRatio", doubleValue);
    double homeUesHomeEnbRatio = doubleValue.Get ();
    

    Box macroUeBox;
    double ueZ = 1.5;
    if(nMacroEnbSites > 0) {
        uint32_t currentSite = nMacroEnbSites -1;
        uint32_t biRowIndex = (currentSite / (nMacroEnbSitesX + nMacroEnbSitesX + 1));
        uint32_t biRowRemainder = currentSite % (nMacroEnbSitesX + nMacroEnbSitesX + 1);
        uint32_t rowIndex = biRowIndex*2 + 1;
        if(biRowRemainder >= nMacroEnbSitesX){
            ++rowIndex;
        }
        uint32_t nMacroEnbSitesY = rowIndex;
        NS_LOG_LOGIC ("nMacroEnbSitesY = " << nMacroEnbSitesY);

        macroUeBox = Box(-areaMarginFactor*interSiteDistance, 
                        (nMacroEnbSitesX + areaMarginFactor)*interSiteDistance, 
                        -areaMarginFactor*interSiteDistance, 
                        (nMacroEnbSitesY -1)*interSiteDistance*sqrt (0.75) + areaMarginFactor*interSiteDistance,
                        ueZ, ueZ);
    } else {
        // still need the box to place femtocell blocks
        macroUeBox = Box (0, 150, 0, 150, ueZ, ueZ);
    }

    
    FemtocellBlockAllocator blockAllocator(macroUeBox, nApartmentsX, nFloors);
    blockAllocator.Create(nBlocks);
    
    uint32_t nHomeEnbs = round (4 * nApartmentsX * nBlocks * nFloors * homeEnbDeploymentRatio * homeEnbActivationRatio);
    NS_LOG_LOGIC ("nHomeEnbs = " << nHomeEnbs);
    uint32_t nHomeUes = round (nHomeEnbs * homeUesHomeEnbRatio);
    NS_LOG_LOGIC ("nHomeUes = " << nHomeUes);
    double macroUeAreaSize = (macroUeBox.xMax - macroUeBox.xMin) * (macroUeBox.yMax - macroUeBox.yMin);
    uint32_t nMacroUes = round (macroUeAreaSize * macroUeDensity);
    NS_LOG_LOGIC ("nMacroUes = " << nMacroUes << " (density=" << macroUeDensity << ")");

    std::cout << "nApartmentsX = " << nApartmentsX << std::endl; // 10
    std::cout << "nBlocks = " << nBlocks << std::endl; //1
    std::cout << "homeEnbDeploymentRatio = " << homeEnbDeploymentRatio << std::endl; // 0.2
    std::cout << "homeEnbActivationRatio = " << homeEnbActivationRatio << std::endl; // 0.5
    
    std::cout << "nHomeEnbs = " << nHomeEnbs << std::endl; // 4
    std::cout << "nHomeUes = " << nHomeUes << std::endl; // 4
    std::cout << "nMacroUes = " << nMacroUes << std::endl; // 19
    std::cout << "nMacroEnbSites = " << nMacroEnbSites << std::endl; // 3
    

    NodeContainer macroEnbs;
    macroEnbs.Create(3 * nMacroEnbSites);
    NodeContainer macroUes;
    macroUes.Create(nMacroUes);
    NodeContainer enbNodes;
    enbNodes.Create(nHomeEnbs);
    NodeContainer ueNodes;
    ueNodes.Create(nHomeUes);
    
    
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

    Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper (epcHelper);
    
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    // Macro eNBs in 3-sector hex grid
    mobility.Install(macroEnbs);
    BuildingsHelper::Install(macroEnbs);
    Ptr<LteHexGridEnbTopologyHelper> lteHexGridEnbTopologyHelper = CreateObject<LteHexGridEnbTopologyHelper>();
    lteHexGridEnbTopologyHelper->SetLteHelper(lteHelper);
    lteHexGridEnbTopologyHelper->SetAttribute("InterSiteDistance", DoubleValue (interSiteDistance));
    lteHexGridEnbTopologyHelper->SetAttribute("MinX", DoubleValue (interSiteDistance/2));
    lteHexGridEnbTopologyHelper->SetAttribute("GridWidth", UintegerValue (nMacroEnbSitesX));
    lteHelper->SetEnbAntennaModelType("ns3::ParabolicAntennaModel");
    NetDeviceContainer macroEnbDevs = lteHexGridEnbTopologyHelper->SetPositionAndInstallEnbDevice(macroEnbs);

    // this enables handover for macro eNBs
    lteHelper->AddX2Interface (macroEnbs);
    
    Ptr<PositionAllocator> positionAlloc = CreateObject<RandomRoomPositionAllocator> ();
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(enbNodes);
    BuildingsHelper::Install(enbNodes);
    lteHelper->SetEnbAntennaModelType("ns3::IsotropicAntennaModel");
    lteHelper->SetEnbDeviceAttribute ("CsgId", UintegerValue (1));
    lteHelper->SetEnbDeviceAttribute ("CsgIndication", BooleanValue (true));
    NetDeviceContainer enbLteDevs  = lteHelper->InstallEnbDevice(enbNodes);
    
    // home UEs located in the same apartment in which there are the Home eNBs
    positionAlloc = CreateObject<SameRoomPositionAllocator>(enbNodes);
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(ueNodes);
    BuildingsHelper::Install(ueNodes);
    // set the home UE as a CSG member of the home eNodeBs
    lteHelper->SetUeDeviceAttribute ("CsgId", UintegerValue (1));
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
    

    positionAlloc = CreateObject<RandomBoxPositionAllocator>();
    Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable>();
    xVal->SetAttribute("Min", DoubleValue(macroUeBox.xMin));
    xVal->SetAttribute("Max", DoubleValue(macroUeBox.xMax));
    positionAlloc->SetAttribute("X", PointerValue (xVal));
    Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable>();
    yVal->SetAttribute("Min", DoubleValue(macroUeBox.yMin));
    yVal->SetAttribute("Max", DoubleValue(macroUeBox.yMax));
    positionAlloc->SetAttribute ("Y", PointerValue(yVal));
    Ptr<UniformRandomVariable> zVal = CreateObject<UniformRandomVariable>();
    zVal->SetAttribute("Min", DoubleValue(macroUeBox.zMin));
    zVal->SetAttribute("Max", DoubleValue(macroUeBox.zMax));
    positionAlloc->SetAttribute("Z", PointerValue (zVal));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(macroUes);
    
    BuildingsHelper::Install (macroUes);
    NetDeviceContainer macroUeDevs = lteHelper->InstallUeDevice (macroUes);

    // =============================================================================    
    // END POSITION ALLOCATOR
    // =============================================================================

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get (0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
    p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
    
    NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
    // interface 0 is localhost, 1 is the p2p device
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

    std::cout << "remoteHostAddr = " << remoteHostAddr << std::endl;

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get (u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

    // Attach one UE per eNodeB
    if(ueNodes.GetN() == enbNodes.GetN()){
        for (uint16_t i = 0; i < ueNodes.GetN(); i++)
        {
            lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
            // side effect: the default EPS bearer will be activated
        }
    } else {
        lteHelper->Attach(ueLteDevs);
    }
    
    internet.Install(macroUes);
    ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (macroUeDevs));
    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < macroUes.GetN (); ++u)
    {
        Ptr<Node> ueNode = macroUes.Get (u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }
    
    // macro UEs attached to the closest macro eNB
    lteHelper->AttachToClosestEnb (macroUeDevs, macroEnbDevs);


    NS_LOG_LOGIC ("Setting up Server application");

    int dashPort = 80;
    std::string srv_ip = "1.0.0.2";

    std::string representationStrings = "/home/eduardo/dataset/netflix_vid1.csv";
    DASHServerHelper server(Ipv4Address::GetAny(), dashPort, srv_ip,
				            "/content/mpds/", representationStrings, "/content/segments/");
    ApplicationContainer serverApp;
    serverApp = server.Install(remoteHost);
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(simTime));
    
    // ---------- Network Seed Setup ------------------------------------------------
    SeedManager::SetRun(time(0));
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
    // ---------- End Network Seed Setup --------------------------------------------

    // ---------- Create Dash client Setup ------------------------------------------
    vector<DASHHttpClientHelper> clients;
    vector<ApplicationContainer> clientApps;

    std::cout << "numberOfNodes = " << ueNodes.GetN() << std::endl;

    double rdm = 0.0;
    for (uint32_t user = 0; user < ueNodes.GetN(); user++) {
        std::cout << "User " << user << " Createad." << std::endl;
    
        Ptr<Node> ue = ueNodes.Get(user);
        
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

        ApplicationContainer clientApp = player.Install(ue);
        clientApp.Start(Seconds(0.25 + rdm));
        clientApp.Stop(Seconds(simTime));

        clients.push_back(player);
        clientApps.push_back(clientApp);
    }
    
    std::cout << "Macro Ues Applications ..." << std::endl;
    
    rdm = 0.0;
    for (uint32_t user = 0; user < macroUes.GetN(); user++) {
        std::cout << "User " << user << " Createad." << std::endl;
    
        Ptr<Node> ue = macroUes.Get(user);
        
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
        player.SetAttribute("UserId", UintegerValue(user + ueNodes.GetN()));
        player.SetAttribute("AllowDownscale", BooleanValue(true));
        player.SetAttribute("AllowUpscale", BooleanValue(true));
        player.SetAttribute("MaxBufferedSeconds", StringValue("60"));

        rdm += uv->GetValue();

        ApplicationContainer clientApp = player.Install(ue);
        clientApp.Start(Seconds(0.25 + rdm));
        clientApp.Stop(Seconds(simTime));

        clients.push_back(player);
        clientApps.push_back(clientApp);
    }
    
    BuildingsHelper::MakeMobilityModelConsistent ();

    AnimationInterface anim("topology1.netanim");
    for (uint32_t i = 0; i < ueNodes.GetN (); ++i)
    {
        anim.UpdateNodeDescription (ueNodes.Get (i), "Ue"); // Optional
        anim.UpdateNodeColor (ueNodes.Get (i), 255, 0, 0); // Optional
    }
    for (uint32_t i = 0; i < enbNodes.GetN (); ++i)
    {
        anim.UpdateNodeDescription (enbNodes.Get (i), "hEnb"); // Optional
        anim.UpdateNodeColor (enbNodes.Get (i), 0, 255, 0); // Optional
    }
    for (uint32_t i = 0; i < macroEnbs.GetN (); ++i)
    {
        anim.UpdateNodeDescription (macroEnbs.Get (i), "mEnb"); // Optional
        anim.UpdateNodeColor (macroEnbs.Get (i), 0, 0, 255); // Optional 
    }
    for (uint32_t i = 0; i < macroUes.GetN (); ++i)
    {
        anim.UpdateNodeDescription (macroUes.Get (i), "mUe"); // Optional
        anim.UpdateNodeColor (macroUes.Get (i), 0, 255, 255); // Optional 
    }

    anim.UpdateNodeDescription(remoteHost, "rmtHost"); // Optional
    anim.UpdateNodeDescription(pgw, "pgw"); // Optional
    
//    lteHelper->EnableTraces ();
    // Uncomment to enable PCAP tracing
    //p2ph.EnablePcapAll("lena-epc-first");

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();


    Simulator::Destroy();
    return 0;
}

