// Chaya Chrein
// gcc main.c -o main.exe -lrt
// ./main.exe input.txt out.txt 24 128


//how to get rid of extra at end of read in "and rob"
//how to get rid of sprintf



#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>  

//toAsc takes an integer and converts it into a string by using the ascii code
char* toAsc(int number){
 int n = number;
 int digits=0;

  //get the number of digits
 if (number == 0) digits = 1;
  while(n > 0){
     digits++;
     n /= 10;
   }

    //find the degree of the number - basically 10^digits-1
   int deg = 1;
   for(int i = 1; i < digits; i++){
     deg*=10;
   }

  //need to allocate memory for the new string
   char *str = malloc(digits+1);
   int r;
   int code;

   //convert each digit into it's ascii code
   for(int i = 0; i < digits; i++){
     r = number%deg;
     code = number/deg + 48;
     deg/=10;
     char convert = code;
     str[i] = code;
     number = r;
   }
    //append null character to end of string
   str[digits] = '\0';
   return str;
 }

//integers for buffer which will be shared
int *in;
int *out;
int *counter;

int main(int argc, char*argv[]) {
  const char* SRC_FILE = argv[1];
  const char* TGT_FILE = argv[2];
  const int CHUNK_SIZE = strtol(argv[3],NULL,10);
  const int BUFFER_SIZE = strtol(argv[4],NULL,10);
  int srcfile, tgtfile;
  int pipefd[2];
  
  if (argc != 5){
    printf("Wrong number of command line arguments\n"); 
    return 1;
  }
  if ((srcfile = open(SRC_FILE, O_RDONLY, 0)) == -1){ 
    printf("Can't open %s \n", SRC_FILE); 
    return 2;
  }
  if ((tgtfile = creat(TGT_FILE, 0644)) == -1){ 
    printf("Can't create %s \n", TGT_FILE); 
    return 3;
  }
  if (pipe(pipefd)<0){
    printf("Pipe faild");
    return 1;
    }

  //create the shared memory
  char* name = "sharedMem";
  void* ptr;
  int shmfd;
  shmfd = shm_open(name, O_CREAT | O_RDWR, 0666);
  if (ftruncate(shmfd, BUFFER_SIZE)<0) printf("TRUNCATE FAILED");
  ptr = mmap (0, BUFFER_SIZE*CHUNK_SIZE, PROT_WRITE, MAP_SHARED, shmfd, 0);
  char buf[BUFFER_SIZE];
  int line[BUFFER_SIZE];

  //make variables able to be shared between parent and child- allocating dynamic memory
  in = mmap(NULL, sizeof *in, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  out = mmap(NULL, sizeof *out, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  counter = mmap(NULL, sizeof *counter, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *in=0;
  *out=0;
  *counter=0;

  //create parent and child process
  pid_t pid;
  pid = fork();
  
  if (pid <0) {
    printf("FORK FAILED");
    return 1;
  }
    
  else if (pid == 0){  
    int shMemChldCharCount = 0;
    int ccount = 0;
    int n;
    
    while(1){
      if (*in == *out) {
      }
      else{
      if ((n=write(tgtfile, ptr, CHUNK_SIZE))<= 0) printf("CHILD CANT PRINT\n");
      ccount++;
      write(STDOUT_FILENO, "\nChild out: ", sizeof("Child out: "));
      write(STDOUT_FILENO, toAsc(*out), 2);

      write(STDOUT_FILENO, "\nChild item: ", sizeof("Child item: "));
      write(STDOUT_FILENO, ptr, CHUNK_SIZE);


      ptr+=CHUNK_SIZE;
      shMemChldCharCount+=n;
      *out = (*out + 1) % BUFFER_SIZE; 

        //if read in by child is equal to counter reurned by parent then read from pipe because it finished
      if(ccount!=0 && ccount==*counter){
        close(tgtfile);
        close (pipefd[1]);
        n = read (pipefd[0], line, BUFFER_SIZE);
        write(STDOUT_FILENO, "\nCHILD: The producer value of shMemPrntCharCount  = ", 52);
        write(STDOUT_FILENO, line, 4);
        write(STDOUT_FILENO, "\nCHILD: The consumer value of shMemChldCharCount  = ", 52);
        write(STDOUT_FILENO, toAsc(shMemChldCharCount), sizeof(toAsc(shMemChldCharCount)));
        write(STDOUT_FILENO, "\n", 1);
        break;
        }
     }
    }
    shm_unlink(name);
    }   
 
  else{
    int shMemPrntCharCount=0;
    int n;
    int pcount=0;
    
    while (1){ 
      while ((n = read(srcfile, buf, CHUNK_SIZE)) > 0) {
        //if waiting for child process to catch up
        while (((*in+1) % BUFFER_SIZE) == *out);

      write(STDOUT_FILENO, "\nParent in: ", 12);
      write(STDOUT_FILENO, toAsc(*in), 2);

      write(STDOUT_FILENO, "\nParent item: ", 15);
      write(STDOUT_FILENO, buf, CHUNK_SIZE);

        //write data to pointer
        *in = (*in+1) % BUFFER_SIZE;
        for (int i=0; i<n; i++){
          if (buf[i] != 0){
          *(char*)ptr=buf[i];
          ptr++;
          shMemPrntCharCount++;
          }
        }
        pcount++;

        
        }      
        close(srcfile);
        *(char*)ptr='\0';
        ptr++;
        break;
    }
    //share that it's done and how many it read with child
    *counter=pcount;
    close (pipefd[0]);

    write (pipefd[1],toAsc(shMemPrntCharCount),4);
      write(STDOUT_FILENO, "\nPARENT: The producer value of shMemPrntCharCount  = ", sizeof("\nPARENT: The producer value of shMemPrntCharCount  = "));
      write(STDOUT_FILENO,toAsc(shMemPrntCharCount), sizeof(toAsc(shMemPrntCharCount))); 
    wait(NULL);
  }

  return 0;
}