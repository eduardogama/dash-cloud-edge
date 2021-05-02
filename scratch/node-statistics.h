#ifndef NODE_STATISTICS_H
#define NODE_STATISTICS_H

#include <iostream>
#include <vector>
#include <stdlib.h>     /* atoi */

#include "ns3/gnuplot.h"

#include "dash-utils.h"
#include "network-topology.h"

using namespace std;

class NodeStatistics
{
public:
  NodeStatistics(NetworkTopology *network, double stepsTime=0.1, string pathFile="../Troughput_", bool toRedirect=false);

  void CheckStatistics (double time);

  void RateCallback(std::string context, Ptr<const Packet> packet);

  void CalculateThroughput();

  void setLinkMap(string link, double datarate);
  double getLinkMap(string link);

  void setNodes(NodeContainer *nodes);
  NodeContainer* getNodes();

  void setController(Ptr<DashController> controller);
  Ptr<DashController> getController();

  void setLinkCapacityMap(string link, int src, int dst, double datarate);
  void getLinkCapacityMap(string link);
  void getLinkCapacityMap(int src, int dst);

  void createTroughputFile(string strip, int srcnode, int dstnode);
  void storeTroughputInFile(string strip, double mbs);
private:
  Ptr<DashController> controller;
  NetworkTopology *network;

  map<string, double> linkMap;

  map<string, double> linkCapacityMap;
  map<string, pair<int,int>> linkNodesMap;

  map<string, string> troughputFile;

  bool toRedirect;
  double stepsTime;
  string pathFile;

  NodeContainer *nodes;
};

NodeStatistics::NodeStatistics (NetworkTopology *network, double stepsTime, string pathFile, bool toRedirect)
  : network(network), stepsTime(stepsTime), pathFile(pathFile), toRedirect(toRedirect)
{
  // system(string(string("mkdir -p troughput")).c_str());
}

void NodeStatistics::CheckStatistics (double time)
{
  // Simulator::Schedule (Seconds (stepsTime), &NodeStatistics::RateCallback, this);
}

void NodeStatistics::setLinkMap(string link, double datarate)
{
  this->linkMap[link] = datarate;
}

double NodeStatistics::getLinkMap(string link)
{
  return this->linkMap[link];
}

void NodeStatistics::setNodes(NodeContainer *nodes)
{
  this->nodes = nodes;
}

NodeContainer* NodeStatistics::getNodes()
{
  return this->nodes;
}

void NodeStatistics::setController(Ptr<DashController> controller)
{
  this->controller = controller;
}

Ptr<DashController> NodeStatistics::getController()
{
  return this->controller;
}

void NodeStatistics::setLinkCapacityMap(string link, int src, int dst, double datarate)
{
  pair<int,int> linknode{src,dst};
  this->linkNodesMap[link] = linknode;
  this->linkCapacityMap[link] = datarate;
}

void NodeStatistics::createTroughputFile(string strip, int srcnode, int dstnode)
{
  ostringstream arq;
  ofstream filetostore;

  arq << pathFile << srcnode << "_" << dstnode;

  troughputFile[strip] = arq.str();

  filetostore.open (arq.str().c_str(), ios::out);
  filetostore << "Time troughput(bit/s)" << endl;
  filetostore.close();
}

void NodeStatistics::storeTroughputInFile(string strip, double mbs)
{
  ostringstream arq;
  ofstream filetostore;

  filetostore.open (troughputFile[strip].c_str(), ios::out | ios::app);
  filetostore << Simulator::Now().GetSeconds() << " "<< mbs << endl;
  filetostore.close();
}

//---------------------------------------------------------------------------------------
//-- Callback function is called whenever a packet is received successfully.
//-- This function cumulatively add the size of data packet to totalBytesReceived counter.
//---------------------------------------------------------------------------------------
void NodeStatistics::RateCallback(std::string context, Ptr<const Packet> packet)
{
  vector<string> str_context = split(context, "/");

  int strnode = atoi(str_context[2].c_str());
  int strdev  = atoi(str_context[4].c_str());

  Ptr<Node>      node = getNodes()->Get(strnode);

  Ptr<NetDevice> dev  = node->GetDevice(strdev);
  Ptr<Ipv4>      ipv4 = node->GetObject<Ipv4>();

  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (strdev,0);

  string ipAddrBdst = Ipv4AddressToString(iaddr.GetBroadcast());

  setLinkMap(ipAddrBdst, getLinkMap(ipAddrBdst) + packet->GetSize());
}

void NodeStatistics::CalculateThroughput()
{
  for (auto& link: linkMap) {
    string iplink   = link.first;
    double datarate = link.second;

    double mbs = ((datarate * 8.0) / (1000000.0 * stepsTime));

    storeTroughputInFile(iplink, mbs);
    setLinkMap(iplink, 0);

    if (mbs > this->linkCapacityMap[iplink] && toRedirect) {
    // if (mbs > 5) {
      int actualNode = this->linkNodesMap[iplink].first;
      int nextNode   = this->linkNodesMap[iplink].second;

      // std::cout << "Troughput "<< mbs << " > 5 =======" << '\n';
      // std::cout << iplink << "=" << actualNode << "," << nextNode << '\n';
      this->controller->RedirectUsers(actualNode, nextNode);
    }
  }

  Simulator::Schedule(Seconds(stepsTime), &NodeStatistics::CalculateThroughput, this);
}

#endif //NODE_STATISTICS_H
