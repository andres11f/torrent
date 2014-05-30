#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <czmq.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;


class File {
private:
  string name;
  unordered_map<string, vector<int>> peers; //ip, partes
  //after download starts
  vector<int> downloadedParts;
public:
  File(string n, unordered_map<string, vector<int>> ps){name = n; peers = ps;}
  void printFile(){};
  vector<string> choosePeers(int maxDownloads){};
  bool checkComplete(){};
};

void createTorrent (string fileName, string trackerAdr, string peerAdr, zctx_t *context);
string defineParts(istream& baseFile, string fileName);
/*
int download (string torrentName, int maxDownloads, zctx_t *context);
vector<int> createVector(char *ps);
*/

unordered_map<string, File> files;


int main () {
  zctx_t *context = zctx_new();
  files.clear();
  string adr;
  cout << "port to use for listening:";
  cin >> adr;
  /*cout << "Max Downloads: ";
  cin >> maxDownloads;*/
  string peerAdr = "tcp://localhost:";
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
      string torrentName;
      int maxDownloads;
      cout << "Torrent Name: ";
      cin >> torrentName;
      //download(torrentName, maxDownloads, context);
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
  void *tracker = zsocket_new(context, ZMQ_REQ);
  string tmp = fileName, tmptracker = trackerAdr, parts;

  //define number of parts
  ifstream baseFile(tmp += ".txt");
  parts = defineParts(baseFile, fileName);
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
  getline (torrentFileI,tmptracker);
  trackerAdr = "tcp://localhost:";
  trackerAdr.append(tmptracker);
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
      cout <<  tmp << "\n";
    }
  }
  part.close();
  parts = to_string(contParts);
  return parts;
}

/*
int download(string torrentName, int maxDownloads, zctx_t *context){
  string trackerAdr, torrent fileName;
  zmsg_t *msg = zmsg_new();
  void *tracker = zsocket_new(context, ZMQ_REQ);
  //read from file tracker address and file name
  ifstream torrentFile(torrentName+=".torrent");
  getline (torrentFile, trackerAdr);
  getline (torrentFile, fileName);

  //connection to tracker
  int rc = zsocket_connect(tracker, trackerAdr);
  assert (rc == 0);

  string op = "download";
  zmsg_addstr(msg, op.c_str());
  zmsg_addstr(msg, fileName.c_str());
  zmsg_dump(msg);
  zmsg_send(&msg, tracker);
  assert (msg == NULL);
  msg = zmsg_recv(tracker);

  //obtain parts
  char *result = zmsg_popstr(msg);
  if (strcmp(result, "failure") == 0)
    return -1;
  char *np = zmsg_popstr(msg);
  int nPeers = atoi(np);
  for (int i = 0; i < nPeers*2; i++){
    char *p = zmsg_popstr(msg);  //peer address
    char *ps = zmsg_popstr(msg);  //partes
    string peer = p;

    //create vector and unordered map
    vector<int> parts = createVector(ps);
    unordered_map<string, vector<int>> umparts;
    umparts[p]=parts;
    //create file
    File f = File(filename, um);
    files.push_back(f);
  }
  f.choosePeers(maxDownloads);
  //elegir peers de donde se descargara (debe ser menor al numero de descargas posibles)
  //abrir hilos
  //POR HILO
  //elegir parte a descargar
  //pedir parte
  //guardar en carpeta temporal
  //actualizo estado del archivo: vector downloadedParts
  //checkComplete()
}

vector<int> createVector(char *ps){

}*/