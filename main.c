//*****************************************************************************
//
//  name:   pina_shell - A screen reader shell for Linux that speaks the stdin,
//                       the stdout and the stderr.
//
//  description: A screen reader shell for Linux that speaks with the open source
//               espeak-ng TTS, the stdin, the stdout and the stderr.
//
//              The pina_shell is a fork of the lsh (Libstephen Shell) shell,
//              with many improvements.
//
//  TTS:    you need to install the espeak-ng TTS for linux. 
//
//  to compile: $ make
//  
//  to run:     $ ./pina_shell
//
//  file:   main.c
//
//  author: Original 280 lines Stephen Brennan ( very simples c code Shell )
//          Current  1120 lines João Carvalho  ( A screen reader shell )
//
//  url:    Original - Stephen Brennan - Tutorial - Write a Shell in C
//          https://brennan.io/2015/01/16/write-a-shell-in-c/
//          
//          github - brenns10 / lsh
//          https://github.com/brenns10/lsh
//
//          Current:
//          Github  - joaocarvalhoopen - pina_shell
//          https://github.com/joaocarvalhoopen/pina_shell--A-screen-reader-shell-for-Linux          
//
//  date:   Original: Thursday,  8 January 2015
//          Current:  Saturday,  18 August 2023
//
//  brief:  Original: LSH (Libstephen Shell)
//          Current:  pina_shell ( A screen reader shell for Linux )
//
//  License: MIT Open Source License
//
//*****************************************************************************

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// jnc begin
#include <ctype.h>
#include <termios.h>
// jnc end

//  Function Declarations for builtin shell commands:
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

/// List of builtin commands, followed by their corresponding functions.
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}


// jnc begin
int lsh_execute(char **args, int bool_int);

char **lsh_split_line(char *line);

// Node structure for a linked list
typedef struct Node {
    char *data;
    struct Node *next;
    struct Node *prev;
} Node;

// Linked list structure
typedef struct LinkedList {
    Node *head;
    Node *tail;
    int size;
} LinkedList;


// Global linked list
LinkedList list;

void speak_audio(char *text) {

    char *espeak_ng   = "espeak-ng";
    char *punctuation = "--punct";

    // Allocate memory for the new string:
    // length of text + 2 for the double quotes + 1 for the null-terminator
    char * text_with_quotes = (char*) malloc(
                + strlen( espeak_ng )   + 2
                + strlen( punctuation ) + 2
                + strlen( text ) + 3 
                + 2 );   // 2 Quotes (not the double quotes)

    sprintf( text_with_quotes, "%s %s \"'%s'\"", espeak_ng, punctuation, text ); 

    // printf("text_with_quotes: %s\n", text_with_quotes);

    char ** speak_ng_command_args = lsh_split_line( text_with_quotes );

    // Print to verify speak_ng_command_args
    // for (int i = 0; i < 2; i++) {
    //     printf("args[%d]: %s\n", i, speak_ng_command_args[i]);
    // }

    int dont_gen_pipes_dont_capture_output = 0;
    int status = lsh_execute( speak_ng_command_args, dont_gen_pipes_dont_capture_output );

    free( text_with_quotes );
    free( speak_ng_command_args );
}

void speak_audio_char(char char_value) {
    char char_str[2];
    char_str[0] = char_value;
    char_str[1] = '\0';
    speak_audio( char_str );
}


// ***************************************************************
// Double linked list of strings ( implementation ).

// Function to initialize the linked list
void list_init(LinkedList *list) {
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

// Append a string to the beginning of the linked list
void list_append_first(LinkedList *list, char *str) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = strdup(str);  // Copy the string
    newNode->next = list->head;
    newNode->prev = NULL;

    if (list->head) {
        list->head->prev = newNode;
    } else {
        // If the list is empty, new node is also the tail
        list->tail = newNode;
    }

    list->head = newNode;
    list->size++;
}

// Get data at a specific index
char *list_get_at(LinkedList *list, int index) {
    if (index < 0 || index >= list->size) {
        return NULL;  // Out of bounds
    }

    Node *current = list->head;
    for (int i = 0; i < index; i++) {
        current = current->next;
    }
    return current->data;
}

// Delete a node from the list
void list_delete(LinkedList *list, Node *node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        // If node is head
        list->head = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    } else {
        // If node is tail
        list->tail = node->prev;
    }

    free(node->data);
    free(node);
    list->size--;
}

// Get the first node
Node *list_first(LinkedList *list) {
    return list->head;
}

// Get the last node
Node *list_last(LinkedList *list) {
    return list->tail;
}

// Get the next node
Node *list_next(Node *node) {
    return node->next;
}

// Get the previous node
Node *list_prev(Node *node) {
    return node->prev;
}

// Get the node data
char *list_get_data(Node *node) {
    return node->data;
}

// Function to free up memory for entire list
void list_free(LinkedList *list) {
    Node *current = list->head;
    while (current) {
        Node *nextNode = current->next;
        free(current->data);
        free(current);
        current = nextNode;
    }
}

void list_print_reverse(LinkedList *list) {
    int list_size = list->size;    
    
    if (list_size > 15 /* 12 */) {
        // Remove the last element
        list_delete( list, list->tail );
    }
    
    list_size = list->size - 1;
    Node *current = list->tail;  // Start by the end (tail)
    while (current) {
        printf(" %2d : %s\n", list_size--, current->data);
        current = current->prev;  // Navigate to the previous element.
    }
}

// ***************************************************************


int FALSE = 0;
int TRUE  = 1;

int is_all_whitespace(char *str) {
    while (*str) {
        if (!isspace((unsigned char)*str))
            return FALSE;
        str++;
    }
    return TRUE;
}

// jnc end


// ____________________________________________________________________________
// Builtin function implementations.


///  @brief Builtin command: change directory.
///  @param args List of args.  args[0] is "cd".  args[1] is the directory.
///  @return Always returns 1, to continue executing.
int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "pina_shell: expected argument to \"cd\"\n");

    // jnc inicio

    speak_audio("pina_shell: expected argument to cd");
    
    // jnc fim

  } else {
    if (chdir(args[1]) != 0) {
      perror("pina_shell");
    }
  }
  return 1;
}


/// @brief Builtin command: print help.
/// @param args List of args.  Not examined.
/// @return Always returns 1, to continue executing.
int lsh_help(char **args)
{
  int i;
  printf("Joao Carvalho - pina_shell\n");
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/// @brief Builtin command: exit.
/// @param args List of args.  Not examined.
/// @return Always returns 0, to terminate execution.
int lsh_exit(char **args)
{
  return 0;
}

// jnc begin

void replace_newline_space_tab_with_char_name(const char *src, char *dest, char *read_context_txt) {
    // Copy the context text to the destination (std out: \n or std err: \n).
    strcat(dest, read_context_txt);

    while (*src) {
        if (*src == '\n') {
            strcat(dest, " newline ");
            src += 1;  // Skip both characters '\n'
        } if (*src == '\t') {
            strcat(dest, " tab ");
            src += 2;  // Skip both characters '\t'
        } if (*src == ' ' ) {
            strcat(dest, " space ");
            src += 1;  // Skip both characters ' '
        } else {
            strncat(dest, src, 1);
            src++;
        }
    }
}

char * join_args_with_space( char **args ) {
    // Step 1: Calculate total length required
    size_t total_len = 0;
    for (int i = 0; args[i] != NULL; i++) {
        total_len += strlen(args[i]) + 1;  // +1 for space or null terminator
    }
    if (total_len == 0) return NULL;

    // Step 2: Allocate memory
    char *result = malloc(total_len);
    if (!result) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    // Step 3: Copy strings
    char *dest = result;
    for (int i = 0; args[i] != NULL; i++) {
        if (i > 0) {
            *dest++ = ' ';
        }
        size_t len = strlen(args[i]);
        strncpy(dest, args[i], len);
        dest += len;
    }
    *dest = '\0';

    return result;
}


// jnc end


///  @brief Launch a program and wait for it to terminate.
///  @param args Null terminated list of arguments (including program).
///  @return Always returns 1, to continue execution.
int lsh_launch(char **args, int bool_int)
{
  pid_t pid;
  int status;

// jnc brgino
  int pipe_fd__std_out[2];
  int pipe_fd__std_err[2];

  if (bool_int == 1) {
    if (pipe(pipe_fd__std_out) == -1 || pipe(pipe_fd__std_err) == -1) {
      perror("pipe");
      exit(EXIT_FAILURE);
    }
  }
// jnc end

  pid = fork();
  if (pid == 0) {

// jnc begin
      if (bool_int == 1) {
        // std_out
        close(pipe_fd__std_out[0]);
        dup2(pipe_fd__std_out[1], STDOUT_FILENO);
        close(pipe_fd__std_out[1]);

        // std_err
        close(pipe_fd__std_err[0]);
        dup2(pipe_fd__std_err[1], STDERR_FILENO);
        close(pipe_fd__std_err[1]);
    }
// jnc fim


      // IMPORTANT: I comment the following code so I could use the system call
      //            that calls the shell program inside the shell.
      //            This way I can already use the redirection with the pipes implemented
      //            inside the bash shell.

      // Child process

      // if (execvp(args[0], args) == -1) {
      //   perror("lsh");
      // }


      // Child process
    
// jnc begin

      char * command = join_args_with_space(args);

      if (execl("/bin/sh", "sh", "-c", command, (char *) NULL) == -1) {
          perror("execl");
          exit(EXIT_FAILURE);
      }

      free( command );

// jnc end

      exit(EXIT_FAILURE);
  } else if (pid < 0) {
      // Error forking
      perror("pedro_pina");
  } else {
      // Parent process

// jnc begin
      if (bool_int == 1) {
    
        // std_out
        char buffer__std_out[1024 * 20];
        ssize_t bytes_read__std_out;
      
        // std_err
        char buffer__std_err[1024 * 20];
        ssize_t bytes_read__std_err;

        close(pipe_fd__std_out[1]);
        close(pipe_fd__std_err[1]);

        // std_out
        while ((bytes_read__std_out = read(pipe_fd__std_out[0], buffer__std_out, sizeof(buffer__std_out) - 1)) > 0) {
            buffer__std_out[bytes_read__std_out] = '\0';
            printf("Parent read std out:\n%s", buffer__std_out);
        }

        // std_err
        while ((bytes_read__std_err = read(pipe_fd__std_err[0], buffer__std_err, sizeof(buffer__std_err) - 1)) > 0) {
            buffer__std_err[bytes_read__std_err] = '\0';
            printf("Parent read std error:\n%s", buffer__std_err);
        }

        close(pipe_fd__std_out[0]);
        close(pipe_fd__std_err[0]);
    

        char buffer_clone_replaced__std_out[10000] = {0};
        char buffer_clone_replaced__std_err[10000] = {0};

        char * read_context_txt;
      
        if (buffer__std_err != '\0' ) {
            read_context_txt = "stdout: \n";
            replace_newline_space_tab_with_char_name( buffer__std_out, buffer_clone_replaced__std_out, read_context_txt );
            
            // debug
            // printf("=>debug: %s\n", buffer_clone_replaced__std_out);
        }

        if (buffer__std_err[0] != '\0' ) {
            read_context_txt = "stderr: \n";
            replace_newline_space_tab_with_char_name( buffer__std_err, buffer_clone_replaced__std_err, read_context_txt );

            // debug
            // printf("=>debug: %s\n", buffer_clone_replaced__std_err);
        }

        // printf("%s", buffer_clone_replaced);

        // Executes other child forked process the espeak-ng to speak the
        // stdout (ouput) and stdin (input) of the commando executable process.
        // The father captured the child process.
        
        speak_audio( buffer_clone_replaced__std_out );

        speak_audio( buffer_clone_replaced__std_err );
    }
// jnc end


    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

///  @brief Execute shell built-in or launch program.
///  @param args Null terminated list of arguments.
///  @return 1 if the shell should continue running, 0 if it should terminate
int lsh_execute(char **args, int bool_int)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args, bool_int);
}

///  @brief Read a line of input from stdin.
///  @return The line from stdin.
char *lsh_read_line(void)
{
/*
#ifdef LSH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // We received an EOF
    } else  {
      perror("lsh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
*/
#define LSH_RL_BUFSIZE 1024


  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "pina_shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  // jnc begin

  struct termios oldt, newt;

  // Get the current terminal settings
  tcgetattr(STDIN_FILENO, &oldt);

  // Make a copy of the current settings to modify them
  newt = oldt;

  // Modify the terminal settings
    newt.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo

  // Set the new terminal settings
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  // jnc end

  Node * node_current = list_first( &list );

  int flag_before_up_arrow = 1;

  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      
      // jnc begino
      // Restore the old terminal settings
      tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
      // jnc fim

      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      position++;  // aqui

      printf("\n");

      // jnc  begin
      // Restore the old terminal settings
      tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
      // jnc fim

      return buffer;
    } else {
      // buffer[position] = c;

      // jnc begin

      // Speak the character
      switch (c)
      {
        case ' ':
          buffer[position] = c;
          position++;
          printf("%c", c);
          speak_audio("space");

          // printf("\n position: %d\n", position);

          if (position > 1 && buffer[position - 2] != ' ' && buffer[position - 2] != '\t') {
              // Speak last word if last character wasn't a space.
              
              // printf("\n buffer[position - 2]: %d\n", buffer[position - 2]);

              // Find the beginning of the last word.
              int n = 2; 
              int flag_found_begin_prev_word = 0;
              while (flag_found_begin_prev_word == 0  && position - n >= 0) {
                  char char_tmp = buffer[position - n];

                  // printf("\n n inside cycle: %d\n", n);    
                  // printf("\n char_tmp: [%c]\n", char_tmp);

                  if (char_tmp == ' ' || char_tmp == '\t' || position - n == 0) {
                      flag_found_begin_prev_word = 1;
                      break;
                  } else {
                      n++;
                  }
              }
                        
              // Copy the last word to a null terminated buffer.
              char last_word_buffer[300] = {0};
              strncpy(last_word_buffer, &buffer[position - n], n - 1);

              // printf("\n n: %d\n", n);
              // printf("\n last_word_buffer: %s\n", last_word_buffer);

              speak_audio( last_word_buffer );
          }
          break;
        case '\t':
          buffer[position] = c;
          position++;
          printf("%c", c);
          speak_audio("tab");
          break;
        // case '-':
        //   buffer[position] = c;
        //   position++;
        //   printf("%c", c);
        //   speak_audio("minus");
        //   break;  
        case '\b':
        case 127:  // 127 The ASCII para backspace DEL in same terminal's shell.
          speak_audio("backspace");
        
          if (position > 0) {

            // speak_audio_char( c );

            if (buffer[position - 1] == '\t') {  // backspace em tab.
                speak_audio( "tab" );

                // There should be a better way to do this.
                // It has to make the backspace 5 times, then print 5 spaces to
                // write over with spaces and erase the character.
                // that are on the screen, then it does backspace again. 
                printf("\b\b\b\b \b");
            } else {
                if (buffer[position - 1] == ' ') {  // backspace with space.
                    speak_audio( "space" );
                } else {
                    speak_audio_char( buffer[position - 1] );
                }

                // Has to do backspace, then print of one space to erase the
                // character that is on the screen e then backspace again. 
                printf("\b \b");
            } 

            buffer[position] = '\0';
            position--;
            // And also doesn't increment the position variable because the
            // character has been erased and the current is a erasing character
          } else {
            speak_audio("Empty line");
          }
          break;
        case 0x1B:  
           // Escape character  0x1B '[' 'A'   // 'A' - UP Arrow and 'B' - DOWN Arrow
          
          // buffer[position] = c;
          // position++;
          // printf("%c", c);

          // Read a character
          c = getchar();
          if ( c != '[' ) {
              // It's not the up or down arrows characters.
              printf("%c", c );
              // printf("Character escape followed by Square bracket" );
              break;
          }

          // Read a character
          c = getchar();
          if ( c != 'A' && c != 'B' ) {
              // It's not the up or down arrows characters.
              printf("%c", c );
              break;
          }
          printf(" ");

          
          int flag_end_list = 0;

          if (c == 'A') {
              // UP ARROW

              speak_audio("up arrow");

              if (flag_before_up_arrow ==  1) {
                  // Shows the current line.
                  // Pass
                  flag_before_up_arrow = 0;
                  
              } else {
                
                // Advances to the next line, that means one line up.

                if (node_current != NULL) {
                  Node * node_tmp = list_next( node_current );

                  if (node_tmp == NULL) {
                      // espeak-ng end list.
                      speak_audio("end list");
                      flag_end_list = 1;
                      // position--;   // aqui
                      // This is because the espeak-ng seams to be putting one
                      // more character in the buffer.
                      printf("\b");
                      break;
                  } else {
                    node_current = node_tmp;
                  }
                }
              }
          } else if (c == 'B') {
              // DOWN ARROW

              speak_audio("down arrow");
              
              // int flag_end_list = 0;

              if (flag_before_up_arrow ==  1) {
                  // Shows the current line.
                  
                  // Pass
                  
                  // flag_before_down_arrow = 0;
                  break;
                  
              } else {
                // Comes back to the previous line, that means one line down.

                if (node_current != NULL) {
                  Node * node_tmp = list_prev( node_current );

                  if (node_tmp == NULL) {
                      // espeak-ng end list.
                      speak_audio("begin list");
                      flag_end_list = 1;
                      // This is because the espeak-ng seams to be putting one
                      // more character in the buffer.
                      printf("\b");   
                      
                      break;
                  } else {
                    node_current = node_tmp;
                  }
                }
              }
          } 


          if (flag_end_list == 0) {
              if (node_current != NULL) {
                  // Erases the previous line.
                                  
                  // printf("\r                                                                            \r");

                  // Moves the cursor to the beginning of the line
                  // and writes over the line with spaces.
                  if (position >= 0) {
                      printf("\r%*s", (int)(strlen(buffer) + 60), "");
                  } 

                  // printf("\r%*s", (int)strlen(buffer - 1), "");

                  char * my_str = (char *) node_current->data;
                  // Put's the curor at the beginning of the line.
                  printf( "\rpina_shell> %s", my_str );
                  fflush( stdout );

                  // Copies thea node_current->data to the buffer.
                  int i = 0;
                  while (my_str[i] != '\0') {
                      buffer[i] = my_str[i];
                      i++;
                  }
                  buffer[i] = '\0';
                  // position = i + 1;
                  position = i;
                 
                  speak_audio( my_str );
                  // node_current = node_current->next;
              }
              // } else {
              //    speak_audio("Empty line");  
              // }
          }
                    
          break;
        
        default:
label:    buffer[position] = c;
          position++;
          printf("%c", c);
          
          // printf("\n up arrow [%d]:(%d) \n", c, c);
          
          char char_str_2[2];
          char_str_2[0] = c;
          char_str_2[1] = '\0';
          speak_audio( char_str_2 );
          break;
      }

      // if (c == " " || c == "\t" || c == "\r" || c == "\n") {
      
      // jnc end

    }
    // position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "pina_shell: allocation error\n");

        // jnc end

        // Restore the old terminal settings
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

        // jnc fim

        exit(EXIT_FAILURE);
      }
    }
  }

/*
#endif
*/

}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

///  @brief Split a line into tokens (very naively).
///  @param line The line.
///  @return Null-terminated array of tokens.
 

/*
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		      free(tokens_backup);
          fprintf(stderr, "lsh: allocation error\n");
          exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

*/



// jnc begin

char **lsh_split_line(char *line) {
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    char *start = line, *end = line;
    while (*end) {
        // Skip whitespace
        while (*end && strchr(LSH_TOK_DELIM, *end)) {
            end++;
        }
        start = end;

        // Handle quoted strings
        if (*end == '"') {
            start++;  // move past the starting quote
            end++;
            while (*end && *end != '"') {
                end++;
            }
            if (*end) {
                tokens[position] = strndup(start, end - start);
                end++;  // skip the ending quote
            } else {
                tokens[position] = strdup(start);  // unmatched quote, copy the rest
            }
        } else {
            // Find the next delimiter
            while (*end && !strchr(LSH_TOK_DELIM, *end) && *end != '"') {
                end++;
            }
            tokens[position] = strndup(start, end - start);
        }

        if (!tokens[position]) {
            fprintf(stderr, "lsh: allocation error\n");
            exit(EXIT_FAILURE);
        }
        position++;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    tokens[position] = NULL;
    return tokens;
}


// jnc end



/// @brief Loop getting input and executing it.
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;


  // jnc begin

  // LinkedList list;
  list_init(&list);

  
  /*
    printf("First element: %s\n", list_get_at(&list, 0));
    printf("Second element: %s\n", list_get_at(&list, 1));
  */

  speak_audio("Pina shells is ready.");
  
  // jnc end

  do {
      printf("pina_shell> ");

// jnc begin
  
      // 1. Asks for the next command.
      speak_audio("Next command!");

// jnc end

      line = lsh_read_line();
    

// jnc begin 

      if ( //   line == NULL 
           // || line[0] == '\0'
           // || line[0] == ' '
           // || line[0] == '\t'
           is_all_whitespace( line ) 
           ) {
          speak_audio("No command to execute."); 
          status = 1;
          continue;
      }

      // 2. espeak-ng of the line containing the command.
      speak_audio(line);

      // 3. Adds the command to the list od past executed commands.
      list_append_first(&list, line);

      printf("\nComand prev reverse list:\n");
      list_print_reverse(&list);

      /*  
      // NOTA: Pina didn't liked the idea of asking for confirmation after
      //       entering each command.

      // 3. espeak-ng.
      speak_audio("Execute command? yes or no.");

      printf("y/n ? : ");

      // 4. Reads the line with the response of the user y or n or nothing.
      char *line_2;
      line_2 = lsh_read_line();
      if (line_2 != NULL && line_2[0] !='y') {
          free(line_2);
 
          // Goes to the next iteration
          continue;
      }
      free(line_2);
      */

// jnc end
    

    args = lsh_split_line(line);
    status = lsh_execute(args, 1);

    free(line);
    free(args);

  } while (status);

  // jnc begin
  list_free(&list);
  // jnc end  

}


///  @brief Main entry point.
///  @param argc Argument count.
///  @param argv Argument vector.
///  @return status code
int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}

