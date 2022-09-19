#include <vector>
#include <map>


using namespace std;


map<int, int> capacityNodes;
map<int, int> assignedNodes;
map<int, vector<int>> contentVideoToNodes;

string OutputVideos(int start, int n)
{
    string representationStrings = "";
    for (int i=start; i <= n; i++) {
        representationStrings += "content/representations/vid" + to_string(i) + ".csv";
        if (i < n) {
            representationStrings += ",";
        }
    }

    return representationStrings;
}

void BindVideosToNode(int idNode,int start, unsigned contentN)
{
    for (size_t i = start; i <= contentN; i++) {
        contentVideoToNodes[idNode].push_back(i);
    }
}

bool hasVideoByNode(int idNode, int content)
{
    for (auto& videoId : contentVideoToNodes[idNode]) {
        if (videoId == content) {
            return true;
        }
    }
    return false;
}

void AddCapacityNode(int idNode, int capacity)
{
    capacityNodes[idNode] = capacity;
    assignedNodes[idNode] = 0;
}

bool VideoAssignment(int idNode, int content)
{
    if (assignedNodes[idNode] < capacityNodes[idNode]) {
        assignedNodes[idNode]++;
        BindVideosToNode(idNode,content,content);

        return true;
    }

    return false;
}

string getVideoPath(int idVideo)
{
    return "content/representations/vid" + to_string(idVideo) + ".csv";
}
