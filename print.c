#include <ctype.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

size_t bufsize=256; 
speed_t baudrate = B115200; 
#define device "/dev/ttyUSB0"

int err(char* msg){
  perror(msg);
  _exit(errno); 
}

int wgetline(char* line, int fd, bool eof){
  size_t i=0; 
  for(int i=0; i<bufsize; i++) line[i]=0; 
  while(i < bufsize){
    int n = read(fd, line+i, 1);  
    if(eof && n == 0) return -1; 
    if(line[i] == '\n') return i; 
    i += n; 
  }
  return -1;
}

void stripcomment(char* cmd){
  if(*cmd == 0) return; 
  if(*cmd == ';') strcpy(cmd, "\n\0"); 
  else stripcomment(cmd+1);
}

bool iswhitespace(char* cmd){
  return *cmd == 0 || (isspace(*cmd) && iswhitespace(cmd+1)); 
}


int main(int argc, char** argv){
  // Initialization
  struct termios tty; 
  int ttyfd = open(device, O_RDWR | O_NOCTTY | O_SYNC); 
  if(ttyfd < 0) err("failed to open " device); 
  if(tcgetattr(ttyfd, &tty) != 0) err("tcgetaddr error\n"); 
  tty.c_iflag |= ICRNL; // Carriage returns replaced with newlines 
  tty.c_cc[VMIN] = 1; // Don't return until there's a character available
  tty.c_cc[VTIME] = 1; // Timeout after 0.1 seconds
  if(tcsetattr(ttyfd, TCSANOW, &tty) != 0) err("tcsetaddr error\n"); 
  if(cfsetspeed(&tty, baudrate) != 0) err("cfsetspeed error\n"); 

  // Main loop
  char cmdline[bufsize], respline[bufsize];  
  int nbytes;  
  while((nbytes = wgetline(cmdline, STDIN_FILENO, true)) != -1){
    stripcomment(cmdline); 
    if(!iswhitespace(cmdline)){
      if(dprintf(ttyfd, cmdline) < 0) err("dprintf"); 
      printf("sent: %s", cmdline); 
      while(1){
        nbytes = wgetline(respline, ttyfd, false); 
        if(nbytes > 0){
          printf("recv: %s", respline); 
          if(strncmp("ok", respline, 2) == 0) break; 
        }
      }
    }
  }
}
