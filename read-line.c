
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_BUFFER_LINE 2048

int HISTORY_SIZE = 100;
// Buffer where line is stored
int line_length = 0;
char *line_buffer = NULL;

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char **history = NULL;
int history_length = 0;
static int tracker = 0;

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {
  if(line_buffer == NULL) {
	line_buffer = (char*) malloc(MAX_BUFFER_LINE*sizeof(char));
  }
  if(history == NULL) {
    history = (char **) malloc(sizeof(char*)* HISTORY_SIZE);
    int z = 0;
    for(z = 0; z < HISTORY_SIZE; z++) {
      history[z] = (char *) malloc(sizeof(char)*MAX_BUFFER_LINE);
    }
  }

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  tracker = 0;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32 && ch != 127) {
      // It is a printable character. 

      // Do echo
      write(1,&ch,1);

      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break; 

	  // add char to buffer if in the center or in the center
	  	if(tracker != line_length) {
		  int i = 0;
		  
		  line_length++;
		  //shifting and making room for new stuff!!!
		  for(i = line_length; i > tracker; i--)
			line_buffer[i] = line_buffer[i-1];
		  
		  line_buffer[tracker] = ch;
		  
		  for(i = tracker+1; i < line_length; i++) {
			  ch = line_buffer[i];
			  write(1, &ch, 1);
		  }
		  //shoving everything in accordingly
		  ch = 8;
		  for(i = tracker+1 ; i < line_length; i++)
			write(1, &ch, 1); 
		} else {
		  line_buffer[line_length]=ch;
		  line_length++;
		}

	  tracker++;
    } else if (ch==10) {
      // <Enter> was typed. Return line
      
      // Print newline
      write(1,&ch,1);

      if(history_length == HISTORY_SIZE)
      {
        HISTORY_SIZE *=2; 
        history = (char**) realloc(history, HISTORY_SIZE*sizeof(char*));
      }

      line_buffer[line_length]= 0;
      if(line_buffer[0] != 0)
      {
        history[history_length] = strdup(line_buffer);
        history_length++;
        history_index++;
      }

      break;
    } else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    } else if (ch == 8 || ch == 127) {
      // <backspace> was typed. Remove previous character read.
	  if (line_length <= 0 || tracker <= 0 || tracker > line_length) 
	    continue;

      if(line_length == tracker && tracker > 0) {
        // Go back one character
        ch = 8;
        write(1,&ch,1);

        // Write a space to erase the last character read
        ch = ' ';
        write(1,&ch,1);

        // Go back one character
        ch = 8;
        write(1,&ch,1);
		
		line_buffer[line_length-1] = '\0';
        // Remove one character from buffer
        line_length--;
        tracker--;
      } else if(tracker < line_length && tracker > 0) {
        // Go back one character
        
        int i;
        for (i = tracker; i < line_length; i++)
            line_buffer[i-1] = line_buffer[i];

		// Go back one character
        ch = 8;
        write(1,&ch,1);  

        // Write a space to erase the last character read
        ch = 32;
        write(1,&ch,1);                   
        
        // Go back one character
        ch = 8;
        write(1,&ch,1); 
        
		line_length--;
        tracker--;
        
        for (i = tracker; i < line_length; i++)
        {
            ch = line_buffer[i];
            write(1, &ch, 1);
        }
        
        ch = ' ';
        write(1,&ch,1);
        
        ch = 8;
        write(1,&ch,1);
        
        for (i = tracker; i < line_length; i++)
        {
            write(1,&ch,1);
        }
      }
    }else if(ch == 1) {
      //Move to beginning of line
      int i;
      for(i = tracker; i > 0; i--)
      {
        ch = 8;
        write(1,&ch,1);
        tracker--;
      }
    } else if(ch == 4) { 
      //Remove char at position x
      if (tracker >= line_length || tracker < 0) 
		continue;
            
		// Write a space to erase the last character read
		ch = ' ';
		write(1,&ch,1);

		// Go back one character
		ch = 8;
		write(1,&ch,1);

		int i;
		for (i = tracker; i < line_length - 1; i++)
		{
		    line_buffer[i] = line_buffer[i + 1];
		}
		
		line_length--;
		//tracker--;

		for (i = tracker; i < line_length; i++)
		{
		    ch = line_buffer[i];
		    write(1,&ch,1);
		}

		ch = ' ';
		write(1,&ch,1);
		
		ch = 8;
		write(1,&ch,1);
		
		for (i = tracker; i < line_length; i++)
		{
		    write(1,&ch,1);
		}
    } else if(ch == 5) {
      //Move to end of line
      int i;
      for(i = tracker; i < line_length; i++)
      {
        char ch = line_buffer[i];
        write(1, &ch, 1);
        tracker++;
      }
    } else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
      	// Up arrow. Print next line in history.

      	// Erase old line
      	// Print backspaces
        if(history_index > 0 && history_index <= history_length)
          history_index--;
        else if(history_index <= 0)
          history_index = 0;

      	int i = 0;
      	for (i =0; i < line_length; i++) {
      	  ch = 8;
      	  write(1, &ch, 1);
      	}

      	// Print spaces on top
      	for (i =0; i < line_length; i++) {
      	  ch = ' ';
      	  write(1,&ch,1);
      	}

      	// Print backspaces
      	for (i =0; i < line_length; i++) {
      	  ch = 8;
      	  write(1,&ch,1);
      	}	

      	// Copy line from history
      	strcpy(line_buffer, history[history_index]);
      	line_length = strlen(line_buffer);

      	// echo line
      	write(1, line_buffer, line_length);
      } else if (ch1==91 && ch2==66) {
        // Down arrow. Print next line in history.

        // Erase old line
        // Print backspaces
        if(history_index >= 0 && history_index <= history_length-1)
          history_index++;

        int i = 0;
        for (i =0; i < line_length; i++) {
          ch = 8;
          write(1,&ch,1);
        }

        // Print spaces on top
        for (i =0; i < line_length; i++) {
          ch = ' ';
          write(1,&ch,1);
        }

        // Print backspaces
        for (i =0; i < line_length; i++) {
          ch = 8;
          write(1,&ch,1);
        } 
        // Copy line from history
        strcpy(line_buffer, history[history_index]);
        line_length = strlen(line_buffer);

        // echo line
        write(1, line_buffer, line_length);
      } else  if(ch1 == 91 && ch2 == 68) {
        if(tracker > 0)
          {
            ch = 8;
            write(1,&ch,1);
            tracker--;
          }
      } if(ch1 == 91 && ch2 == 67) {
        if(tracker < line_length && tracker >= 0)
          {
            char ch = 27;
            write(1, &ch, 1);
            ch = 91;
            write(1, &ch, 1);
            ch = 67;
            write(1, &ch, 1);
            tracker++;
          }
      } 
    }

  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  return line_buffer;
}

