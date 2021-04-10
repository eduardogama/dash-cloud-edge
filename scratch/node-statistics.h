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
  NodeStatistics(NetworkTopology *network, double stepsTime=0.1, string pathFile="../Troughput_");

  void CheckStatistics (double time);

  void RateCallback(std::string context, Ptr<const Packet> packet);

  void CalculateThroughput();

  void setLinkMap(string link, double datarate);
  double getLinkMap(string link);

  void createTroughputFile(string strip, int srcnode, int dstnode);
  void storeTroughputInFile(string strip, double mbs);
private:
  NetworkTopology *network;
  map<string, double> linkMap;
  map<string, string> troughputFile;
  double stepsTime;
  string pathFile;

  // ofstream filetostore;
};

NodeStatistics::NodeStatistics (NetworkTopology *network, double stepsTime, string pathFile)
  : network(network), stepsTime(stepsTime), pathFile(pathFile)
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

double NodeStatistics::getLinkMap(string link)
{
  return this->linkMap[link];
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

  Ptr<Node>      node = network->getNodeContainers()->Get(strnode);

  // if (strdev >= node->GetNDevices())
  //   return;

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
  }

  Simulator::Schedule(Seconds(stepsTime), &NodeStatistics::CalculateThroughput, this);
}

#endif //NODE_STATISTICS_H
