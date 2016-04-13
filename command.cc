#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "command.h"
#include <unistd.h>
#include <pwd.h>

extern char **environ;
int *backgroundPID;
bool running = false;

SimpleCommand::SimpleCommand()
{
	// Create available space for 5 arguments
	_numOfAvailableArguments = 5;
	_numOfArguments = 0;
	_arguments = (char **) malloc( _numOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numOfAvailableArguments == _numOfArguments  + 1 ) {
		// Double the available space
		_numOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numOfAvailableArguments * sizeof( char * ) );
	}

    int i = 0;
    int j = 0;
    char* temp = (char*)malloc(sizeof(char)*1024);
    char* var;
    char* temp2;
    
    const char * pattern = "\\${.*}";
    regex_t preg;
    regmatch_t match;
    if ( regcomp(&preg, pattern, 0) ) {
        perror("regex failed to compile");
        exit(1);
    }

    if (strchr(argument, '$'))
    {
        while(argument[i] != '\0')
        {
            var = (char*)malloc(strlen(argument));
            temp2 = (char*)malloc(strlen(argument));
            if (argument[i] == '$')
            { 
                i += 2;
                while(argument[i] != '}')
                {
                    var[j] = argument[i];
                    j++;
                    i++;
                }
                var[j] = '\0';
                strcat(temp, getenv(var));
                j = 0;
            }
            else
            {
                while(argument[i] != '\0' && argument[i] != '$')
                {
                    temp2[j] = argument[i];
                    j++;
                    i++;
                }
                j = 0;
                strcat(temp, temp2);
                i--;
            }
            i++;
            free(temp2);
            free(var);
        }
        sprintf(argument, "%s", temp);
    }

	if(argument[0] == '~') {
		if(strlen(argument) == 1){
			argument = strdup(getenv("HOME"));
		} else {
			if(strchr(argument, '/') != NULL) {
				char *t1 = strdup(argument);
				char *t = strchr(t1, '/');
				*t = '\0';
				sprintf(argument, "%s/%s", strdup(getpwnam(t1+1)->pw_dir), t+1);
			} else {
				char *temp = strchr(argument, '~');
				sprintf(argument, "%s", strdup(getpwnam(argument+1)->pw_dir));
			}
		}
	}
	
	_arguments[ _numOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numOfArguments + 1] = NULL;
	
	_numOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_appendOut = 0;
	_appendError = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

void
Command:: clear()
{
	running = false;
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inFile ) {
		free( _inFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_appendOut = 0;
	_appendError = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i+1 );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
		printf("\n");
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inFile?_inFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}
	
	if(strcmp(_simpleCommands[0]->_arguments[0], "exit") == 0) {
		printf("Good bye!!\n");
		exit(100);
	}

	if (strcmp(_simpleCommands[0]->_arguments[0], "setenv") == 0) {
		int result = setenv(_simpleCommands[0]->_arguments[1], _simpleCommands[0]->_arguments[2], 1);
		if(result != 0)
			perror("setenv");
		clear();
		prompt();
		return;
	}

	if (strcmp(_simpleCommands[0]->_arguments[0], "unsetenv") == 0) {
		int result = unsetenv(_simpleCommands[0]->_arguments[1]);
		if(result != 0)
			perror("setenv");
		clear();
		prompt();
		return;
	}
	if(strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0) {
		int ret;
		char **ptr = environ;
		if(_simpleCommands[0]->_numOfArguments == 1)
			ret = chdir(getenv("HOME"));
		else
			ret = chdir(_simpleCommands[0]->_arguments[1]);

		if(ret != 0)
			perror("chdir");
		clear();
		prompt();
		return;
	}

	int tmpin = dup(0);
	int tmpout = dup(1);
	int tmperr = dup(2);
	
	int fdin;
	int fderr;
	
	if(_errFile != NULL) {
		if(_appendError != 0) {
			fderr = open(_errFile, O_WRONLY | O_APPEND | O_CREAT, 0600);
			if(fderr < 0) {
				perror("Error file opening error !");
				exit(1);
			}
		} else {
			fderr = open(_errFile, O_CREAT | O_WRONLY | O_TRUNC, 0600);
			if(fderr < 0) {
				perror("Error file opening error !");
				exit(1);
			}
		}
	} else {
		fderr = dup(tmperr);	
	}

	dup2(fderr, 2);
	close(fderr);

	if(_inFile != NULL) {
		fdin = open(_inFile, O_RDONLY);
		if(fdin < 0) {
			perror("Cannot open the input file");
			exit(1);
		}
	} else {
		fdin = dup(tmpin);
	}

	int ret;
	int fdout;


	for(int i = 0; i < _numOfSimpleCommands; i++) {
		dup2(fdin, 0);
		close(fdin);
		
		if(i == _numOfSimpleCommands-1) {
			if(_outFile != NULL) {
				if(_appendOut == 1) {
					fdout = open(_outFile, O_WRONLY | O_APPEND | O_CREAT, 0600);
					if(fdout < 0) {
						perror("Error opening the out file");
						exit(1);
					}
				} else {
					fdout = open(_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
					if(fdout < 0) {
						perror("Error opening the out file");
						exit(1);
					}
				}
			} else {
				fdout = dup(tmpout);
			}
		} else {
			int fdpipe[2];
			pipe(fdpipe);
			fdout=fdpipe[1];
			fdin=fdpipe[0];
		}

		dup2(fdout, 1);
		close(fdout);
	
		ret = fork();

		if(ret == 0) {
			running = true;
			if(strcmp(_simpleCommands[i]->_arguments[0], "printenv") == 0) {
				char **ptr = environ;
				while(*ptr != NULL) {
					printf("%s\n", *ptr);
					ptr++;
				}

				exit(0);
			}
			execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
			perror("Error executing command");
			_exit(1);
		} else if ( ret < 0 ) {
			perror("Fork Error");
			exit(2);
		} else {
			waitpid(ret, NULL, 0);
		}
	
	}
	
	dup2(tmpin, 0);
	dup2(tmpout, 1);
	close(tmpin);
	close(tmpout);

	if(_background == 0) {
		waitpid(ret, NULL, 0);
		clear();
		prompt();
	} else {
		int i = 0;
		for(i = 0; i < 1024; i++) {
			if(backgroundPID[i] == 0)
				break;
		}
		backgroundPID[i] = ret;
		clear();
	
	}
	// Print contents of Command data structure
	//print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec

	// Clear to prepare for next command
	//clear();
	
	// Print new prompt
	//prompt();
}

// Shell implementation

void
Command::prompt()
{
	if (getenv("PROMPT") != NULL) {
		if (isatty(0)){
			printf("%s>",getenv(strdup("PROMPT")));
			fflush(stdout);
		}
	}
	else if(isatty(0)) {
		printf("kartik's shell>");
		fflush(stdout);
	}
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

extern "C" void disp( int sig )
{
	Command::_currentCommand.clear();
}

extern "C" void killzombie( int sig )
{
	while(waitpid(-1, NULL, WNOHANG) > 0); 
}

extern "C" void suspendor(int sig) {
	printf("%s", "[1]	 suspended\n");
	/*if(running == true) {
		printf("%s", "[1]	 suspended\n");
		Command::_currentCommand.clear();
		Command::_currentCommand.prompt();
	} else {
		printf("%s", "\n");
		Command::_currentCommand.prompt();
	}*/
	/*int i = 0;
	for(i = 0; i < 1024; i++) {
		if(backgroundPID[i] == 0)
			break;
	}
	backgroundPID[i] = getpid();*/
	
}


int main()
{
	backgroundPID = (int*)calloc(1024,sizeof(int));

	struct sigaction signalAction;
	signalAction.sa_handler = disp;
	sigemptyset(&signalAction.sa_mask);
	signalAction.sa_flags = SA_RESTART;
	int error = sigaction(SIGINT, &signalAction, NULL );
	if ( error )
	{
		perror( "sigaction" );
		exit( 100 );
	}

	struct sigaction signalAction1;
	signalAction1.sa_handler = killzombie;
	sigemptyset(&signalAction1.sa_mask);
	signalAction1.sa_flags = SA_RESTART;

	int err1 = sigaction(SIGCHLD, &signalAction1, NULL);
	if(err1) {
		perror("sigaction");
		exit( 200 );
	}

	struct sigaction signalAction2;
	signalAction2.sa_handler = suspendor;
	sigemptyset(&signalAction2.sa_mask);
	signalAction2.sa_flags = 0;

	int err2 = sigaction(SIGTSTP, &signalAction2, NULL);
	if(err2) {
		perror("sigaction");
		exit( 300 );
	}

	Command::_currentCommand.prompt();
	yyparse();
}

