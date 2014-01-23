//Tim Skowronski
//Program 09 - Final Exam
/*
Server source code notes:
This program is responsible for handling all incoming connections from the port 5555.
It also creates a socket with infromation from the host, and the incoming client connection.
Phrases/words and their respective hints are stored in a plain text file,
and are loaded the first time a client connects to the server.
I used send() to send information to the client over the socke.
I used ford() to make a child process for every client that connects to the server.
There should be no issues with the code, everything gets closed, and freed once it is done being used.

If Altering the file phrases.txt please make sure to change the randomdly generated value
used to choose which phrase and hint pair to send to the client.

phrases.txt file needs to be in the following format
[phrase]|[hint]\n
DO NOT include the brackets, or \n
they are merely there for clearer example.
*/

//Includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

//Constants
#define PORT "5555"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold

//Typedefs
typedef struct words
{
  char* phrase;
  char* hint;
} words;

//Prototypes
void sigchld_handler(int s);// My signal handler
void *get_in_addr(struct sockaddr *sa); //get sockaddr, IPv4 or IPv6
void read_file(FILE* fin, words* arr[]); // Used to read the phrases.txt fil

int main(int argc, char* argv[])
{
  int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd.
  struct addrinfo hints, *servinfo, *p; //struct for server address info.
  struct sockaddr_storage their_addr; // connector's address information.
  socklen_t sin_size;
  struct sigaction sa;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  FILE* fin;
  words* arr[1024];
  int i;
  int x;
  char temp[1024];

  for(i = 0; i < 1024; ++i)
  {
    arr[i] = (words*)malloc(sizeof(words));
    arr[i]->phrase = (char*)malloc(sizeof(char) * 1024);
    arr[i]->hint = (char*)malloc(sizeof(char) * 1024);
  }
  fin = fopen("phrases.txt", "r");
  if(fin == NULL)
  {
    printf("phrases.txt could not be opened, check dir path\n");
    exit(1);
  }
  read_file(fin, arr); //load file of phrases into struct

  //set up of struct holding server info.
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  //get IP address from server, load serverinfo struct.
  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next)
  {
    //Returns a new socket descriptor that you can use to do sockety things with
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("server: socket");
      continue;
    }
    //Binds socket to port, can allow multiple sockets to bind to port, aslong there is not active data transfer going on.
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
      perror("setsockopt");
      exit(1);
    }
    //Finally bind the socket.
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("server: bind");
      continue;
    }
    break;
  }
  //check to see if it actually binded a socket.
  if (p == NULL)
  {
    fprintf(stderr, "server: failed to bind\n");
    return 2;
  }
  freeaddrinfo(servinfo); // all done with this structure
  if (listen(sockfd, BACKLOG) == -1)
  {
    perror("listen");
    exit(1);
  }
  //Deals with all the signals.
  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1)
  {
    perror("sigaction");
    exit(1);
  }

  //Starts infinite accept loop for connections.
  printf("server: waiting for connections...\n");
  while(1)
  {  // main accept() loop
    sin_size = sizeof(their_addr);
    //get a socket descriptor that is used for communication with the client.
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1)
    {
      perror("accept");
      continue;
    }
    //format the incoming client's IP address from binary to a readable format.
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
    printf("server: got connection from %s\n", s);
    if (!fork())
    { // this is the child process
      close(sockfd); // child doesn't need the listener
      srand(time(NULL));
      /*
        set to how many i have in my file.
        NOT set to dynamically change with the file size.
        Will need to change this with altercations to the,
        phrases.txt file, as noted at the top of this code.
      */
      x = (rand() % 17);
      /*
        Re-adding a break after the end of phrase.
        My reason for this is on the client source code,
        i have it recive the everything with one recv() call.
      */
      strcat(temp, arr[x]->phrase);
      strcat(temp, "|");
      //Send a pair of phrases and hints over the socket, to the client.
      if (send(new_fd, temp, strlen(temp), 0) == -1)
      {
        perror("send");
      }
      if (send(new_fd, arr[x]->hint, strlen(arr[x]->hint), 0) == -1)
      {
        perror("send");
      }
      close(new_fd); // close the socekt.
      exit(0);//kill child process.
    }
    close(new_fd);  // parent doesn't need this
  }
  free(arr); // free my arr of pairs of phrases and hints.
  return 0;
}

//Used to handle singles.
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

//get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//Used to read the phrases.txt file.
//Files needs to be in format PHRASE|HINT
//They also need to be on their own line.
//For the algorithm to sepeate the phrases and hints correcly.
void read_file(FILE* fin, words* arr[])
{
  int j = 0;
  char line[1024];
  char temp[1024];
  char *data[1];
  while(( fgets(line, sizeof(line), fin)) != NULL)
  {
    strcpy(temp, line);
    data[0] = strtok(temp, "|");
    data[1] = strtok(NULL, "|");
    strcpy(arr[j]->phrase, data[0]);
    strcpy(arr[j]->hint, data[1]);
    ++j;
  }
  return;
}
