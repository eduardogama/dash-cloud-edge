#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/command-line.h"
#include "ns3/random-variable-stream.h"
#include "ns3/core-module.h"

#include <iostream>
#include <random>
#include <ctime>
#include <vector>
#include <algorithm>    // std::sort


using namespace std;
using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("VariablesDistMain");


int main(int argc, char *argv[])
{

  uint32_t n = 100;
  double alpha = 0.7;
  vector<int> zipf_vector(100,0);
  CommandLine cmd;
  cmd.Parse (argc, argv);


  SeedManager::SetRun (time(0));

  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();

  uv->SetAttribute ("Min", DoubleValue (0.0));
  uv->SetAttribute ("Max", DoubleValue (10.0));

  std::cout << uv->GetInteger () << std::endl;
  std::cout << "============================" << '\n';
  Ptr<ZipfRandomVariable> x = CreateObject<ZipfRandomVariable> ();

  x->SetAttribute ("N", IntegerValue (n));
  x->SetAttribute ("Alpha", DoubleValue (alpha));

  for (size_t i = 0; i < 100; i++) {
    int value = x->GetValue ();
    // std::cout << value << std::endl;
    zipf_vector[value]++;
  }
  sort (zipf_vector.begin(), zipf_vector.end());  
  for (size_t i = 0; i < zipf_vector.size(); i++) {
    std::cout << zipf_vector[i] << '\n';
  }
  std::cout << "============================" << '\n';


	Simulator::Run();
	Simulator::Destroy();


	return 0;
}
