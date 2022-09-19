#ifndef NETWORK_GRAPH_H
#define NETWORK_GRAPH_H

#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/internet-module.h"


#include <string>
#include <vector>

#include "group-user.h"
#include "path.h"


using namespace ns3;

class NetworkGraph
{
public:
	NetworkGraph();
	virtual ~NetworkGraph();

	void setClientContainers(NodeContainer *clients);
	NodeContainer *getClientContainers(void);

	void setNodeContainers(NodeContainer *nodeContainers);
	NodeContainer *getNodeContainers(void);

	void SetUpAdjList(int size);
	void AddLink(int src_id, int dst_id, double rate, double delay, double ploss, int buffersize_pkts);

	bool SearchRoute(unsigned s, unsigned t);

	Path getRoute();

private:
	bool dijkstra(unsigned s, unsigned t);

private:
	NodeContainer *clients;
	NodeContainer *nodeContainers;

	unsigned size;
	Path route;

	vector<vector<unsigned>> adjList;

};

NetworkGraph::NetworkGraph()
{

}

NetworkGraph::~NetworkGraph()
{

}

void NetworkGraph::SetUpAdjList(int size)
{
	this->size = size;
	adjList.resize(size);
}

void NetworkGraph::AddLink(int src_id, int dst_id, double rate, double delay, double ploss, int buffersize_pkts)
{
	adjList[src_id].push_back(dst_id);
  adjList[dst_id].push_back(src_id);
}

bool NetworkGraph::SearchRoute(unsigned s, unsigned t)
{
	return dijkstra(s, t);
}

bool NetworkGraph::dijkstra(unsigned s, unsigned t)
{
	const unsigned inf = 0xFFFFFFFF;

	vector<unsigned> dist(size, inf);
	vector<unsigned> prev(size, inf);
	vector<bool> discovered(size, false);

	dist[t] = 0;
	prev[t] = t;
	discovered[t] = true;

	unsigned k = t, min = inf;
	unsigned assegure = 0;
	do
	{
		for (unsigned i = 0; i < adjList[k].size(); i++) {
	        if (!discovered[adjList[k][i]]) {
                if (dist[k] + 1 < dist[ adjList[k][i] ]) {
                  prev[adjList[k][i]] = k;
                  dist[adjList[k][i]] = dist[k] + 1;
                }
	        }
		}

		k = 0;
		min = inf;

		for (unsigned i = 0; i < adjList.size(); i++) {
	        if (!discovered[i] && dist[i] < min) {
                min = dist[i];
		        k = i;
            }
	    }

		discovered[k] = true;

		if(assegure++ >= size) //grafo disconexo
	        return false;

	} while (k != s && s != t);

    route.clear();

    for(k = s; prev[k] != k; k = prev[k]){
        route.addLinkToPath(k);
    }
    route.addLinkToPath(k);

    return true;
}

Path NetworkGraph::getRoute()
{
	return this->route;
}

void NetworkGraph::setClientContainers(NodeContainer *clients)
{
	this->clients = clients;
}

NodeContainer *NetworkGraph::getClientContainers(void)
{
	return this->clients;
}

void NetworkGraph::setNodeContainers(NodeContainer *nodeContainers)
{
	this->nodeContainers = nodeContainers;
}

NodeContainer *NetworkGraph::getNodeContainers(void)
{
	return this->nodeContainers;
}

#endif //NETWORK_GRAPH_H
