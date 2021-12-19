#ifndef UTILS_HH_
#define UTILS_HH_

#include "ns3/internet-module.h"
#include "ns3/string.h"


#include <string>
#include <vector>
#include <fstream>
#include <random>

#include "dash-utils.h"
#include "dash-define.h"

#include "network-topology.h"


using namespace std;
using namespace ns3;

static string Ipv4AddressToString (Ipv4Address ad)
{
	ostringstream oss;
	ad.Print (oss);
	return oss.str ();
}

string GetCurrentWorkingDir(void)
{
	char buff[250];
	getcwd( buff, 250 );
	string current_working_dir(buff);
	return current_working_dir;
}

string CreateDir(string name)
{
	if(system(string(string("mkdir -p ") + name).c_str()) != -1)
		return name;
    return "";
}

vector<string> str_split(const string& s, const string& delimiter, const bool& removeEmptyEntries = false)
{
    vector<string> tokens;

    for (size_t start = 0, end; start < s.length(); start = end + delimiter.length()) {
         size_t position = s.find(delimiter, start);
         end = position != string::npos ? position : s.length();

         string token = s.substr(start, end - start);
         if (!removeEmptyEntries || !token.empty()) {
             tokens.push_back(token);
         }
    }

    if (!removeEmptyEntries && (s.empty() || endsWith(s, delimiter))) {
        tokens.push_back("");
    }

    return tokens;
}

template <typename T>
bool ReadTopology(string linksFile, string nodesFile, T &net)
{
	// node data
  int id;
	string type;
	int n=0;
	int linecount = 0;
	
	ifstream nodes_file(nodesFile.c_str());
	linecount = 0;
	while (nodes_file) {
		string line;
		linecount++;
		if (!getline(nodes_file, line)) {
			break;
		}

		cout << line << endl;

		if (linecount > 1) {
			// Get each element of the demand
			istringstream linestream( line );
			int columncount = 1;
			while (linestream) {
				string s;
				if (!getline( linestream, s, ' ' )) {
					break;
				}
				switch (columncount) {
					case 1:
						id = std::atoi(s.c_str());
						break;
					case 2:
						type = s;
						break;
				}
				columncount++;
			}
			net.AddNode(id, type);
			n++;
		}
	}
	nodes_file.close();

	net.SetUpAdjList(n);


	ifstream links_file(linksFile.c_str());

	// link data
  int src_id;
  int dst_id;
  double rate;
  double delay;
  double ploss;
  double buffersize_pkts;

	while (links_file) {
		string line;
		linecount++;

		if (!getline(links_file, line)) {
			break;
		}
		if (line == "\n") {
			break;
		}

		cout << line << endl;

		if (linecount > 1) {
			istringstream linestream(line);
			int columncount = 1;
			while (linestream) {
				string str;

				if (!getline(linestream, str, ' ')) {
					break;
				}

				switch (columncount) {
					case 1:
						src_id = atoi(str.c_str());
						break;
					case 2:
						dst_id = atoi(str.c_str());
						break;
					case 3:
						rate = atof(str.c_str());
						break;
					case 4:
						delay = atof(str.c_str());
						break;
					case 5:
						ploss = atof(str.c_str());
						break;
					case 6:
						buffersize_pkts = atof(str.c_str());
						break;
				}
				columncount++;
			}
			net.AddLink(src_id, dst_id, rate, delay, ploss, buffersize_pkts);
		}
	}
	links_file.close();

//	net.printAdjList();
//	getchar();

	return true;
}

std::vector< double > sum_probs;
std::vector< double > sum_probsQ;

double averageArrival = 5;
double lamda = 1 / averageArrival;
std::mt19937 rng (0);
std::exponential_distribution<double> poi (lamda);
std::uniform_int_distribution<> dis(3, 6);
std::uniform_real_distribution<double> unif(0, 1);

double sumArrivalTimes=0;
double newArrivalTime;


double poisson()
{
  newArrivalTime=poi.operator() (rng);// generates the next random number in the distribution
  sumArrivalTimes=sumArrivalTimes + newArrivalTime;
  std::cout << "newArrivalTime:  " << newArrivalTime  << "    ,sumArrivalTimes:  " << sumArrivalTimes << std::endl;
  if (sumArrivalTimes<3.6)
  {
    sumArrivalTimes=3.6;
  }
  return sumArrivalTimes;
}

uint32_t  uniformDis()
{
  uint32_t value = dis.operator() (rng);
  std::cout << "Ilha:  " << value << std::endl;
  return value;
}

int zipf(double alpha, int n)
{
	static int first = true;      // Static first time flag
	static double c = 0;          // Normalization constant
	double z;                     // Uniform random number (0 < z < 1)
	int    zipf_value;            // Computed exponential value to be returned
	int    i;                     // Loop counter
	int    low, high, mid;        // Binary-search bounds

	// Compute normalization constant on first call only
	if (first == true)
	{
		sum_probs.reserve(n+1); // Pre-calculated sum of probabilities
		sum_probs.resize(n+1);
		for (i = 1; i <= n; i++)
			c = c + (1.0 / pow((double) i, alpha));
		c = 1.0 / c;

		//sum_probs = malloc((n+1)*sizeof(*sum_probs));
		sum_probs[0] = 0;
		for (i = 1; i <= n; i++) {
			sum_probs[i] = sum_probs[i-1] + c / pow((double) i, alpha);
		}
		first = false;
	}

	// Pull a uniform random number (0 < z < 1)
	do {
		z = unif.operator()(rng);
	} while ((z == 0) || (z == 1));

	//for (i=0; i<=n; i++)
	//{
	//  std::cout << "sum_probs:  " << sum_probs[i] << std::endl;
	//}
	//std::cout << "z:  " << z << std::endl;
	// Map z to the value
	low = 1; high = n;
	do {
		mid = floor((low+high)/2);
		if (sum_probs[mid] >= z && sum_probs[mid-1] < z) {
			zipf_value = mid;
			break;
		} else if (sum_probs[mid] >= z) {
			high = mid-1;
		} else {
			low = mid+1;
		}
	} while (low <= high);
	cout << "ZIPF:  " << zipf_value << endl;
	// Assert that zipf_value is between 1 and N
	assert((zipf_value >=1) && (zipf_value <= n));

	return (zipf_value);
}

#endif
