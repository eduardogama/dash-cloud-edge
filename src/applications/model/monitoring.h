#ifndef MONITORING_H
#define MONITORING_H

#include "ns3/event-id.h"
#include "ns3/node-container.h"
#include "ns3/internet-module.h"
#include <vector>

#include <stdlib.h>     /* atoi */

#include "ctrl-main.h"

using namespace std;

namespace ns3 {

  class Monitoring : public Application
  {
    public:
      static TypeId GetTypeId();

      Monitoring();
      virtual ~Monitoring();

      void RateCallback(string context, Ptr<const Packet> packet);

      void BandwidthEstimator();

      void setToRedirect(bool toRedirect);

      void setPathFile(string pathFile);

      void setStepsTime(double stepsTime);

      void setLinkMap(string link, double datarate);
      double getLinkMap(string link);

      void setLinkCapacityMap(string link, int src, int dst, double datarate);

      void setNodes(NodeContainer *nodes);
      NodeContainer* getNodes();

      void CreateFile(string strip, int srcnode, int dstnode);
      void StoreFile(string strip, double mbs);

      void setController(Ptr<ControllerMain> controller);

    private:
      string Ipv4AddressToString (Ipv4Address ad);

    private:
      bool toRedirect;
      string pathFile;
      double stepsTime;

      map<string, double> linkMap;
      map<string, double> linkCapacityMap;
      map<string, pair<int, int>> peerMap;

      map<string, string> bwFile;

      Ptr<ControllerMain> controller;

      NodeContainer *nodes;
  };

}

#endif // MONITORING_H
