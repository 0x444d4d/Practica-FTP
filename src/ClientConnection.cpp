//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//              This class processes an FTP transactions.
// 
//****************************************************************************



#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

//#include <stringstream>

#include "common.h"

#include "ClientConnection.h"
#include "FTPServer.h"

#include <fstream>


const unsigned short PATH_BUFF = 4098;


ClientConnection::ClientConnection(int s) {
    int sock = (int)(s);
  
    char buffer[MAX_BUFF];

    control_socket = s;
    // Check the Linux man pages to know what fdopen does.
    fd = fdopen(s, "a+");

    if (fd == NULL) {
        std::cout << "Connection closed" << std::endl;

        fclose(fd);
        close(control_socket);
        ok = false;
        return ;
    }
    
    ok = true;
    data_socket = -1;
    parar = false;
  
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
  
}


int ClientConnection::connect_TCP( uint32_t address,  uint16_t  port) {

     // Implement your code to define a socket here

   //char addr[25] = {address};
   printf( "Connecting to %d, %d\n", address, port );

   data_socket = socket(AF_INET, SOCK_STREAM, 0);
   if (data_socket < 0) {
       printf("Error al asignar socket a data_socket\n");
   } else {
       printf("Socket asignado correctamente\n");
   }



   struct sockaddr_in sin;
   memset( &sin, 0, sizeof(sin));
   sin.sin_family = AF_INET;
   sin.sin_addr.s_addr = address;
   sin.sin_port = htons(port);

   if ( connect(data_socket, (struct sockaddr *) &sin, sizeof(sin)) == 0) {
       printf("Connection stablished\n");
       return 0;
   } else {
       printf("Connection not stablished\n");
       return -1;
   }


}






void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;
  
}





    
#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
    //bool parar = false;
    if (!ok) {
	 return;
    } 
    
    fprintf(fd, "220 Service ready\n");
  
    while(!parar) {
 
      fscanf(fd, "%s", command);
      if (COMMAND("USER")) {
	    fscanf(fd, "%s", arg);
	    fprintf(fd, "331 User name ok, need password\n");
      }
      else if (COMMAND("PWD")) {
          char cwdirectory[PATH_BUFF]; //Tamaño maximo de path en linux.
          if (getcwd(cwdirectory, sizeof(cwdirectory) ) != nullptr) {
              fprintf(fd, "257 \"%s\"\n",cwdirectory); //REVISAR
          } else {
              fprintf(fd, "501 Directory does not exist\n");
          }
      }
      else if (COMMAND("PASS")) {
          fscanf(fd, "%s", arg);
          if ( strcmp(arg, "1234") == 0 ) {
              fprintf(fd, "230 Correct password\n");
          } else {
              fprintf(fd, "500 Error\n");
          }
          fflush(fd);
	   
      }
      else if (COMMAND("PORT")) {
          uint32_t a1, a2, a3, a4, p1, p2;
          int port = 0, address;
          fscanf(fd, "%u,%u,%u,%u,%u,%u", &a4, &a3, &a2 ,&a1, &p1, &p2);

          port = p1 << 8;
          port = port | p2;
          
          address = a1 << 24 | a2 << 16 | a3 << 8 | a4;
          int error =  connect_TCP(address, port);
          if (error < 0) {
              fprintf(fd, "421 Cannot stablish connection (%d,%d,%d,%d,%d,%d)\n",a4, a3, a2 , a1, p1, p2);
              fflush(fd);
              ok = false;
          } else {
              fprintf(fd, "200 Ok.\n");
              fflush(fd);
              ok = true;
          }
      }
      else if (COMMAND("PASV")) {
          printf("Received PASV\n");

          int s = define_socket_TCP(0);

          struct sockaddr_in sin;
          socklen_t slen = sizeof(sin);
          getsockname(s, (sockaddr*)&sin, &slen);
          int port = sin.sin_port;
          
          int phigh = port & 0xFF;
          int plow = port >> 8;

          fprintf(fd, "227 Entering Passive Mode 127,0,0,1,%d,%d\n", phigh, plow);
          fflush(fd);

          data_socket = accept(s, (sockaddr*)&sin, &slen);
      }
      else if (COMMAND("CWD")) {
          fscanf(fd, "%s", arg);
          if (chdir(arg) == 0) {
              //Error detectado, errno contiene el codigo
              fprintf(fd, "250 Working directory changed\n");
              fflush(fd);
          } else {
              fprintf(fd, "550 Changing directory failed\n");
              fflush(fd);
          }
	   
      }
      else if (COMMAND("STOR") ) {
          std::ofstream file;
          char buff[4096];
          int size;

          printf("Received STOR\n");

          fscanf(fd, "%s", arg);
          printf("Requested file: %s\n", arg);
          file.open(arg);

          if (!file.is_open()) {
              fprintf(fd, "450 File unavailable\n");
              fflush(fd);
              close(data_socket);
          } else {
              fprintf(fd, "150 File creation ok, about to open data connection\n");
              fflush(fd);
              
              struct stat buf;

              do {
                  size = recv(data_socket, buff, 4096, 0);
                  printf("Recividos %d bytes\n", size);
                  file << buff;

                  fstat(data_socket, &buf);

              } while (buf.st_size != 0);

              fprintf(fd, "226 Stored file correctly\n");
              file.close();
              close(data_socket);

          }
      }
      else if (COMMAND("SYST")) {
          fprintf(fd, "215 UNIX Type: L8\n");
	   
      }
      else if (COMMAND("TYPE")) {
          fscanf(fd, "%s", arg);

          if ( strcmp(arg, "A") || strcmp(arg, "B") || strcmp(arg, "I")) {
              fprintf(fd, "200 Binary mode.\n");
              fflush(fd);
          } else {
              fprintf(fd, "501 Syntax error\n");
              fflush(fd);
          }

      }
      else if (COMMAND("RETR")) {

          FILE* ofile;
          std::ifstream file;
          std::string fstring;
          char buff[4096];
          fscanf(fd, "%s", arg);
          file.open(arg);
          int size = 4096;

          ofile = fopen(arg, "r");

          if ( ofile != NULL) {

              fprintf(fd, "150 requested file opened\n");
              while (size == 4096) {
                  size = fread(buff, 1, 4096, ofile);
                  send(data_socket, buff, size, 0);
              }

              fclose(ofile);
              fprintf(fd, "226 file sent correctly\n");
              fflush(fd);

          } else {
              fprintf(fd, "501 file doesn't exist\n");
              fflush(fd);
          }
          close(data_socket);
      }
          else if (COMMAND("QUIT")) {
          fprintf(fd, "221 Closing server\n");
          fflush(fd);
          parar = true;
          //stop();
	 
      }
      else if (COMMAND("LIST")) {

          printf("Received LIST\n");

          struct stat buf;
          int tmpfd = fileno(fd);
          fstat(tmpfd, &buf);

          char path[4096];
          getcwd(path, sizeof(path));
          printf("Directorio: %s\n", path);

          if (buf.st_size != 0) {

              fscanf(fd, "%s", arg);
              printf("arg = %s\n", arg);

              strcat(path, "/") ;
              strcat(path, arg);

              printf("Directorio completo: %s\n", path);
          }

          if (path != NULL) {

              DIR* dir = opendir(path);
              struct dirent *dp;
              fprintf(fd, "125 list started ok\n");
              fflush(fd);

              while ( (dp = readdir(dir)) != NULL ) {

                  std::string name = dp->d_name;
                  name += "\n";
                  printf("LIST: %s", name.c_str());
                  send(data_socket, name.c_str(), name.size(), 0);
              }

              fprintf(fd, "250 completed list.\n");
              fflush(fd);
              close(data_socket);
          } else {

              printf("Error en path\n");
              fprintf(fd, "500 error\n");
              fflush(fd);
          }
      }
      else  {
	    fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	    printf("Comando : %s %s\n", command, arg);
	    printf("Error interno del servidor\n");
	
      }
      
    }
    
    fclose(fd);

    
    return;
  
};
