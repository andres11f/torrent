#include <iostream>
#include <unordered_map>
#include <czmq.h>
#include <list>
#include <vector>

using namespace std;

class Peer {
private:
  string address;
  vector<int> parts;  //0 not, 1 yes
public:
  Peer(string a, vector<int> ps){address = a; parts = ps;}
  string getAddress(){return address;}
  string printParts(){
    int i=0;
    string line = "";
    while(i<parts.size()){
      if (parts[i]==1){
        string s = to_string(i);
        line += s;
      }
      i++;
    }
    return line;
  }
};

class File {
private:
  string name;
  list<Peer> peers;
public:
  File() {}
  File(string n, list<Peer> ps) {name = n; peers = ps;}
  void printFile(){
    cout << "------FILE-----\n";
    cout << "Name: " << name << "\n";
    for (list<Peer>::iterator it = peers.begin(); it != peers.end(); ++it)
      cout << "Peer: " << it->getAddress() << " parts: " << it->printParts() << "\n";
  }
};

unordered_map<string, File> files;

void dispatch(zmsg_t *msg, zmsg_t *response);
bool createFile(char* fileName, char* nParts, char* peerAdr);
void printFiles();


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
    files.begin()->second.printFile();
    free(fileName);
    free(nParts);
    free(peerAdr);
  }
}


bool createFile(char* fileName, char* nParts, char* peerAdr){
  string pa = peerAdr, fName = fileName;
  int np = atoi(nParts);
  vector<int> parts(np,1);
  Peer p = Peer(pa, parts);
  list<Peer> peers (1, p);
  files[fName]=File(fName, peers);
  return true;
}


void printFiles(){
  for (unordered_map<string,File>::iterator it = files.begin(); it != files.end(); ++it)
    it->second.printFile();
}