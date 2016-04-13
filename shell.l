

%{
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "y.tab.h"

extern "C" char * read_line();
int mygetc(FILE * f) {
static char *p;
char ch;
if (!isatty(0)) {
  // stdin is not a tty. Call real getc
  return getc(f);
}
// stdin is a tty. Call our read_line.
if (p==NULL || *p == 0) {
  char * s = read_line();
  p = s;
}
ch = *p;
p++;
return ch;
}
#undef getc
#define getc(f) mygetc(f)


static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

%}

%%

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

">" 	{
		return GREAT;
	}

"<"	{
		return LESS;
	}

">>"	{
		return GREATGREAT;
	}

">&"	{
		return GREATAMPERSAND;
	}

">>&"	{
		return GREATGREATAMPERSAND;
	}

"&"	{
		return AMPERSAND;
	}

"|"	{
		return PIPE;
	}

`[^\n`]*` {
        char *buffer = (char*) malloc(1024*sizeof(char));

        char *temp = strdup(yytext);
        char *commands = (char*) malloc(sizeof(char) * strlen(temp));
        
        int j = 0;
        for(int i = 0; i < strlen(temp); i++) {
        	if(temp[i] == '`')
        		continue;
        	else {
	        	commands[j] = temp[i];
	        	j++;
        	}
        }
        commands[j] = '\0';

        int i = 0;
        int len = 0;

        int tmpin = dup(0);
        int tmpout = dup(1);
       
        int pipe1[2];
        int pipe2[2];
       	
       	pipe(pipe1);
       	pipe(pipe2);
       
       	write(pipe2[1],commands,strlen(commands));  
	    write(pipe2[1],"\nexit\n",6);                   
	    close(pipe2[1]);

        dup2(pipe2[0],0);
        close(pipe2[0]);
       
        dup2(pipe1[1],1);
	    close(pipe1[1]);
	    
        int ret = fork();
       
        if(ret == 0) {
			execvp("/proc/self/exe",NULL);
			perror("execvp");
            _exit(1);
        } else if(ret < 0) {
            perror("fork");
            _exit(1);
        }
       	
       	dup2(tmpout, 1);
        dup2(tmpin, 0);   

       	int count = 0;
       	int k = 0;
       	char c = 0;

       	while (read(pipe1[0], &c, 1) != 0)
        {
        	c == '\n' ? buffer[k] = ' ' : buffer[k] = c;
            k++;
        }

        buffer[k] = 0;

        int length = strlen(buffer);
        int c1 = length;
        while(c1 >= 0) {
        	if(buffer[c1] == '\n'){      
				buffer[c1] = ' ';
			}
			unput(buffer[c1]);
			c1--;
        }
    }

[^ \t\n<>&\|]*(\\.[^ \t\n<>&\|]*)*  {
    
    int i = 0;
        int j = 0;
    yylval.string_val = strdup(yytext);
                
        int len = strlen(yylval.string_val);
        char *command = (char*) malloc(sizeof(char)*len);
        
        for(i = 0 ; i < len; i++)
      {           
        if(yylval.string_val[i] != '\\')
          command[j++] = yylval.string_val[i];
        else
        {
          command[j++] = yylval.string_val[i+1];
      i++;
        }
        
      }
    command[j] = '\0';
    strcpy(yylval.string_val, command);

    return WORD;
  }


\"[^\n\"]*\" {
		yytext = (char *) (yytext+1);
		yytext[strlen(yytext)-1] = '\0';

		yylval.string_val = strdup(yytext);
		return WORD;
	}

[^ \t\n|<>&][^ \t\n|<>&]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}

