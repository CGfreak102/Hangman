// Tim Skowronski
//Program 09 - Final Exam
/*
Client source code notes:
This program is responsible for connecting to a known server defined by constant SERVER.
It then creats a socket between this computer and the server allowing flow of information.
It recives a single string that contains a phrase and a respective hint.
It is passed in the format [phrase]|[hint], with NO brackets.
At that point, it starts a game of hangman with the phrase and hint passed from the server.
It endlessly promts the user for a single character until all the letters in the phrase have been uncovered.
*/

//Include's
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//Constants
#define PORT "5555" // the port client will be connecting to
#define MAXDATASIZE 1024 // max number of bytes we can get at once
#define SERVER "ale.cae.uwm.edu" //Hard coded server it connects to

//Prototpyes
void *get_in_addr(struct sockaddr *sa); //get sockaddr, IPv4 or IPv6.
void game(char phrase[], char hint[]);  //The hang man game is completely ran in this function.

int main(int argc, char* argv[])
{
  char phrase[MAXDATASIZE];
  char hint[MAXDATASIZE];
  int sockfd, numbytes;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];
  char temp[1024];
  char *data[1];

  //The following three lines load strucs
  //with information regarding the client address
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  //Gets IP address from the server and loads it into the structs
  if ((rv = getaddrinfo(SERVER, PORT, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next)
  {
    //Returns a new socket descriptor that you can use to do sockety things with
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("client: socket");
      continue;
    }
    //Attempted to connect to the socket we just created.
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("client: connect");
      continue;
    }
    //if no errors occur and the socket is created, we exit the loop!.
    break;
  }
  //another check to see if a connection was made.
  if (p == NULL)
  {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

  //format the servers IP addres from binary to a readable format.
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  printf("client: connecting to %s\n", s);
  freeaddrinfo(servinfo); // all done with this structure

  //recive information being sent from the server and load it into phrase.
  if ((numbytes = recv(sockfd, phrase, MAXDATASIZE-1, 0)) == -1)
  {
    perror("recv");
    exit(1);
  }
  /*
    The next few lines of code deal with breaking the string sent from the server,
    into the respective variables the phrase and hint need to be loaded into.
    the format the data is sent like is the following: [phrase]|[hint], not including the brackets.
  */
  strncpy(temp, phrase, strlen(phrase));
  data[0] = strtok(temp, "|");
  data[1] = strtok(NULL, "|");
  strcpy(phrase, data[0]);
  strcpy(hint, data[1]);
  game(phrase, hint); //play the game with server passed phrase and hint.
  close(sockfd); // close the socket to the server.
  return 0;
  ;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//The hangman game algorithm.
void game(char phrase[], char hint[])
{
  int x;
  int finished = 0;
  int totGuessed = 0;
  char input[strlen(phrase)]; // user inputted character
  char output[strlen(phrase)]; //outputed string to screen of guessed and non guessed letters of the phrase
  //load output with the correct format, ? for letters, and a space for any spaces.
  for(x = 0; x <= strlen(phrase); ++x)
  {
    if(phrase[x] == ' ' )
    {
      output[x] = ' ';
      ++totGuessed;  //Add one to guess's for space.
    }
    else
    {
      output[x] = '?';
    }
  }
  printf("HINT: %s", hint);
  //enter loop for constant user input until they have guessed the phrase.
  while(finished == 0)
  {
    printf("Common Phrase\n");
    printf("-------------\n");
    printf("%s\n", output);
    do
    {
      printf("Enter in a single character guess: ");
      scanf("%s", input);
    }while(strlen(input) != 1); //will repeatly ask the user for an input of 1 character, not lower or higher.

    /*
      loops threw the phrase and if the guess letter is found,
      it adds 1 to the totGuessed counter and then changes the ?
      to the respect character in the phrase.
    */
    for(x = 0; x <= strlen(phrase); ++x)
    {
      if(phrase[x] == input[0])
      {
	output[x] = input[0];
        ++totGuessed;
      }
    }

    //If the totGuessed is equal to the length of the phrase, all letter have been discovered and the game ends.
    if(totGuessed == strlen(phrase))
    {
      finished = 1;
    }
  }
  //One last print out to show the full phrase, and a congratulation message.
  printf("Common Phrase\n");
  printf("-------------\n");
  printf("%s\n", output);
  printf("Congratulations, you have guessed the word!\n");
}
