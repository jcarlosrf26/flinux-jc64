#include <stdio.h>
#include <string.h>

int fnsplit(const char* infile,char* d,char* dir, char* outfile,char* e){
   char* dotpos = NULL;  //position of dot before extension
   char* tmpfile = NULL; //position within infile
   while (*infile) {
     if (*infile++ == '.') {
         dotpos = infile;
     }
     if (*infile == '/') {
         tmpfile = infile+1; //position after slash
     }
   }
   sprintf(outfile,"%s",tmpfile);    //move filename into outfile array
   outfile[dotpos-tmpfile-1]='\0';   //end filename at dot position
   return 0;
}
