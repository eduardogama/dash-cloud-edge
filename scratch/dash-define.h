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

#ifndef DASH_DEFINE_HH_
#define DASH_DEFINE_HH_

#include <iostream>
#include <fstream>
#include <string>

using namespace std;


//source, destination, rate_bps, delay_ms, pakt_loss, buffersize_pkts
class _Link{
	public:
		_Link(int src_id, int dst_id, double rate, double delay, double ploss, int buffersize_pkts){
			_src_id			 = src_id;
			_dst_id			 = dst_id;
			_rate            = rate;
			_delay			 = delay;
			_ploss			 = ploss;
			_buffersize_pkts = buffersize_pkts;
		}
		int getSrcId() { return _src_id;}
		int getDstId() { return _dst_id;}
		double getRate() { return _rate;}
		double getDelay() { return _delay;}
		double getPLoss() { return _ploss;}
		int getBufferSize() { return _buffersize_pkts;}
		void setSrcId(int src_id){_src_id=src_id;}
		void setDstId(int dst_id){_dst_id=dst_id;}
		void setRate(double rate){_rate=rate;}
		void setDelay(double delay){_delay=delay;}
		void setPLoss(double ploss){_ploss=ploss;}
		void setBufferSize(int buffersize_pkts){_buffersize_pkts=buffersize_pkts;}
	private:
		int _src_id;
		int _dst_id;
		double _rate;
		double _delay;
		double _ploss;
		int _buffersize_pkts;
};

//id, type, AdaptationLogic, StartUpDelay, AllowDownscale, AllowUpscale, MaxBufferedSeconds
class _Node {
	public:
		_Node(int id, string type, string AdaptationLogic, string StartUpDelay, string AllowDownscale, string AllowUpscale, string MaxBufferedSeconds){
            _id=id;
			_type=type;
            _AdaptationLogic=AdaptationLogic;
            _StartUpDelay=StartUpDelay;
		
			if(AllowDownscale.compare("yes")==0) 
				_AllowDownscale=true;
			else 
				_AllowDownscale=false;
            
            if(AllowUpscale.compare("yes")==0) 
            	_AllowUpscale=true;
            else 
            	_AllowUpscale=false;
            _MaxBufferedSeconds=MaxBufferedSeconds;
        }
        _Node(int id, string type){
            _id   = id;
			_type = type;
        }
        int getId() { return _id;}
        string getType() { return _type;}
        string getAdaptationLogic() { return _AdaptationLogic;}
        string getStartUpDelay() { return _StartUpDelay;}
        bool getAllowDownscale() { return _AllowDownscale;}
        bool getAllowUpscale() { return _AllowUpscale;}
        string getMaxBufferedSeconds() { return _MaxBufferedSeconds;}
        void setId(int id){_id=id;}
        void setType(string type){_type=type;}
        void setAdaptationLogic(string AdaptationLogic){_AdaptationLogic=AdaptationLogic;}
        void setStartUpDelay(string StartUpDelay ){_StartUpDelay=StartUpDelay;}
        void setAllowDownscale(bool AllowDownscale){_AllowDownscale=AllowDownscale;}
        void setAllowUpscale(bool AllowUpscale){_AllowUpscale=AllowUpscale;}
        void setMaxBufferedSeconds(string MaxBufferedSeconds){_MaxBufferedSeconds=MaxBufferedSeconds;}
    private:
        int _id;
		string _type;
        string _AdaptationLogic;
        string _StartUpDelay;
        bool _AllowDownscale;
        bool _AllowUpscale;
        string _MaxBufferedSeconds;
};

//id, source, startsAt, stopsAt, videoId, screenWidth, screenHeight
class _Request{
    public:
        _Request(int id, int src_id, int server_id, double startsAt, double stopsAt, int videoId, int screenWidth, int screenHeight){
            _id=id;
            _src_id=src_id;
			_server_id=server_id;
            _startsAt=startsAt;
            _stopsAt=stopsAt;
            _videoId=videoId;
            _screenWidth=screenWidth;
            _screenHeight=screenHeight;
        }
		int getId() { return _id;}
        int getSrcId() { return _src_id;}
        int getServerId() { return _server_id;}
        double getStartsAt() { return _startsAt;}
        double getStopsAt() { return _stopsAt;}
        int getVideoId() { return _videoId;}
        int getScreenWidth() { return _screenWidth;}
        int getScreenHeight() { return _screenHeight;}
        void setId(int id){_id=id;}
        void setSrcId(int src_id){_src_id=src_id;}
        void setServerId(int server_id){_server_id=server_id;}
        void setStartsAt(double startsAt){_startsAt=startsAt;}
        void setStopsAt(double stopsAt){_stopsAt=stopsAt;}
        void setVideoId(int videoId){_videoId=videoId;}
        void setScreenWidth(int screenWidth){_screenWidth=screenWidth;}
        void setScreenHeight(int screenHeight){_screenHeight=screenHeight;}
    private:
        int _id;
        int _src_id;
		int _server_id;
        double _startsAt;
        double _stopsAt;
        int _videoId;
        int _screenWidth;
        int _screenHeight;
};

//source, destination, hops
class _Route{
    public:
        _Route(int src_id, int dst_id, double bw_alloc, vector<int> & hops){
            _src_id=src_id;
            _dst_id=dst_id;
			_bw_alloc=bw_alloc;
            _hops=hops;
        }
        int getSrcId(){ return _src_id;}
        int getDstId(){ return _dst_id;}
        double getBwAlloc(){ return _bw_alloc;}
        vector<int> getHops() { return _hops;}
        void setSrcId(int src_id){_src_id=src_id;}
        void setDstId(int dst_id){_dst_id=dst_id;}
        void setBwAlloc(double bw_alloc){_bw_alloc=bw_alloc;}
        void setHops(vector<int> hops){_hops=hops;}
    private:
        int _src_id;
        int _dst_id;
        double _bw_alloc;
        vector<int> _hops;
};

#endif
