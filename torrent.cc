#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include <czmq.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;


class File {
private:
  string name;
  vector<int> parts; //partes   0 dont, 1 have it
public:
  File(){}
  File(string n, vector<int> ps){name = n; parts = ps;}
  File(string n, int nParts){name = n; vector<int> ps(nParts,0); parts = ps;}

  string choosePart(){
    srand(time(NULL));
    while(1){
      int n = rand() % parts.size();
      if (parts[n] == 0)
        return to_string(n);
    }
  }

  void partDownloaded(string p){
    int pos = atoi(p.c_str());
    if (parts[pos] == 0)
      parts[pos] = 1;
    else
      return;
  }

  void printFile(){
    cout << "------FILE-----\n";
    cout << "Name: " << name << "\n";
    cout << "Parts: ";
    for (int i = 0; i<parts.size(); i++)
      if (parts[i] == 1)
        cout << i << " ";
  }

  bool checkComplete(){
    for (int i=0; i != parts.size(); i++){
      if (parts[i] == 0)
        return false;
    }
    return true;
  }

  string getPart(int p){
    string file = name, line, filePart = "";
    file += "/";
    file += to_string(p);
    ifstream filestream (file);
    if (filestream.is_open()){
      while ( getline (filestream,line)){
        line+="\n";
        filePart+=line;
      }
    filestream.close();
    }
    return filePart;
  }

  void rebuild(){
    string folder = name, line, nfinalfile = name;
    folder += "/";
    ofstream finalFile(nfinalfile+="_download.txt");
    for (int i = 0; i<parts.size(); i++){
      string tmp = folder;
      ifstream part(tmp+=to_string(i));
      while(getline(part, line)){
        line+="\n";
        finalFile << line;
      }
      part.close();
    }
    finalFile.close();
  }

};


unordered_map<string, File> files;


int createTorrent (string fileName, string trackerAdr, string peerAdr, zctx_t *context);
int download (string torrentName, int maxDownloads, string peerAdr, zctx_t *context);
list<string> obtainPeers(char *p);
int createTorrent(string trackerAdr, string fileName, string tmp);
string defineParts(istream& baseFile, string fileName);
string choosePeer(list<string> peers);


int main () {
  zctx_t *context = zctx_new();
  files.clear();
  string adr;
  int maxDownloads = 42;
  cout << "port to use for listening:";
  cin >> adr;
  string adrtmp = "tcp://*:";
  adrtmp.append(adr);

  void *listener = zsocket_new(context, ZMQ_REP);
  int rc = zsocket_bind (listener, adrtmp.c_str());
  cout << rc << "\n";

  /*cout << "Max Downloads: ";
  cin >> maxDownloads;*/
  string peerAdr = "tcp://localhost:";
  peerAdr.append(adr);

  zmq_pollitem_t items [] = {
    {NULL, STDIN_FILENO, ZMQ_POLLIN, 0},
    {listener, 0, ZMQ_POLLIN, 0}
  };
  
  while(1){
    string input = "";
    cout << "\n1. Download  \n";
    cout << "2. Create torrent \n";
    cout << "0. Exit \n";
    cout << "Selection: ";
    cout.flush();

    zmq_poll(items, 2, -1);

    if (items[0].revents & ZMQ_POLLIN){
      cin >> input;
      if (input == "1"){
        string torrentName;
        cout << "Torrent Name: ";
        cin >> torrentName;
        int r = download(torrentName, maxDownloads, peerAdr, context);
        if (r == -1)
          cout << "ERROR: FILE DOESN'T EXIST";
      }

      else if (input == "2"){
        string fileName, tracker;
        cout << "File:";
        cin >> fileName;
        cout << "Tracker:";
        cin >> tracker;
        int r = createTorrent(fileName, tracker, peerAdr, context);
        if (r == -1)
          cout << "ERROR: FILE DOESN'T EXIST";
      }

      else if (input == "0"){
        return 0;
      }
    }

    if (items[1].revents & ZMQ_POLLIN){
      zmsg_t *msgrecv = zmsg_recv(listener);
      char *op = zmsg_popstr(msgrecv);
      if (strcmp(op, "part") == 0){
        string fileName = zmsg_popstr(msgrecv);
        char *p = zmsg_popstr(msgrecv);
        int part = atoi(p);
        string filePart = files[fileName].getPart(part);
        cout << "\nPart to send: " << part << "\n";
        zmsg_t *msgout = zmsg_new();
        zmsg_addstr(msgout, "success");
        zmsg_addstr(msgout, filePart.c_str());
        zmsg_send(&msgout, listener);
        zmsg_destroy(&msgrecv);
        zmsg_destroy(&msgout);
        free(p); 
      }
    }
  }
  
  zctx_destroy(&context);
  return 0;
}


int createTorrent (string fileName, string trackerAdr, string peerAdr, zctx_t *context){
  zmsg_t *msg = zmsg_new();
  void *tracker = zsocket_new(context, ZMQ_REQ);
  string tmp = fileName, tmptracker, parts;

  //define number of parts
  ifstream baseFile(tmp += ".txt");
  if (!baseFile.is_open())
    return -1;
  parts = defineParts(baseFile, fileName);
  cout << "parts of file "<< fileName << ": " << parts << "\n";
  tmp = fileName;

  //create torrent file
  if (createTorrent(trackerAdr, fileName, tmp) == -1)
    return -1;

  //connection to tracker
  ifstream torrentFileI (tmp += ".torrent");
  getline (torrentFileI,trackerAdr);
  tmptracker = "tcp://localhost:";
  tmptracker.append(trackerAdr);
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
  //create object File in this peer if result is success
  if (strcmp(result, "success") == 0){
    vector<int> ps(atoi(parts.c_str()),1);
    File f = File(fileName, ps);
    f.printFile();
    files[fileName]=f;
  }

  torrentFileI.close();
  free(result);
  zmsg_destroy(&msg);
  zsocket_destroy(context, tracker);
}


int download(string torrentName, int maxDownloads, string peerAdr, zctx_t *context){
  string trackerAdr, tmptracker, fileName;
  zmsg_t *com = zmsg_new();
  void *tracker = zsocket_new(context, ZMQ_REQ);

  //read from file tracker address and file name
  ifstream torrentFile(torrentName+=".torrent");
  if (!torrentFile.is_open())
    return -1;
  getline (torrentFile, tmptracker);
  trackerAdr = "tcp://localhost:";
  trackerAdr.append(tmptracker);
  getline (torrentFile, fileName);

  //connection to tracker
  int rc = zsocket_connect(tracker, trackerAdr.c_str());
  assert (rc == 0);

  //first message to tracker, asking for the number of parts of the file
  string op = "download";
  zmsg_addstr(com, op.c_str());
  zmsg_addstr(com, fileName.c_str());
  zmsg_dump(com);
  zmsg_send(&com, tracker);
  assert (com == NULL);
  com = zmsg_recv(tracker);

  char *result = zmsg_popstr(com);
  char *np = zmsg_popstr(com);    //number of parts
  if (strcmp(result, "failure") == 0)
    return -1;
  int nParts = atoi(np);
  cout << "Number of parts of file " << fileName << ": " << nParts << "\n";
  File file = File(fileName, nParts);
  free(result);
  free(np);

  //creation of temporary folder for parts
  string folder = fileName;
  folder+="_downloading";
  if (mkdir(folder.c_str(),0777) == -1){
    cerr<<"Error : "<<strerror(errno)<<endl;
    exit(1);
  }

  while(!file.checkComplete()){
    zmsg_t *msg = zmsg_new();
    string part = file.choosePart();    //part to ask to tracker

    //without concurrence asking who has x part
    op = "askpart";
    zmsg_addstr(msg, op.c_str());
    zmsg_addstr(msg, fileName.c_str());
    zmsg_addstr(msg, peerAdr.c_str());
    zmsg_addstr(msg, part.c_str());
    zmsg_send(&msg, tracker);
    assert (msg == NULL);
    msg = zmsg_recv(tracker);

    result = zmsg_popstr(msg);
    char *p = zmsg_popstr(msg);
    if (strcmp(result, "failure") == 0)
      return -1;
    //list of peers with part, in one string
    list<string> peers = obtainPeers(p);
    //print peers
    cout << "\nPeers with part " << part << ": \n";
    for (list<string>::iterator it = peers.begin(); it != peers.end(); ++it)
      cout << (*it) << "\n";
    free(result);
    free(p);
    /*
    zmsg_destroy(&msg);
    zsocket_destroy(context, tracker);
    */

    //connection to peer
    void *peerSocket = zsocket_new(context, ZMQ_REQ);
    string chosenPeer = choosePeer(peers);
    cout << "Chosen peer: " << chosenPeer << "\n";
    rc = zsocket_connect(peerSocket, chosenPeer.c_str());
    assert (rc == 0);

    //download part
    zmsg_t *peermsg = zmsg_new();
    op = "part";
    zmsg_addstr(peermsg, op.c_str());
    zmsg_addstr(peermsg, fileName.c_str());
    zmsg_addstr(peermsg, part.c_str());
    zmsg_send(&peermsg, peerSocket);
    assert (peermsg == NULL);
    peermsg = zmsg_recv(peerSocket);

    result = zmsg_popstr(peermsg);
    char *fp = zmsg_popstr(peermsg);
    string filePart = fp;
  
    //part to folder
    string tmp = folder+="/";
    ofstream partStream(tmp+=part);
    partStream << filePart;
    partStream.close();
    file.partDownloaded(part);
    file.printFile();

    zmsg_destroy(&peermsg);
    zmsg_destroy(&msg);
    zsocket_destroy(context, peerSocket);
  }
  file.rebuild();
  zmsg_destroy(&com);
  zsocket_destroy(context, tracker);
}


//obtain peers from tracker
list<string> obtainPeers(char *p){
  string sp = p, tmp = "";
  list<string> peers;
  for (int i=0; i<sp.size(); i++){
    if (sp[i] != '+')
      tmp.push_back(sp[i]);
    else{
      peers.push_back(tmp);
      tmp="";
    }
  }
  return peers;
}


int createTorrent(string trackerAdr, string fileName, string tmp){
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
    return -1;
  }
}

string defineParts(istream& baseFile, string fileName){
  string parts, line, folder = fileName, npart, tmp;
  int cont = 0, contParts = 0;
  npart = to_string(contParts);
  if (mkdir(folder.c_str(),0777) == -1){
    cerr<<"Error :  "<<strerror(errno)<<endl;
    exit(1);
  }
  tmp = folder+="/";
  ofstream part(tmp+=npart);
  while(getline(baseFile, line)){
    cont++;
    line+="\n";
    part << line;
    if (cont == 256){
      contParts++;
      cont = 0;
      tmp.clear();
      npart.clear();
      tmp = folder;
      npart = to_string(contParts);
      part.close();
      part.open(tmp+=npart);
    }
  }
  contParts++;
  part.close();
  parts = to_string(contParts);
  return parts;
}


string choosePeer(list<string> peers){
  //srand(time(NULL));
  int n = rand() % peers.size();
  int i = 0;
  cout << "\nnumero aleatorio: " << n << " peers size: " << peers.size() << "\n";
  list<string>::iterator it = peers.begin();
  while (i < n){
    ++it;
    i++;
  }
  cout << "valor: " << (*it) << "\n";
  return (*it);
}