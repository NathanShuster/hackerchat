#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

#define PORT 8080
int listener;
int queue_size = 10;

void bind_to_port(int socket){
  struct sockaddr_in name; /* create socket descriptor*/
  name.sin_family = AF_INET;
  name.sin_port = (in_port_t)htons(PORT);
  name.sin_addr.s_addr=htonl(INADDR_ANY); /*Bind socket to all available*/
                                          /*interfaces, constant = 0*/
  int reuse = 1;
  // Forcefully attaching socket to make it sticky so can be reused, parameter(socket, level, option)
  if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1){
    perror("Can't set the reuse option on the socket");
  }
  int c = bind(socket, (struct sockaddr *) &name, sizeof(name)); // grab the port
  // sockaddr is a struct that has socket family name and address
  if (c == -1){
    perror("Can't bind to socket");
    exit(EXIT_FAILURE);
  }
}

int open_listener_socket(){
  int listener_d = socket(PF_INET, SOCK_STREAM, 0); //Sock stream is connection orientated
  if(listener_d == -1){
    perror("Can't open socket");
    exit(EXIT_FAILURE);
  }
  return listener_d;
}

/*
Send a string to a client
*/
int say(int socket, char *s){
  int result = send(socket, s, strlen(s),0);
  if (result == -1){
    fprintf(stderr, "%s: %s\n", "Error talking to the client", strerror(errno));
    /* DOn't call error since we don't want to stop the server if there's a problem with one client*/
  }
  return result; // check if successfully send a string to the client
}

void handle_shutdown(int sig){
  if(listener){
    close(listener);
  }
  fprintf(stderr, "Bye!\n");
  exit(0);
}

/*Reads input into buffer of size len or until reaches '/n'
  calling recv doesn't guarantee a full read, so it is called more than once*/
int read_in(int socket, char *buf, int len)
{
  char *s = buf;
  int slen = len;
  int c = recv(socket, s, slen, 0); /*descriptor, buffer, bytes to read, 0*/
  while ((c > 0) && (s[c-1] != '\n')) {
    s += c;
    slen -= c;
    c = recv(socket, s, slen, 0);
    /*returning length (of characters) of input*/
  }
  if (c < 0)
    return c;
  else if (c == 0)
    buf[0] = '\0';
  else
    s[c-1]='\0';
  return len - slen;

}


int main(int argc, char *argv[]){
  listener = open_listener_socket(); // create internet streaming socket
  bind_to_port(listener); // bind the socket on the port
  if (listen(listener,queue_size) == -1){ // set queue size for listening
    perror("Can't listen!");
    exit(EXIT_FAILURE);
  }
  struct sockaddr_storage client_addr;
  unsigned int address_size = sizeof(client_addr);

  puts("Waiting for connection...\n");
  char buf[1024];
  int connect_d;
  while(1){
    connect_d = accept(listener, (struct sockaddr *)&client_addr, &address_size);
    if(connect_d == -1){
      perror("Can't open secondary socket");
    }
    if (say(connect_d, "Type your message:\r\nHack3rCh@t v1.0\r\n") != -1) {
      read_in(connect_d, buf, sizeof(buf));
      printf("Message: %s\n", buf);
      say(connect_d, "Message Received.\r\n");
    }
  }
  close(connect_d);



  return 0;
}
