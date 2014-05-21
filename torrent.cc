#include <iostream>
#include <fstream>
#include <string>
#include <czmq.h>

using namespace std;


void createTorrent (string fileName, string trackerAdr, string peerAdr, zctx_t *context);
string defineParts(istream& baseFile);


int main () {
  zctx_t *context = zctx_new();
  string adr;
  cout << "port to use for listening:";
  cin >> adr;
  string peerAdr = "tcp://*:";
  peerAdr.append(adr);

  while(1){
    string input = "";
    cout << "1. Download  \n";
    cout << "2. Create torrent \n";
    cout << "0. Exit \n";
    cout << "Selection: ";
    cout.flush();
    cin >> input;

    if (input == "1"){
    }

    else if (input == "2"){
      string fileName, tracker;
      cout << "File:";
      cin >> fileName;
      cout << "Tracker:";
      cin >> tracker;
      createTorrent(fileName, tracker, peerAdr, context);
    }

    else if (input == "0"){
      return 0;
    }
  }

  zctx_destroy(&context);
  return 0;
}


void createTorrent (string fileName, string trackerAdr, string peerAdr, zctx_t *context){
  zmsg_t *msg = zmsg_new();
  string tmp = fileName, line, tmptracker = trackerAdr, parts;
  trackerAdr = "tcp://localhost:";
  trackerAdr.append(tmptracker);

  //define number of parts
  ifstream baseFile(tmp += ".txt");
  parts = defineParts(baseFile);
  cout << "parts of file "<< fileName << ": " << parts << "\n";
  tmp = fileName;

  //create torrent file
  ofstream torrentFileO (tmp += ".torrent");
  if (baseFile.is_open()){
    torrentFileO << trackerAdr;
    torrentFileO << "\n";
    torrentFileO << fileName;
    baseFile.close();
    torrentFileO.close();
  }
  else{
    cout << "Error: No File \n";
    return;
  }

  //connection to tracker
  ifstream torrentFileI (tmp += ".torrent");
  void *tracker = zsocket_new(context, ZMQ_REQ);
  getline (torrentFileI,line);
  int rc = zsocket_connect (tracker, trackerAdr.c_str());
  assert(rc == 0);

  //create File object in tracker
  string op = "newtorrent";
  zmsg_addstr(msg, op.c_str());
  zmsg_addstr(msg, fileName.c_str());
  zmsg_addstr(msg, parts.c_str());
  zmsg_addstr(msg, peerAdr.c_str());
  zmsg_dump(msg);
  zmsg_send(&msg, tracker);
  assert (msg == NULL);
  msg = zmsg_recv(tracker);

  char *result = zmsg_popstr(msg);
  cout << result << "\n";

  torrentFileI.close();
  free(result);
  zmsg_destroy(&msg);
  zsocket_destroy(context, tracker);
}


string defineParts(istream& baseFile){
  string parts, line;
  int cont=0, tmp;
  while(getline(baseFile, line))
    cont++;
  cout << "Number of lines: " << cont << "\n";
  tmp = cont/256;
  if (cont%256>0)
    tmp=tmp+1;
  parts = to_string(tmp);
  return parts;
}