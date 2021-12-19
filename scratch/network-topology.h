#ifndef NETWORK_TOPOLOGY_HH_
#define NETWORK_TOPOLOGY_HH_

#include "ns3/core-module.h"

#include <string>
#include <vector>

#include "dash-define.h"
#include "group-user.h"

#include "path.h"


using namespace ns3;

class NetworkTopology
{
	public:
		NetworkTopology();
		virtual ~NetworkTopology();

		void AddLink(int src_id, int dst_id, double rate, double delay, double ploss, int buffersize_pkts);
		void AddNode(int id, string type);

		vector<_Link *> getLinks(void);
		vector<_Node *> getNodes(void);

		bool SearchRoute(unsigned s, unsigned t);

		Path getRoute();

		bool canAlloc(int actualPos, int nextPos);
		bool canAllocGroups(vector<GroupUser *>& groups, vector<int>& group_i, int actualPos, int nextPos);

		void allocStream(unsigned actualPos, unsigned nextPos);
		unsigned CapacityLink(unsigned actualPos, unsigned nextPos);
		void ResetCapacityLink(unsigned actualPos, unsigned nextPos);

		void SetUpAdjList(int size);
		void printAdjList();

		void setClientContainers(NodeContainer *clients);
		NodeContainer *getClientContainers(void);

		void setNodeContainers(NodeContainer *nodeContainers);
		NodeContainer *getNodeContainers(void);
	private:
		bool dijkstra(unsigned s, unsigned t);

		vector<_Link *> links;
		vector<_Node *> nodes;

		// construct a vector of vectors to represent an adjacency list
		vector<vector<unsigned>> adjList;
		vector<unsigned> matrixAllocation;

		unsigned size;
		Path route;

		vector< vector<string> > interfaceNode;

		NodeContainer *clients;
		NodeContainer *nodeContainers;
};

NetworkTopology::NetworkTopology()
{

}

NetworkTopology::~NetworkTopology()
{

}

void NetworkTopology::AddLink(int src_id, int dst_id, double rate, double delay, double ploss, int buffersize_pkts)
{
	this->links.push_back(new _Link(src_id, dst_id, rate, delay, ploss, buffersize_pkts));
}

void NetworkTopology::AddNode(int id, string type)
{
	this->nodes.push_back(new _Node(id, type));
}

vector<_Link *> NetworkTopology::getLinks(void)
{
	return this->links;
}

vector<_Node *> NetworkTopology::getNodes(void)
{
	return this->nodes;
}

bool NetworkTopology::canAlloc(int actualPos, int nextPos)
{
	for (auto& link: links) {
		if ((link->getSrcId() == actualPos && link->getDstId() == nextPos)
				|| (link->getSrcId() == nextPos && link->getDstId() == actualPos)) {
			return matrixAllocation[actualPos*size + nextPos] + 4300000 <= link->getRate();
		}
	}
	return true;
}

// bool NetworkTopology::canAllocGroups (vector<GroupUser *>& groups, vector<int>& group_i, int actualPos, int nextPos)
// {
// 	int sum = 0;
// 	return sum <= link->getRate();
// }

bool NetworkTopology::canAllocGroups (vector<GroupUser *>& groups, vector<int>& group_i, int actualPos, int nextPos)
{
	int sum = 0;
	for (auto& i : group_i) {
		sum += groups[i]->getUsers().size() * 4300000;
	}

	_Link *link = NULL;
	for (auto &_link: links) {
		if ((_link->getSrcId() == actualPos && _link->getDstId() == nextPos)
    		|| (_link->getSrcId() == nextPos && _link->getDstId() == actualPos)) {
			link = _link;
			break;
		}
	}

	if (link == NULL) {
		return false;
	}

	return sum <= link->getRate();
}

void NetworkTopology::allocStream(unsigned actualPos, unsigned nextPos)
{
	matrixAllocation[actualPos*size + nextPos] += 4300000;
}

unsigned NetworkTopology::CapacityLink(unsigned actualPos, unsigned nextPos)
{
	return this->matrixAllocation[actualPos*size + nextPos];
}

void NetworkTopology::ResetCapacityLink(unsigned actualPos, unsigned nextPos)
{
	matrixAllocation[actualPos*size + nextPos] = 0;
}

void NetworkTopology::SetUpAdjList(int size)
{
	// resize the vector to N elements of type vector<int>
	adjList.resize(nodes.size());

	// add edges to the directed graph
	for (auto &link: links)
	{
		// insert at the end
		adjList[link->getSrcId()].push_back(link->getDstId());

		// Uncomment below line for undirected graph
		adjList[link->getDstId()].push_back(link->getSrcId());
	}

	size = nodes.size();
	matrixAllocation.resize(size*size, 0);
}

void NetworkTopology::printAdjList()
{
	for (unsigned i = 0; i < size; i++) {
		cout << i << " --> ";

		for (int v : adjList[i]) {
			cout << v << " ";
		}
		cout << endl;
	}
}

void NetworkTopology::setClientContainers(NodeContainer *clients)
{
	this->clients = clients;
}

NodeContainer *NetworkTopology::getClientContainers(void)
{
	return this->clients;
}

void NetworkTopology::setNodeContainers(NodeContainer *nodeContainers)
{
	this->nodeContainers = nodeContainers;
}

NodeContainer *NetworkTopology::getNodeContainers(void)
{
	return this->nodeContainers;
}

Path NetworkTopology::getRoute()
{
	return this->route;
}

bool NetworkTopology::SearchRoute(unsigned s, unsigned t)
{
	if (s == t) {
		route.clear();
		route.addLinkToPath(s);
		return true;
	}
	return dijkstra(s,t);
}

bool NetworkTopology::dijkstra(unsigned s, unsigned t)
{
	const unsigned inf = 0xFFFFFFFF;

	vector<unsigned> dist(this->size,inf);
	vector<unsigned> prev(this->size,inf);
	vector<bool> pego(this->size,false);

	dist[t] = 0;
	prev[t] = t;
	pego[t] = true;

	unsigned k = t, min = inf;
	unsigned assegure = 0;
	do
	{
		for (unsigned i = 0; i < adjList[k].size(); i++) {
	    if (!pego[adjList[k][i]]) {
        if (dist[k] + 1 < dist[ adjList[k][i] ]) {
          prev[adjList[k][i]] = k;
          dist[adjList[k][i]] = dist[k] + 1;
        }
	    }
		}

		k = 0;
		min = inf;

		for (unsigned i = 0; i < adjList.size(); i++) {
	    if (!pego[i] && dist[i] < min) {
        min = dist[i];
				k = i;
	    }
		}

		pego[k] = true;

		if (assegure++ >= size) //grafo disconexo
	    return false;

	} while (k != s);

	k = s;

	route.clear();
	do
	{
		route.addLinkToPath(k);
		k = prev[k];
	} while (prev[k] != k);
	route.addLinkToPath(k);

	return true;
}

#endif
