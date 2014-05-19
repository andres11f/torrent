#include <unordered_map>

using namespace std;

int main () {
  unordered_map<string, list<string>> files;  //name of file, address of peers
  

  zctx_t *context = zctx_new();
  void *responder = zsocket_new(context, ZMQ_REP);
  int pn = zsocket_bind(responder, "tcp://*:12345");
  cout << "Port number " << pn << "\n";

  while(1){
    zmsg_t *msg = zmsg_new();
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
  if (strcmp(op, "search") == 0){
    char *q = zmsg_popstr(msg);
    string query = q;
    if (files.count(query))
      zmsg_addstr(response,"success");
      zmsg_addstr(response, files[query].size());
      list<string>::iterator it;
      for (int it=files[query].begin(); it != files[query].end(); ++it)
        zmsg_addstr(response, it->c_str());
    else
      zmsg_addstr(response,"failure");
  }
}