#include <vector>
#include <map>

#include "utils.h"


using namespace std;



map<int, vector<int>> contentVideoToNodes;

string OutputVideos(int start, int n)
{
  string representationStrings = "";
  for (int i=start; i <= n; i++) {
    // representationStrings += GetCurrentWorkingDir() + "/../content/representations/vid" + std::to_string(i) + ".csv";
    representationStrings += "content/representations/vid" + std::to_string(i) + ".csv";
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
