#include <stdio.h>  //printf, fgets, perror
#include <stdlib.h>   //exit, EXIT_FAILURE
#include <unistd.h>   //fork, execvp, dup2, pipe
#include <string.h>    //strtok, strlen, strcmp
#include <sys/wait.h>   //wait, waitpid
#include <stdbool.h>    //for true/false support
#include <fcntl.h> //This is used to open files
//This will count the processes that are running in the background we dont want more than 4
int background_count = 0;
//This will store the string user has give
char input[500];
//This will store the arguments give by user
char *args[6];
//This will store the commands given by the user
char *commands[5]; // store pipe segments

// Tokenize a command and detect &
int tokenx(char *user_input, char *args[], bool *is_background) {
    int i = 0; //iterator
     //this splits the input by spaces
    char *ttt = strtok(user_input, " ");
    //loop goes up to 5 tokens
    while (ttt != NULL && i < 5) {
            //We check for the & where we have to send it to the background
        if (strcmp(ttt, "&") == 0) {
            *is_background = true;
           //we check if the string ends with & then ls & or echo hello&
        } else if (ttt[strlen(ttt) - 1] == '&') {
            ttt[strlen(ttt) - 1] = '\0';
            *is_background = true;
            if (strlen(ttt) > 0)
                args[i++] = ttt; //adds remaing part to the array
        } else {
            args[i++] = ttt;
        }
         //This is used for loop it knows where to start off
        ttt = strtok(NULL, " ");
    }

    args[i] = NULL;
    return i;
}
//--------------We are creating a function that deals with pipes |------------
// We will use the command "ls -l | grep .c | wc -l
//ls -l will list all the files
// grep .c will filter all the files with c
// wc -l will count the files
// All of this needs to work independently but need to be connected somehow
//Methodology : We use fork() to run ech chunk in its own child
//2. then we use dup2 to hook up pipe connect
// we use execvp() in order to run the command
int piper(char *input){
       //This function accepts input from the user
         char *commands[5]; //we store commands in this
         int limit = 0; //this variable will store the number of commands
       //Split the string give by user at | and get the commands
        char *commander_safeguard = strtok(input, "|");  //we are spliting the string at |
         //here we are looping and tokenizing
            while (commander_safeguard != NULL && limit < 5) { //We use this loop to get all the commands
             //We store them in commands
               commands[limit++] = commander_safeguard;
                    //this is used to keep moving forward
                commander_safeguard = strtok(NULL, "|");
            }
    //Now we will create piptes between commands
             //This create 4 pipefd
            int pipefd[2 * (4)];//we are using this array
         //Each file has2 descriptors
            for (int i = 0; i < limit - 1; i++) {
                if (pipe(pipefd + i * 2) < 0) { //we throw error
                    perror("Pipe failed");
                    exit(0);
                }
            }
          //Now we will for a child
            for (int i = 0; i < limit; i++) {
              //Here we are forking our child
                pid_t pid = fork();
                 //We are in the child process
                if (pid == 0) {
                    //We take the input from the previous pipes read end
                    if (i > 0) {
                        dup2(pipefd[(i - 1) * 2], 0); // read end
                    }

                    //We set the output to current pipes write end
                    if (i < limit - 1) {
                        dup2(pipefd[i * 2 + 1], 1); // write end
                    }

                    // Close all pipe fds
                    for (int j = 0; j < 2 * (limit - 1); j++) {
                        close(pipefd[j]);
                    }

                    // Parse and run current command
                    bool bg = false;
                    char *cmd_args[6];
                     //Here we are one by one tokenizing and the executing the commands
                    tokenx(commands[i], cmd_args, &bg);
                    execvp(cmd_args[0], cmd_args);
                    perror("Execution failed");
                    exit(EXIT_FAILURE);
                }
            }
            //After all of the excution we will close the pipes
            for (int i = 0; i < 2 * (limit - 1); i++) {
                close(pipefd[i]);
            }
            //HERE WE ARE IN THE PARENT AND WAITING FOR THE CHILD TO FINISH
            for (int i = 0; i < limit; i++) {
                wait(NULL);
            }
}
//----------------------------------- << <-----------------------------------------
void greater(char *user_input){
      //We are taking array
    char *args[6];
    //this tracks if the commnd has & sign
    bool is_background = false;
     //this will hold the left side of the command
    char *commander_safeguard = NULL;
    char *filez = NULL;
    int redir_type = 0; // 1 = <, 2 = >, 3 = >>

    // Scan the string manually to find operator
    //Here we are looping over user input and then we are separating input
    for (int i = 0; user_input[i]; i++) {
        if (user_input[i] == '<') { //in this case
            redir_type = 1; //we know its <
            user_input[i] = '\0';  // Split the string at '<'
            commander_safeguard = user_input; //we store it in command
            filez = user_input + i + 1;
            break;
        }
        else if (user_input[i] == '>') {
            if (user_input[i + 1] == '>') {
                redir_type = 3;
                user_input[i] = '\0';          // Split at '>'
                user_input[i + 1] = '\0';      // Null-terminate >>
                commander_safeguard = user_input;
                filez = user_input + i + 2;
            } else {
                redir_type = 2;
                user_input[i] = '\0';  // Split at '>'
                commander_safeguard = user_input;
                filez = user_input + i + 1;
            }
            break;
        }
    }

    if (!commander_safeguard || !filez) {
        printf("Invalid redirection format.\n");
        return;
    }

    // Remove leading spaces from filez
    while (*filez == ' ') filez++;
    //Here we tokenize the given statement
    tokenx(commander_safeguard, args, &is_background);
   //Here is the execution
    pid_t pid = fork();
    if (pid == 0) {
        int fd;
        //Here we are working according to the user unput operand 
        if (redir_type == 1) { // this is <
            fd = open(filez, O_RDONLY);
            if (fd < 0) {
                perror("user_input open failed");
                exit(EXIT_FAILURE);
            }
            dup2(fd, 0); // stdin
        } else if (redir_type == 2) {       //this is  >
            fd = open(filez, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("Output open failed");
                exit(0);
            }
            dup2(fd, 1); // stdout
        } else if (redir_type == 3) { //this is <<
            fd = open(filez, O_WRONLY | O_CREAT | O_APPEND, 0644);
            //Error handling
            if (fd < 0) {
                perror("Append open failed");
                exit(0);
            }
            dup2(fd, 1); // stdout
        }

        close(fd);
       //here we are using execvp
        execvp(args[0], args);
       //We throw an error if it is not working
        perror("Execution failed");
        exit(1);
    } else if (pid > 0) {
           //The parent is waiting
        wait(NULL);
    } else {
        perror("Fork failed");
    }
}
/////////////////////////////////
// Function to handle conditional execution using && and || operators
////////////////////////////////////
//HEre we are looking for condition like AND OR
void conditional_handler(char *input) {
    char *segments[5];      //To store up to 5 commands
    char *operators[4];     //To store up to 4 operators (;, &&, ||)
    int seg_count = 0;   //this is a segment counter
    char *p = input;
    char *start = p;

    // Manually parse input and extract commands/operators
    while (*p) {
       //we are looping over every command and checking ; & or ||
        if ((*p == ';' && *(p + 1) != ';') ||
            (*p == '&' && *(p + 1) == '&') ||
            (*p == '|' && *(p + 1) == '|')) {

            if (*p == ';') {
               //We store the operator ;
                operators[seg_count] = ";";
                *p = '\0'; //we end the current command with this
                segments[seg_count++] = start; //saving the command in segments
                start = p + 1; //prepares for the next command
            }
            else if (*p == '&') {
                operators[seg_count] = "&&"; //it skips two characters
                *p = '\0';
                segments[seg_count++] = start;
                start = p + 2; //that is why we add 2 to the pointer
                p++;  // skip next &
            }
            else if (*p == '|') {
                operators[seg_count] = "||";
                *p = '\0';
                segments[seg_count++] = start;
                start = p + 2;
                p++;  // skip next |
            }
        }
        p++; // moves pointers forward one character at a time
    }

    // Capture final segment
    segments[seg_count++] = start;

    int last_status = 0;

    // Execute each segment
    for (int i = 0; i < seg_count; i++) {
      //this is the main logic
     //  printf("[mbash25] Segment %d: '%s' \n", i+1, segments[i]);
        // Determine if this command should run based on previous status
        if (i > 0) {
            if (strcmp(operators[i - 1], "&&") == 0 && last_status != 0)
             //   printf("[mbash25] Skipping due to failed previous (&&) \n");
                continue; // Skip if previous failed
            if (strcmp(operators[i - 1], "||") == 0 && last_status == 0)
             //   printf("[mbash25] Skipping due to success previous ||\n");
                continue; // Skip if previous succeeded
        }

        // Tokenize and execute
        char *args[6];
        bool is_bg = false;
        tokenx(segments[i], args, &is_bg);

        pid_t pid = fork();
        if (pid == 0) {
            execvp(args[0], args);
            perror("Execution failed");
            exit(1);
        } else {
            int status;
            waitpid(pid, &status, 0); //waits for child to finish
             // it saves the exit status to work for the output OR and AND
            last_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
           //debugg
            printf("[mbash25] Command: '%s' exited with status: %d\n", args[0], last_status);

        }
    }
}
//Here we will be using ~~~~~~~~~~~
void tilted_life(char *input) {
    char *commander_safeguards[3];
    //this will keep a count
     int count = 0;

    // Split by ~ into 3 parts
    char *token = strtok(input, "~"); //we will split at ~
    while (token && count < 3) { //loop until 3 commands are found
        while (*token == ' ') token++; // trim spaces
        commander_safeguards[count++] = token;
        token = strtok(NULL, "~");
    }

    if (count != 3) { //we throw an error
        printf("Invalid usage of ~ (need exactly 3 commands)\n");
        return;
    }
    //here we are creating two pipes
    int pape1[2];
    int pape2[2];
    pipe(pape1);  // between cmd1 and cmd2
    pipe(pape2);  // between cmd2 and cmd3

    // First child
    //Here we will be forking the child
    pid_t pid1 = fork();
    if (pid1 == 0) {
        //We will be using the dupe to write out
        dup2(pape1[1], 1);  // stdout → pape1 write
        close(pape1[0]); close(pape1[1]); //we close
        close(pape2[0]); close(pape2[1]); //clsoe to avoid deadlocks

        char *args[6]; //we initialize arguments
        bool bg = false;  //we have bg
        tokenx(commander_safeguards[0], args, &bg);//now we tokenize our command
        execvp(args[0], args); //we execute it
        perror("FAILEDDDD"); //throw an error if it doesnt work
        exit(EXIT_FAILURE);
    }

    //We are forking a second child here
    pid_t pid2 = fork();
    if (pid2 == 0) {
       //Reads from pipe 1
        dup2(pape1[0], 0);  // stdin ← pape1 read
        //Sends output to pipe 2
        dup2(pape2[1], 1);  // stdout → pape2 write
        close(pape1[0]); close(pape1[1]); //close
        close(pape2[0]); close(pape2[1]); //close
        //SAME AS ABOVE
        char *args[6];
        bool bg = false;
        tokenx(commander_safeguards[1], args, &bg);
        execvp(args[0], args);
        perror("FAILEDDDD");
        exit(EXIT_FAILURE);
    }

    // Third child
     //We have our another fork call
    pid_t pid3 = fork();
    if (pid3 == 0) {
        dup2(pape2[0], 0);  //pape2 read
        close(pape1[0]); close(pape1[1]); //We close to save from deadlock
        close(pape2[0]); close(pape2[1]);
        //Same logic as before
        char *args[6];
        bool bg = false;
        tokenx(commander_safeguards[2], args, &bg);
        execvp(args[0], args);
        perror("Execution failed (cmd3)");
        exit(EXIT_FAILURE);
    }

    // Parent closes all pipe ends
    close(pape1[0]); close(pape1[1]);
    close(pape2[0]); close(pape2[1]);
//Here we are in the parent process
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);
}


/////////////////////////////////////////////
void plusing(char *input){
       //We check for the first position of +
      char *split_pos = strchr(input, '+');
      if(!split_pos){
          printf("Invalid usage of +\n");
          return; //it exits
      }
      *split_pos = '\0'; //we replace it by \0
       char *left = input; //points to the first command
       char *right = split_pos + 1; //points to the second command
      //It trims spaces
       while (*left == ' ')left++;
       while (*right == ' ') right++;
       //Here we check if its an empty strinig we will throw an error
       if(strlen(left) ==0 || strlen(right) == 0){
           printf("Invalid usage of + \n");
           return;
          }
         //Forking creating a child process
        pid_t pid1 = fork();
        if(pid1 == 0){
        //We are tokenizing the left comand into args1
          char *args1[6];
          bool bg1 = true;
          tokenx(left,args1,&bg1);
          execvp(args1[0],args1); //executing it here
          perror("failed");
          exit(EXIT_FAILURE);
       }
       else if (pid1>0){
          //We are in the parent process here
         if(background_count < 4){ //if background process limit is 4 increase it
           background_count ++;
           printf("backgrd process %d started \n", pid1);
         }
        //We wait for the first process to finish here
         else{ waitpid(pid1,NULL,0);}
        }
        else{
           perror("Fork failed");
         }
      //Forking for a second time here
       //same logic here
       pid_t pid2 = fork();
       if(pid2 == 0){
          char *args2[6];
          bool bg2 = true;
          tokenx(right, args2, &bg2);
          execvp(args2[0], args2);
          perror("Execution failed");
          exit(0);
         } else if(pid2 >0){
             if(background_count < 4){
                background_count++;
                printf("The process pid is : %d\n",pid2);
              }else {
                waitpid(pid2, NULL,0);
               }
            }else {
               perror("Fork failed");
            }
}
//___________________________________________
int main() {
   //We have a continuous loop going on
    while (1) {
        //we print the mbash25$ and wait for the user to give an input
        printf("mbash25$ ");
        fflush(stdout);
           //we read line by line
        if (!fgets(input, sizeof(input), stdin)) break;
        //we take the length of the string
        int len = strlen(input);
         //we check the length and at \0 instead of \n
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
         char *comment = strchr(input,'#'); //here we check the position of #
           if(comment){ //we replace it with \0
                 *comment = '\0';
              }
     //-------------------------NOW THE COMMANDS------------------------
        if (strcmp(input, "killterm") == 0) {
            printf("We are terminating mbash25$...\n");
            break;
        }
          // Check for ++ parallel operator
if (strstr(input, "++")) {
    char *split_pos = strstr(input, "++");
    if (!split_pos) {
        printf("Invalid usage of ++\n");
        continue;
    }

    // Split the input into left and right parts
    *split_pos = '\0';              // terminate left command
    char *left = input;             // left command
    char *right = split_pos + 2;    // right command after "++"

    // Trim spaces
    while (*left == ' ') left++;
    while (*right == ' ') right++;

    if (strlen(left) == 0 || strlen(right) == 0) {
        printf("Invalid usage of ++ (missing command)\n");
        continue;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        char *args1[6];
        bool bg1 = false;
        tokenx(left, args1, &bg1);
        execvp(args1[0], args1);
        perror("Execution failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        char *args2[6];
        bool bg2 = false;
        tokenx(right, args2, &bg2);
        execvp(args2[0], args2);
        perror("Execution failed");
        exit(EXIT_FAILURE);
    }

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    continue;
}
	if(strchr(input,'+')){
            plusing(input);
           continue;
        }



        // -------------------- PIPE SUPPORT ---------------------
        if (strchr(input, '|')) {
            piper(input);
        }
// ------------------------------WE ARE CHECKING FOR REDIRECTION
        if(strchr(input,'<')||strstr(input,">>")||strchr(input,'>')){
            greater(input);
           continue;
         }
//We are checking for && || ; operators
          if (strstr(input,"&&")|| strstr(input,"||")||strchr(input,';')){
           conditional_handler(input);
           continue;
            }
// Now we will work with the ++ or +
          if(strchr(input, '~')){
                tilted_life(input);
                continue;
        }

        // -------------------- NO PIPES (normal or background) ---------------------
        else {
            bool is_background = false;
            int num_tokens = tokenx(input, args, &is_background);
            if (num_tokens == 0) continue;

            pid_t pid = fork();

            if (pid == 0) {
                execvp(args[0], args);
                perror("Execution failed");
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                if (is_background) {
                    if (background_count < 4) {
                        printf("[mbash25$] background process %d started\n", pid);
                        background_count++;
                    } else {
                        printf("Too many background processes (limit = 4).\n");
                        waitpid(pid, NULL, 0);
                    }
                } else {
                    waitpid(pid, NULL, 0);
                }
            } else {
                perror("Fork failed");
            }
        }
    }

    return 0;
}
