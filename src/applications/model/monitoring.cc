#include "monitoring.h"

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/string.h"
#include "ns3/simulator.h"

#include <iostream>
#include <fstream>


using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Monitoring");

NS_OBJECT_ENSURE_REGISTERED (Monitoring);


TypeId Monitoring::GetTypeId (void)
{
    static TypeId tid = TypeId("ns3::Monitoring")
        .SetParent<Application> ()
        .SetGroupName("Applications")
        .AddConstructor<Monitoring> ()
    ;
    return tid;
}

Monitoring::Monitoring()
{
    NS_LOG_FUNCTION (this);
}

Monitoring::~Monitoring()
{
    NS_LOG_FUNCTION (this);
}

void Monitoring::RateCallback(string context, Ptr<const Packet> packet)
{
    vector<string> str_context = Split(context, "/");

    int strnode = atoi(str_context[2].c_str());
    int strdev  = atoi(str_context[4].c_str());

    Ptr<Node>      node = nodes->Get(strnode);

    Ptr<NetDevice> dev  = node->GetDevice(strdev);
    Ptr<Ipv4>      ipv4 = node->GetObject<Ipv4>();

    Ipv4InterfaceAddress iaddr = ipv4->GetAddress (strdev,0);

    string ipAddrBdst = Ipv4AddressToString(iaddr.GetBroadcast());

    setLinkMap(ipAddrBdst, getLinkMap(ipAddrBdst) + packet->GetSize());
}

void Monitoring::BandwidthEstimator()
{
    for (auto& link : this->linkMap) {
        string iplink   = link.first;
        double datarate = link.second;

        double mbs = ((datarate * 8.0) / (1000000.0 * stepsTime));

        StoreFile(iplink, mbs);
        setLinkMap(iplink, 0);

        // if ((mbs > this->linkCapacityMap[iplink]) && toRedirect) {
        if ((mbs > 2) && toRedirect) {
            int actualNode = this->peerMap[iplink].first;
            int nextNode   = this->peerMap[iplink].second;

            cout << "Congested Link = ("<< actualNode << ", " << nextNode << ")" << endl;

            this->controller->OptimizerComponent(actualNode, nextNode);
        }
    }

    Simulator::Schedule(Seconds(stepsTime), &Monitoring::BandwidthEstimator, this);
}

void Monitoring::setController(Ptr<ControllerMain> controller)
{
    this->controller = controller;
}


void Monitoring::setToRedirect(bool toRedirect)
{
    this->toRedirect = toRedirect;
}

void Monitoring::setPathFile(string pathFile)
{
    this->pathFile = pathFile;
}

void Monitoring::setStepsTime(double stepsTime)
{
    this->stepsTime = stepsTime;
}

void Monitoring::CreateFile(string strip, int srcnode, int dstnode)
{
    ostringstream arq;
    ofstream filetostore;

    arq << pathFile << srcnode << "_" << dstnode;

    bwFile[strip] = arq.str();

    filetostore.open (arq.str().c_str(), ios::out);
    filetostore << "Time troughput(bit/s)" << endl;
    filetostore.close();
}

void Monitoring::StoreFile(string strip, double mbs)
{
    ostringstream arq;
    ofstream filetostore;

    filetostore.open (bwFile[strip].c_str(), ios::out | ios::app);
    filetostore << Simulator::Now().GetSeconds() << " "<< mbs << endl;
    filetostore.close();
}

void Monitoring::setLinkMap(string link, double datarate)
{
    this->linkMap[link] = datarate;
}

double Monitoring::getLinkMap(string link)
{
    return this->linkMap[link];
}

void Monitoring::setLinkCapacityMap(string link, int src, int dst, double datarate)
{
    pair<int,int> linknode{src,dst};
    this->peerMap[link] = linknode;
    this->linkCapacityMap[link] = datarate;
}

void Monitoring::setNodes(NodeContainer *nodes)
{
    this->nodes = nodes;
}

NodeContainer* Monitoring::getNodes()
{
    return this->nodes;
}

string Monitoring::Ipv4AddressToString (Ipv4Address ad)
{
    ostringstream oss;
    ad.Print (oss);
    return oss.str ();
}

}
