//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%                   QoE-aware Routing Computation                  %%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                                                                    %
// Code for solving the QoE-aware routing problem as the              %
// network evolves. The code can be called by ns3 to admit            %
// a new demand, reconfigure the network according to its status and  %
// the QoE experiend by e2e connections [1].                          %
//                                                                    %
// Created by                                                         %
// - Paris Reseach Center, Huawei Technologies Co. Ltd.               %
// - Laboratoire d'Informatique, Signaux et Systèmes de               %
//   Sophia Antipolis (I3S) Universite Cote d'Azur and CNRS           %
//                                                                    %
// Contributors:                                                      %
// - Giacomo CALVIGIONI (I3S)                                         %
// - Ramon APARICIO-PARDO (I3S)                                       %
// - Lucile SASSATELLI (I3S)                                          %
// - Jeremie LEGUAY (Huawei)                                          %
// - Stefano PARIS (Huawei)                                           %
// - Paolo MEDAGLIANI (Huawei)                                        %
//                                                                    %
// References:                                                        %
// [1] Giacomo Calvigioni, Ramon Aparicio-Pardo, Lucile Sassatelli,   %
//     Jeremie Leguay, Stefano Paris, Paolo Medagliani,               %
//     "Quality of Experience-based Routing of Video Traffic for      %
//      Overlay and ISP Networks". In the Proceedings of the IEEE     %
//     International Conference on Computer Communications            %
//      (INFOCOM), 15-19 April 2018.                                  %
//                                                                    %
// Contacts:                                                          %
// - For any question please use the address: qoerouting@gmail.com    %
//                                                                    %
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


#ifndef DASH_UTILS_HH_
#define DASH_UTILS_HH_

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <iomanip>      // std::setprecision


#include "dash-define.h"

using namespace std;

bool endsWith(const std::string& s, const std::string& suffix)
{
    return s.size() >= suffix.size() &&
           s.substr(s.size() - suffix.size()) == suffix;
}

std::vector<std::string> split(const std::string& s, const std::string& delimiter, const bool& removeEmptyEntries = false)
{
    std::vector<std::string> tokens;

    for (size_t start = 0, end; start < s.length(); start = end + delimiter.length()) {
         size_t position = s.find(delimiter, start);
         end = position != string::npos ? position : s.length();

         std::string token = s.substr(start, end - start);
         if (!removeEmptyEntries || !token.empty()) {
             tokens.push_back(token);
         }
    }

    if (!removeEmptyEntries && (s.empty() || endsWith(s, delimiter))) {
        tokens.push_back("");
    }

    return tokens;
}

string has_algorithm(string const algo)
{
   if (algo == "hybrid") {
       return "RateAndBufferBasedAdaptationLogic";
   } else if (algo == "rate") {
       return "RateBasedAdaptationLogic";
   } else {
       return "BufferBasedAdaptationLogic";
   }
}

bool io_read_scenario_requests(string requestssFile, vector<_Request *> & requests)
{
	int linecount;

	// requests data
	int id;
	int src_id;
	int server_id = 0;
  double startsAt;
  double stopsAt;
  int videoId;
  int screenWidth;
  int screenHeight;

	// Read from Requests
	// id, source, startsAt, stopsAt, videoId, screenWidth, screenHeight
#ifdef DEBUG_VERBOSE_
	cout << "*************************************************" << endl;
	cout << "Start reading requests from file... " << endl;
	cout << "*************************************************" << endl;
#endif
	ifstream requests_file(requestssFile.c_str());
	linecount = 0;
	while (requests_file) {
		string line;
		linecount++;
		if (!getline( requests_file, line )) {
			break;
		}
		if (linecount > 1) {
			// Get each element of the demand
			istringstream linestream(line);
			int columncount = 1;
			while (linestream) {
				string s;
				if (!getline(linestream, s, ',' )) {
					break;
				}
				switch (columncount) {
					case 1:
		                id = std::atoi(s.c_str());
		                break;
					case 2:
						src_id = std::atoi(s.c_str());
						break;
		            case 3:
		                server_id = std::atoi(s.c_str());
		                break;
					case 4:
						startsAt = std::atof(s.c_str());
						break;
		        	case 5:
		                stopsAt = std::atof(s.c_str());
						break;
		            case 6:
		                videoId = std::atoi(s.c_str());
						break;
		            case 7:
		                screenWidth = std::atoi(s.c_str());;
						break;
		            case 8:
		                screenHeight = std::atoi(s.c_str());
		                break;
				}
				columncount++;
			}
			requests.push_back(new _Request(id, src_id, server_id,  startsAt, stopsAt, videoId, screenWidth, screenHeight));
		}
	}
	requests_file.close();

	return true;
}


bool io_read_topology(string linksFile, string nodesFile, vector<_Link *> & links, vector<_Node *> & nodes)
{
	int linecount;
	// std::string::size_type  idxStr2Double;

	// link data
    int src_id;
    int dst_id;
    double rate;
    double delay;
    double ploss;
    double buffersize_pkts;

	// Read from adj_matrix: source, destination, rate_bps, delay_ms, pakt_loss, buffersize_pkts
#ifdef DEBUG_VERBOSE_
	cout << "*************************************************" << endl;
	cout << "Start reading links from file..." << endl;
	cout << "*************************************************" << endl;
#endif

	ifstream links_file(linksFile.c_str());
	linecount = 0;
	while (links_file) {
		string line;
		linecount++;

		if (!getline( links_file, line )) {
			break;
		}
		if(line == "\n") {
			break;
		}

		cout << line << endl;

		if (linecount > 1) {
			// Get each element of this line (each property of the link)
			istringstream linestream( line );
			int columncount = 1;
			while (linestream) {
				string s;
				if (!getline( linestream, s, ' ' )) {
					break;
				}

				switch (columncount) {
					case 1:
						src_id = std::atoi(s.c_str());
						break;
					case 2:
						dst_id = std::atoi(s.c_str());
						break;
					case 3:
						rate = std::atof(s.c_str());
						break;
					case 4:
						delay = std::atof(s.c_str());
						break;
					case 5:
						ploss = std::atof(s.c_str());
						break;
					case 6:
						buffersize_pkts = std::atof(s.c_str());
						break;
				}
				columncount++;
			}
			links.push_back(new _Link(src_id, dst_id, rate, delay, ploss, buffersize_pkts));
		}
	}
	links_file.close();


	// node data
    int id;
	string type;
    string AdaptationLogic;
    string StartUpDelay;
    string AllowDownscale;
    string AllowUpscale;
    string MaxBufferedSeconds;

	// id, type, AdaptationLogic, StartUpDelay, AllowDownscale, AllowUpscale, MaxBufferedSeconds
	cout << "*************************************************" << endl;
	cout << "Start reading nodes from file... " << endl;
	cout << "*************************************************" << endl;

	ifstream nodes_file(nodesFile.c_str());
	linecount = 0;
	while (nodes_file) {
		string line;
		linecount++;
		if (!getline( nodes_file, line )) {
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
				case 3:
					if (type.compare("client")==0)
						AdaptationLogic = s;
					else
						AdaptationLogic=string("");
					break;
				case 4:
					if (type.compare("client")==0)
						StartUpDelay = s;
					else
						StartUpDelay=string("");
					break;
				case 5:
					if (type.compare("client")==0)
						AllowDownscale = s;
					else
						AllowDownscale=string("no");
					break;
				case 6:
					if (type.compare("client")==0)
						AllowUpscale = s;
					else
						AllowUpscale=string("no");
					break;
				case 7:
					if (type.compare("client")==0)
						MaxBufferedSeconds = s;
					else
						MaxBufferedSeconds=string("");
					break;
				}
				columncount++;
			}
			nodes.push_back(new _Node(id, type, AdaptationLogic, StartUpDelay, AllowDownscale, AllowUpscale, MaxBufferedSeconds));
		}
	}
	nodes_file.close();

	return true;
}


bool io_read_scenario(string linksFile, string nodesFile, string requestssFile, string routesFile, vector<_Link *> & links, vector<_Node *> & nodes, vector<_Request *> & requests, vector<_Route *> &routes)
{
	int linecount;
	// std::string::size_type  idxStr2Double;

	// link data
    int src_id;
    int dst_id;
    double rate;
    double delay;
    double ploss;
    double buffersize_pkts;

	// node data
    int id;
	string type;
    string AdaptationLogic;
    string StartUpDelay;
    string AllowDownscale;
    string AllowUpscale;
    string MaxBufferedSeconds;

	// requests data
	//int src_id;
	int server_id;
    double startsAt;
    double stopsAt;
    int videoId;
    int screenWidth;
    int screenHeight;

	// route data
	double _bw_alloc;
    vector<int> _hops;

	// Read from adj_matrix: source, destination, rate_bps, delay_ms, pakt_loss, buffersize_pkts
#ifdef DEBUG_VERBOSE_
	cout << "*************************************************" << endl;
	cout << "Start reading links from file..." << endl;
	cout << "*************************************************" << endl;
#endif
	ifstream links_file(linksFile.c_str());
	linecount = 0;
	while (links_file) {
		string line;
		linecount++;
		if (!getline( links_file, line )) {
			break;
		}
		if (linecount > 1) {
			// Get each element of this line (each property of the link)
			istringstream linestream( line );
			int columncount = 1;
			while (linestream) {
				string s;
				if (!getline( linestream, s, ' ' )) {
					break;
				}

				switch (columncount) {
					case 1:
						src_id = std::atoi(s.c_str());
						break;
					case 2:
						dst_id = std::atoi(s.c_str());
						break;
					case 3:
						rate = std::atof(s.c_str());
						break;
					case 4:
						delay = std::atof(s.c_str());
						break;
					case 5:
						ploss = std::atof(s.c_str());
						break;
					case 6:
						buffersize_pkts = std::atof(s.c_str());
						break;
				}
				columncount++;
			}
			links.push_back(new _Link(src_id, dst_id, rate, delay, ploss, buffersize_pkts));
		}
	}
	links_file.close();

	// Read from Nodes
	// id, type, AdaptationLogic, StartUpDelay, AllowDownscale, AllowUpscale, MaxBufferedSeconds
#ifdef RSP_DEBUG_VERBOSE_
	cout << "*************************************************" << endl;
	cout << "Start reading nodes from file... " << endl;
	cout << "*************************************************" << endl;
#endif
	ifstream nodes_file(nodesFile.c_str());
	linecount = 0;
	while (nodes_file) {
		string line;
		linecount++;
		if (!getline( nodes_file, line )) {
			break;
		}
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
				case 3:
					if (type.compare("client")==0)
						AdaptationLogic = s;
					else
						AdaptationLogic=string("");
					break;
				case 4:
					if (type.compare("client")==0)
						StartUpDelay = s;
					else
						StartUpDelay=string("");
					break;
				case 5:
					if (type.compare("client")==0)
						AllowDownscale = s;
					else
						AllowDownscale=string("no");
					break;
				case 6:
					if (type.compare("client")==0)
						AllowUpscale = s;
					else
						AllowUpscale=string("no");
					break;
				case 7:
					if (type.compare("client")==0)
						MaxBufferedSeconds = s;
					else
						MaxBufferedSeconds=string("");
					break;
				}
				columncount++;
			}
			nodes.push_back(new _Node(id, type, AdaptationLogic, StartUpDelay, AllowDownscale, AllowUpscale, MaxBufferedSeconds));
		}
	}
	nodes_file.close();


	// Read from Requests
	// id, source, startsAt, stopsAt, videoId, screenWidth, screenHeight
#ifdef DEBUG_VERBOSE_
	cout << "*************************************************" << endl;
	cout << "Start reading requests from file... " << endl;
	cout << "*************************************************" << endl;
#endif
	ifstream requests_file(requestssFile.c_str());
	linecount = 0;
	while (requests_file) {
		string line;
		linecount++;
		if (!getline( requests_file, line )) {
			break;
		}
		if (linecount > 1) {
			// Get each element of the demand
			istringstream linestream(line);
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
						src_id = std::atoi(s.c_str());
						break;
		            case 3:
		                server_id = std::atoi(s.c_str());
		                break;
					case 4:
						startsAt = std::atof(s.c_str());
						break;
		        	case 5:
		                stopsAt = std::atof(s.c_str());
						break;
		            case 6:
		                videoId = std::atoi(s.c_str());
						break;
		            case 7:
		                screenWidth = std::atoi(s.c_str());;
						break;
		            case 8:
		                screenHeight = std::atoi(s.c_str());
		                break;
				}
				columncount++;
			}
			requests.push_back(new _Request(id, src_id, server_id,  startsAt, stopsAt, videoId, screenWidth, screenHeight));
		}
	}
	requests_file.close();

    // Read from Routes
    // src dst hops
#ifdef DEBUG_VERBOSE_
    cout << "*************************************************" << endl;
    cout << "Start reading routes from file... " << endl;
    cout << "*************************************************" << endl;
#endif
    ifstream routes_file(routesFile.c_str());
    linecount = 0;
    while (routes_file) {
        string line;
        linecount++;
        if (!getline( routes_file, line )) {
            break;
        }
        if (linecount > 1) {
            // Get each element of the demand
            istringstream linestream(line);
            int columncount = 1;
			_hops.clear();
            while (linestream) {
                string s;
                if (!getline( linestream, s, ' ' )) {
                        break;
                }
                switch (columncount) {
		            case 1:
	                    src_id = std::atoi(s.c_str());
						_hops.push_back(src_id);
	                    break;
		            case 2:
	                    dst_id = std::atoi(s.c_str());
	                    break;
		            case 3:
	                    _bw_alloc = std::atof(s.c_str());
	                    break;
                	default:
						if (columncount>2) _hops.push_back(std::atoi(s.c_str()));
                        break;
                }
                columncount++;
            }
			_hops.push_back(dst_id);
            routes.push_back(new _Route(src_id, dst_id, _bw_alloc, _hops));
        }
    }
    routes_file.close();

	return true;
}

bool io_read_scenario_routes(string routesFile, vector<_Route *> &routes)
{
	int linecount;
	// std::string::size_type  idxStr2Double;

	// route data
    int src_id;
    int dst_id;
	double _bw_alloc;
	vector<int> _hops;

    // Read from Routes
    // src dst hops
#ifdef DEBUG_VERBOSE_
    cout << "*************************************************" << endl;
    cout << "Start reading routes from file... " << endl;
    cout << "*************************************************" << endl;
#endif
    ifstream routes_file(routesFile.c_str());
    linecount = 0;
    while (routes_file) {
        string line;
        linecount++;
        if (!getline( routes_file, line )) {
            break;
        }
        if (linecount > 1) {
            // Get each element of the demand
            istringstream linestream(line);
            int columncount = 1;
			_hops.clear();
            while (linestream) {
                string s;
                if (!getline( linestream, s, ' ' )) {
                    break;
                }
                switch (columncount) {
		            case 1:
		                src_id = std::atoi(s.c_str());
						_hops.push_back(src_id);
			            break;
		            case 2:
		                dst_id = std::atoi(s.c_str());
		                break;
		            case 3:
		                _bw_alloc = std::atof(s.c_str());
		                break;
		            default:
						if (columncount>2) _hops.push_back(std::atoi(s.c_str()));
		                break;
                }
                columncount++;
            }
			_hops.push_back(dst_id);
            routes.push_back(new _Route(src_id, dst_id, _bw_alloc, _hops));
        }
    }
    routes_file.close();

	return true;
}

#endif
