#ifndef GROUP_USER_HH_
#define GROUP_USER_HH_


#include <string>

#include "path.h"

using namespace std;



class EndUser
{
	public:
		EndUser(){}
		EndUser(int _id, string _ip, int _cont){
			this->stream_id = _id;
			this->user_ip = _ip;
			this->content = _cont;
		}

		void setId(unsigned int _id) {this->stream_id = _id;}
		unsigned int getId() {return this->stream_id;}

		void setIp(string _ip) {this->user_ip = _ip;}
		string getIp() {return this->user_ip;}

		void setContent(int c){this->content = c;}
		int getContent(){return this->content;}

	private:

		unsigned int stream_id;
		string user_ip;

		int content;
};

class GroupUser
{
	public:
		GroupUser() {}

		GroupUser(string _id, string serverIp, unsigned from, unsigned to, Path route,int content, EndUser *user) {
			this->id       = _id;
			this->serverIp = serverIp;
			this->ap 			 = from;
			this->from		 = from;
			this->to			 = to;
			this->route    = route;
			this->content  = content;
			this->users.push_back(user);
		}

		virtual ~GroupUser() {}

		void setId(string id) {this->id = id;}
		string getId() {return this->id;}

		void addUser(EndUser *user) {this->users.push_back(user);}
		vector<EndUser *> getUsers() {return this->users;}

		void setRoute(Path route) {this->route = route;}
		Path &getRoute(void) {return this->route;}

		void setAp(unsigned ap) {this->ap = ap;}
		unsigned getAp() {return this->ap;}

		void setActualNode(unsigned actualNode) {this->actualNode = actualNode;}
		unsigned getActualNode() {return this->actualNode;}

		void setServerIp(string serverIp) {this->serverIp = serverIp;}
		string getServerIp() {return this->serverIp;}

		void setFrom(unsigned from) {this->from = from;}
		unsigned getFrom() {return this->from;}

		void setTo(unsigned to) {this->to = to;}
		unsigned getTo() {return this->to;}

		void setContent(int c){this->content = c;}
		int getContent(){return this->content;}

	private:
		string id;
		Path route;
		string serverIp;
		int content;

		vector<EndUser *> users;

		int ap;
		unsigned from;
		unsigned to;
		unsigned actualNode;

};

#endif // GROUP_USER_HH_
