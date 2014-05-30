#include <iostream>
#include <unordered_map>
#include <czmq.h>
#include <vector>

using namespace std;


class File {
private:
  string name;
  unordered_map<string, vector<int>> peers; //ip, vector 0 dont, 1 have it
public:
  File() {}
  File(string n, unordered_map<string, vector<int>> ps) {name = n; peers = ps;}
  void printFile(){
    cout << "------FILE-----\n";
    cout << "Name: " << name << "\n";
    for (unordered_map<string, vector<int>>::iterator it = peers.begin(); it != peers.end(); ++it)
      cout << "Peer: " << it->first << " parts: " << printParts(it->first) << "\n";
  }
  string printParts(string peer){
    int i=0;
    string line = "";
    vector<int> parts = peers[peer];
    while(i<parts.size()){
      if (parts[i]==1){
        string s = to_string(i);
        s += " ";
        line += s;
      }
      i++;
    }
    return line;
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
    printFiles();
    free(fileName);
    free(nParts);
    free(peerAdr);
  }
}


bool createFile(char* fileName, char* nParts, char* peerAdr){
  string pa = peerAdr, fName = fileName;
  int np = atoi(nParts);
  vector<int> parts(np,1);
  unordered_map<string,vector<int>> umFile;
  umFile[fName]=parts;
  files[fName]=File(fName, umFile);
  return true;
}


void printFiles(){
  for (unordered_map<string,File>::iterator it = files.begin(); it != files.end(); ++it)
    it->second.printFile();
}