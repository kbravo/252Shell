

%token	<string_val> WORD

%token 	NOTOKEN PIPE GREAT LESS GREATGREAT GREATAMPERSAND GREATGREATAMPERSAND AMPERSAND NEWLINE

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#define MAXFILENAME 1024
#include <stdio.h>
#include "command.h"
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <regex.h>
#include <dirent.h>
#include <assert.h>


char **arguments = NULL;
int nEntries = 0;
int maxEntries = 10;
void yyerror(const char * s);
int yylex();
void expandWildcardsIfNecessary(char * arg);
void expandWildcard(char * prefix, char *suffix);
int compare(const void *str1, const void *str2);
void sortDirectories();

%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: simple_command
        ;

simple_command:	
	command_and_args pipe_list iomodifier_list background_optional NEWLINE {
		Command::_currentCommand.execute();
	}
	| NEWLINE {
		Command::_currentCommand.prompt();
	} 
	| error NEWLINE { yyerrok; }
	;

pipe_list:
	PIPE command_and_args pipe_list
	|
	;

iomodifier_list:
	iomodifier_opt iomodifier_list
	|
	;

command_and_args:
	command_word argument_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

argument_list:
	argument_list argument
	| /* can be empty */
	;

argument:
	WORD {
	       //Command::_currentSimpleCommand->insertArgument( $1 );
         expandWildcardsIfNecessary(strdup($1));
	}
	;

command_word:
	WORD {      
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_opt:
	GREAT WORD {
		if(Command::_currentCommand._outFile != NULL) {
			yyerror("Ambiguous output redirect.\n");
		}
		Command::_currentCommand._outFile = $2;
	}
	| LESS WORD {
		if(Command::_currentCommand._inFile != NULL) {
			yyerror("Ambiguous input redirect.\n");
		}
		Command::_currentCommand._inFile = $2;
	} 
	| GREATGREAT WORD {
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._appendOut = 1;
	}
	| GREATAMPERSAND WORD {
		Command::_currentCommand._errFile = $2;
	}
	| GREATGREATAMPERSAND WORD {
		Command::_currentCommand._errFile = $2;
		Command::_currentCommand._appendError = 1;
	}
	|/* can be empty */ 
	;

background_optional:
	AMPERSAND {
		Command::_currentCommand._background = 1;
	} |
	;

%%

void sortDirectories()
{
    int i = 0;
    int j = 0;
    for(i = 0; i < nEntries-1; i++)
    {
        for(j = 0; j< nEntries-1; j++)
        {
            char* one = arguments[j];
            char* two = arguments[j+1];
            if(strcmp(one, two) > 0)
            {
                char* three = arguments[j];
                arguments[j] = two;
                arguments[j+1] = three;
            }
        }
    }

    return;
}

void expandWildcardsIfNecessary(char * arg) 
{
  //No wildcards, not necessary to expand
	if (strchr(arg, '*') == NULL && strchr(arg, '?') == NULL) {
		Command::_currentSimpleCommand->insertArgument(arg);
    return;
	}

  //temporary copy of argument
	char *temp = strdup(arg);
  char *pre = strdup("");
  expandWildcard(pre, temp);

  sortDirectories();

	// Add arguments
	for (int i = 0; i < nEntries; i++) {
	 Command::_currentSimpleCommand->insertArgument(arguments[i]);
	}
  
	if(arguments != NULL)
		free(arguments);
	nEntries = 0;
	arguments = NULL;
	return;
}

void expandWildcard(char * prefix, char *suffix) 
{
	//sanity checks
	if(arguments == NULL)
    	arguments = (char**)malloc(maxEntries*sizeof(char*));

	//Recursion base case
	if (suffix[0] == 0) {
		// suffix is empty. Put prefix in argument.
		//Command::_currentSimpleCommand->insertArgument(strdup(prefix));

    // Resize array if reached maxEntries
    if (nEntries == maxEntries) {
      maxEntries *=2; 
      arguments = (char**) realloc(arguments, maxEntries*sizeof(char*));
      assert(arguments!=NULL);
    }
		return;
	}

  //temporary suffix and prefix copies
	char *tempSuffix = strdup(suffix);
	char *tempPrefix = strdup(prefix);

  bool isDirectory = false; // check for '/'

	// Obtain the next component in the suffix
	// Also advance suffix.

	
	char component[MAXFILENAME];
 	
	  //building expression for opendir - flag set
	  if(suffix[0] == '/') {
		isDirectory = true;
	  }
	char * s = strchr(suffix, '/');
	if (s != NULL) { 
		//checking whether 
		if(prefix[0] == 0 && isDirectory == true) {
			suffix = s+1;
			s = strchr(suffix, '/');

			if(s == NULL) {  // if no other '/' exist
    			strcpy(component, suffix);
    			suffix = suffix + strlen(suffix);
    			prefix = strdup("/");
  		} else { // if there is another '/'
	      strncpy(component,suffix, s-suffix);
  			suffix = s + 1;
  			prefix = strdup("/");
  		}
  	} else { // no '/' and empty prefix (tilde recur)
			strncpy(component,suffix, s-suffix);
			suffix = s + 1;
		}
	} else { //if no wildcards found ( only once '/')
    	strcpy(component, suffix);
		suffix = suffix + strlen(suffix);
  }

	// Now we need to expand the component
	char newPrefix[MAXFILENAME];

  //init to avoid segv

	if (strchr(component, '*') == NULL && strchr(component, '?') == NULL) {
  	  // component does not have wildcards

  		if(prefix[0] == 0 && !isDirectory) {
    		sprintf(newPrefix,"%s",component);
      } else if(strcmp(prefix, "/") == 0) {
    		sprintf(newPrefix,"/%s",component);
      } else {
    		sprintf(newPrefix,"%s/%s",prefix,component);    
      }

      if(component[0] != 0)
        expandWildcard(newPrefix, suffix);
      else
        expandWildcard(strdup(""), suffix);
  		return;
	}

  // Component has wildcards
  // Convert component to regular expression

   // 1. Convert wildcard to regular expression
  // Convert “*” -> “.*”
  //         “?” -> “.”
  //         “.” -> “\.”  and others you need
  // Also add ^ at the beginning and $ at the end to match
  // the beginning ant the end of the word.
  // Allocate enough space for regular expression
  	
	char * reg = (char*)malloc(2*strlen(component)+10); 
	char * a = component;
	char * r = reg;
	*r = '^'; r++; // match beginning of line
	while (*a) {
		if (*a == '*') { *r='.'; r++; *r='*'; r++; }
		else if (*a == '?') { *r='.'; r++;}
		else if (*a == '.') { *r='\\'; r++; *r='.'; r++;}
		else { *r=*a; r++;}
		a++;
	}
	*r='$'; r++; *r=0;// match end of line and add null char
  // 2. compile regular expression
  
	regex_t re;
	regmatch_t match; 
	int result = regcomp( &re, reg, REG_EXTENDED);
	//free(reg);
	if (result != 0) {
  		perror("Bad regular expression: compile");
  		return;
	}
  
  // 3. List directory and add as arguments the entries 
  // that match the regular expression
  	
	char *directories;
	if (prefix[0] == 0) {
  		directories = strdup(".");
  } else directories = strdup(prefix);

	DIR * dir = opendir(directories);
	if (dir == NULL) {
    //perror("opendir error");
		return;
	}

	struct dirent * ent;

	while ( (ent = readdir(dir))!= NULL) {
  	// Check if name matches
  	if (regexec(&re, ent->d_name, 1, &match, 0 ) == 0) {

		//finding matching direcotry entries
  		if (ent->d_name[0] == '.') {
      	if (component[0] == '.') {//If it is current wild card
      		if(prefix[0] == 0) {
        			sprintf(newPrefix,"%s",ent->d_name);
          } else if(prefix[0] == '/' && prefix[1] == 0) {
        			sprintf(newPrefix,"/%s",ent->d_name);
          } else {
        			sprintf(newPrefix,"%s/%s",prefix,ent->d_name);
          }

      		expandWildcard(newPrefix,suffix); // recu
          if(s == NULL) { //sanity check
            arguments[nEntries] = strdup(newPrefix);
            nEntries++;
          }
        }
      } else {
        if(prefix[0] == 0) { // empty prefix case
          sprintf(newPrefix,"%s",ent->d_name);
        } else if(prefix[0] == '/' && prefix[1] == 0) { 
          sprintf(newPrefix,"/%s",ent->d_name);
        } else {
          sprintf(newPrefix,"%s/%s",prefix,ent->d_name);
        }
        
        expandWildcard(newPrefix,suffix); // recur
        if(s == NULL) { // sanity check
          arguments[nEntries] = strdup(newPrefix);
          nEntries++;
        }
      }
    }
  }
  closedir(dir);
  return; 
}

int compare(const void *str1, const void *str2)
{
  const char *one = strdup((char*)str1);
  const char *two = strdup((char*)str2);
  return (strcmp(one, two));
}

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif

