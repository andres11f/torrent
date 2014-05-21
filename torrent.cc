#include <iostream>
#include <fstream>
#include <string>
#include <czmq.h>

using namespace std;


void createTorrent (string fileName, string tracker, string adr, zctx_t *context);
int defineParts(istream& baseFile);


int main () {
  zctx_t *context = zctx_new();
  string listenAdr;
  cout << "port to use for listening:";
  cin >> listenAdr;
  string adr = "tcp://*:"+=listenAdr;

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
      createTorrent(fileName, tracker, adr, context);
    }

    else if (input == "0"){
      return 0;
    }

  }

  zctx_destroy(&context);
  return 0;
}


void createTorrent (string fileName, string tracker, string adr, zctx_t *context){
  string tmp = fileName, line, tracker = "tcp://localhost:"+=tracker, parts;
  zmsg_t *msg = zmsg_new();
  ifstream baseFile(tmp += ".txt");
  parts = defineParts(baseFile);
  tmp = fileName;
  fstream torrentFile (tmp += ".torrent");
  if (baseFile.is_open()){
    torrentFile << tracker;
    torrentFile << "\n";
    torrentFile << fileName;
    baseFile.close();
  }
  else{
    cout << "Error: No File \n";
    return;
  }

  //connection to tracker
  void *tracker = zsocket_new(context, ZMQ_REQ);
  getline (torrentFile,line);
  int rc = zsocket_connect (tracker, line.c_str());
  assert(rc == 0);

  string op = "newtorrent";
  zmsg_addstr(msg, op.c_str());
  zmsg_addstr(msg, fileName.c_str());
  zmsg_addstr(msg, parts.c_str());

  zmsg_send(&msg, tracker);
  assert (msg == NULL);
  zmsg_recv(tracker);

  char *result = zmsg_popstr(msg);
  cout << result << "\n";

  torrentFile.close();
  free(result);
  zmsg_destroy(&msg);
  zsocket_destroy(context, tracker);
  

}


int defineParts(istream& baseFile){
  int cont=0;
  string parts; line;
  while(getline(baseFile, line))
    cont++;
  cout << "Number of lines: " << cont << "\n";
  parts = to_string(cont);
  return parts;
}

/*
list<string> search(string query, zctx_t *context){
  zmsg_t *msg = zmsg_new();
  list<string> peers;
  peers.clear();
  //connection to tracker
  void *tracker = zsocket_new(context, ZMQ_REQ);
  int rc = zsocket_connect (tracker, "tcp://localhost:12345");
  assert(rc == 0);

  string op = "search";
  zmsg_addstr(msg, op.c_str());
  zmsg_addstr(msg, query.c_str());
  zmsg_send(&msg, tracker);
  assert(msg == NULL);
  msg = zmsg_recv(tracker);

  char *result = zmsg_popstr(msg);
  if (strcmp(result, "success") == 0){
    char *n = zmsg_popstr(msg);
    for (int i=0; i<n; i++){
      char *a = zmsg_popstr(msg);
      string adr = a;
      peers.push_back(adr);
    }
    free(n);
    free(a);
  }

  cout << result << "\n";

  free(result);
  zmsg_destroy(&msg);
  zsocket_destroy(context, tracker);
}



ifstream file ("movie.torrent");
  if (file.is_open()){
    getline (file,line);
    file.close();
  }
  else cout << "Unable to open file";
*/