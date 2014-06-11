#include <iostream>
#include <unordered_map>
#include <czmq.h>
#include <vector>
#include <stdlib.h>

using namespace std;


class File {
private:
  string name;
  unordered_map<string, vector<int>> peers; //ip, vector 0 dont, 1 have it
public:
  File() {}
  File(string n, unordered_map<string, vector<int>> ps) {name = n; peers = ps;}
  int getNumberParts(){return peers.begin()->second.size();}

  void printFile(){
    cout << "------FILE-----\n";
    cout << "Name: " << name << "\n";
    for (unordered_map<string, vector<int>>::iterator it = peers.begin(); it != peers.end(); ++it)
      cout << "Peer: " << it->first << " parts: " << printParts(it->first) << "\n";
  }
  string printParts(string peer){
    unsigned int i=0;
    string line = "";
    vector<int> parts = peers[peer];
    while(i<parts.size()){
      if (parts[i] == 1){
        string s = to_string(i);
        s += " ";
        line += s;
      }
      i++;
    }
    return line;
  }
  string peersWithParts(string ownPeer){
    string peersList = "";
    for (unordered_map<string, vector<int>>::iterator it = peers.begin(); it != peers.end(); ++it){
      if (it->first != ownPeer){
        peersList += it->first;
        peersList += '-';
        for (unsigned int i = 0; i < it->second.size(); i++)
          peersList += to_string(it->second[i]);
        peersList += '+';
      }
    }
    return peersList;
  }

  void addPeerPart(string peer, int part){
    int nParts = peers.begin()->second.size();
    vector<int> tmp(nParts, 0);
    if (peers.count(peer) > 0){
      tmp = peers[peer];
      if (tmp[part] == 0)
        tmp[part] = 1;
      peers[peer] = tmp;
    }
    else{
      if (tmp[part] == 0)
        tmp[part] = 1;
      peers[peer] = tmp;
    }
  }
};


unordered_map<string, File> files;


void dispatch(zmsg_t *msg, zmsg_t *response);
bool createFile(char* fileName, char* nParts, char* peerAdr);


int main () {
  zctx_t *context = zctx_new();

  string adr;
  cout << "tracker address:";
  cin >> adr;
  string trackerAdr = "tcp://*:";
  trackerAdr.append(adr);
  void *responder = zsocket_new(context, ZMQ_REP);
  int pn = zsocket_bind(responder, trackerAdr.c_str());
  cout << "Port number " << pn << "\n";

  while(1){
    zmsg_t *msg = zmsg_new();
    cout << "waiting... \n";
    msg = zmsg_recv(responder);
    zmsg_dump(msg);
    zmsg_t *response = zmsg_new();
    dispatch(msg, response);
    zmsg_destroy(&msg);
    zmsg_send(&response, responder);
    zmsg_destroy(&response);
  }

  zsocket_destroy(context, responder); 
  zctx_destroy(&context);
  return 0;
}


void dispatch(zmsg_t *msg, zmsg_t *response){
  assert(zmsg_size(msg) >= 1); //zmsg_size da el tamaño en numero de frames
  char *op = zmsg_popstr(msg);

  if (strcmp(op, "newtorrent") == 0){
    char *fileName = zmsg_popstr(msg);
    char *nParts = zmsg_popstr(msg);
    char *peerAdr = zmsg_popstr(msg);
    if (createFile (fileName, nParts, peerAdr))
      zmsg_addstr(response, "success");
    else
      zmsg_addstr(response, "failure");

    //print files
    for (unordered_map<string,File>::iterator it = files.begin(); it != files.end(); ++it)
      it->second.printFile();

    free(fileName);
    free(nParts);
    free(peerAdr);
  }

  if (strcmp(op, "download") == 0){
    //first message received asking for number of parts
    string fileName = zmsg_popstr(msg);
    if (files.find(fileName) != files.end())
      zmsg_addstr(response, "success");
    else
      zmsg_addstr(response, "failure");
    string nParts = to_string(files[fileName].getNumberParts());
    zmsg_addstr(response, nParts.c_str());
    cout << "The number of parts of file " << fileName << " is: " << nParts << "\n";
  }

  if (strcmp(op, "askparts") == 0){
    //message asking for who has x part
    string fileName = zmsg_popstr(msg);
    string peerAdr = zmsg_popstr(msg);

    //generate list of peers with parts of file
    string peersList = files[fileName].peersWithParts(peerAdr);
    cout << "Raw peers list of file " << fileName << ": " << peersList << "\n";
    if (peersList == "")
      zmsg_addstr(response, "failure");
    else
      zmsg_addstr(response, "success");
    zmsg_addstr(response, peersList.c_str());
  }

    //add information about peer now having the part
  if (strcmp(op, "haspart") == 0){
    string fileName = zmsg_popstr(msg);
    string peerAdr = zmsg_popstr(msg);
    char *part = zmsg_popstr(msg);

    files[fileName].addPeerPart(peerAdr, atoi(part));

    zmsg_addstr(response, "success");

    //print files
    for (unordered_map<string,File>::iterator it = files.begin(); it != files.end(); ++it)
      it->second.printFile();
    free(part);
  }
  free(op);
}


bool createFile(char* fileName, char* nParts, char* peerAdr){
  string pa = peerAdr, fName = fileName;
  int np = atoi(nParts);
  vector<int> parts(np,1);
  unordered_map<string,vector<int>> umFile;
  umFile[peerAdr]=parts;
  files[fName]=File(fName, umFile);
  return true;
}