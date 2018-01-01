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
char* device = "/dev/ttyUSB0"; 

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
  bool comment = false; 
  while(*cmd){
    if(comment) *cmd = 0;
    if(*cmd == ';'){
      comment = true;
      *cmd = '\n'; 
    }
    cmd++; 
  }
}

int main(int argc, char** argv){
  struct termios tty; 
  int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC); 
  if(fd < 0) err("failed to open device"); 
  if(tcgetattr(fd, &tty) != 0) err("tcgetaddr error\n"); 
  tty.c_iflag |= ICRNL; // Carriage returns replaced with newlines 
  tty.c_cc[VMIN] = 1; // Don't return until there's a character available
  tty.c_cc[VTIME] = 10; // Timeout after 1 second  
  if(tcsetattr(fd, TCSANOW, &tty) != 0) err("tcsetaddr error\n"); 
  cfsetspeed(&tty, baudrate); 

  char cmdline[bufsize], respline[bufsize];  
  size_t read;  
  
  while((read = wgetline(cmdline, STDIN_FILENO, true)) != -1){
    char* comment = strchr(cmdline, ';'); 
    if(comment == NULL || comment - cmdline > 1){
      stripcomment(cmdline); 
      if(dprintf(fd, cmdline) < 0) err("dprintf"); 
      printf("sent: %s", cmdline); 
      while(1){
        read = wgetline(respline, fd, false); 
        if(read != -1 && read > 0){
          printf("recv %u: %s", read, respline); 
          if(strncmp("ok", respline, 2) == 0) break; 
        }
      }
    }
    else printf("skipped: %s", cmdline); 
  }
}
