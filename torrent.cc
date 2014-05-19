#include <iostream>
#include <fstream>
#include <string>

using namespace std;


int main () {
  zctx_t *context = zctx_new();
  //inform tracker that peer is connected

  while(1){
    string input = "";
    cout << "1. Search";
    cout << "2. Download";
    cout << "3. Use torrent";
    cout << "0. Exit";
    cout << "Selection: ";
    cout.flush();
    cin >> input;

    if (input == "1"){
      string query;
      cout << "File: ";
      cin << query;
      list<string> peers = search (query, context);
    }

    else if (input == "2"){
    }

    else if (input == "3"){
      cout << "File";
      cin << file;
    }

    else if (input == "0"){
      return 0;
    }

  }

  zctx_destroy(&context);
  return 0;
}


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


/*
ifstream file ("movie.torrent");
  if (file.is_open()){
    getline (file,line);
    file.close();
  }
  else cout << "Unable to open file";
*/