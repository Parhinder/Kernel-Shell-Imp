/*
Parhinder Singh
UID: 117979046
Directory ID: psingh13
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <sysexits.h>
#include "command.h"
#include "executor.h"
#define OPEN_FLAGS (O_WRONLY | O_TRUNC | O_CREAT)
#define DEF_MODE 0664


static void print_tree(struct tree *t);
static int process_tree(struct tree *t, int input, int output);

int execute(struct tree *t) {

   /* print_tree(t); */ 
   return process_tree(t, 0, 0);
}

static int process_tree(struct tree *t, int input, int output) {
   pid_t pid;
   int status, arg, dup_result, recursive_ret, pipe_stat;
   int pipefd[2];
   /*check to see if node is a leaf*/   
   if(t->conjunction == NONE){
      
      /*check exit command and see if program needs to be terminated*/
      if(strcmp(t->argv[0], "exit") == 0){
         exit(0);
         /*check to see the cd command*/
      } else if(strcmp(t->argv[0], "cd") == 0){
         /*check to see if there is a second argument*/
         if(t->argv[1] == NULL){
            arg = chdir(getenv("HOME"));
         }
         else{
            arg = chdir(t->argv[1]);
         }
         /*check to see if chdir worked*/
         if(arg == -1){
            perror(t->argv[1]);
            return 1;
         } else {
            /*return 0 if successful*/
            return 0;
         }

      } else {
         /*fork and execute the command*/
         pid = fork();

         /*check for forking error*/
         if(pid == -1){
            perror("fork error");
         }

         /*check to see if parent process*/
         if(pid > 0){
            wait(&status);
            /*If statment that checks if WIFEXTED and WEXITSTATUS of child process
            * is valid*/
          if (WIFEXITED(status) != 0 && WEXITSTATUS(status) == 0) {
            /*return zero*/
             return 0;
         } else {
             /*return one*/
            return 1;
         }

         } else {
            /*check to see if input is different than NULL*/
            if(t->input != NULL){
               /*open file and check to see if open worked*/
               input = open(t->input, O_RDONLY);
               if(input == -1){
                  perror("open error");
                  exit(EX_OSERR);
               }
            
               /*redirect input*/
               dup_result = dup2(input, STDIN_FILENO);
               if(dup_result == -1){
                  perror("dup2 error");
                  exit(EX_OSERR);
               }
               /*close input*/
               close(input);
            }
            
            /*check to see if output is different than NULL*/
            if(t->output != NULL){
               /*open output file and check to see if worked*/
               output = open(t->output, OPEN_FLAGS, DEF_MODE);
               if(output == -1){
                  perror("output open error");
                  exit(EX_OSERR);
               }
            
               /*redirect output*/
               dup_result = dup2(output, STDOUT_FILENO);
               if(dup_result == -1){
                  perror("dup2 error");
                  exit(EX_OSERR);
               }
               close(output);
            }
            
            /*execute the command*/
            arg = execvp(t->argv[0], t->argv);
            /*check if arg is less than 0*/
            if(arg < 0){
               fprintf(stderr, "Failed to execute %s\n", t->argv[0]);
               fflush(stdout);
               exit(EX_OSERR);
            }

         }
      }
   } else if (t->conjunction == AND){
      /*recursivly call the method on the left node*/
      recursive_ret = process_tree(t->left, input, output);

      /*check to see if the left node returned 0*/
      if(recursive_ret == 0){
         /*recursivly call the method on the right node*/
         return recursive_ret = process_tree(t->right, input, output);
      } else {
         /*return 1 otherwise*/
         return 1;
      }

   } else if (t->conjunction == PIPE){

      /*check file redirection for ambigous output in left node*/
      if(t->left->output != NULL){
         /*print error message, flush, and return 0*/
         printf("Ambiguous output redirect.\n");
         fflush(stdout);
         return 0;
      }

      /*check file redirection for ambigous input in right node*/
      if(t->right->input != NULL){
         /*print error message, flush, and return 0*/
         printf("Ambiguous input redirect.\n");
         fflush(stdout);
         return 0;
      }

      /*initialize pipe*/
      pipe_stat = pipe(pipefd);
      /*check to see if pipe worked*/
      if(pipe_stat == -1){
         /*pipe did not work print an error*/
         perror("pipe error");
         exit(EX_OSERR);
      }

      /*fork and create child process*/
      pid = fork();
      /*check to see if fork worked*/
      if(pid == -1){
         /*fork did not work print an error*/
         perror("fork error");
         exit(EX_OSERR);
      }

      if(pid > 0){
         /*parent process*/
         /*close the write end of the pipe*/
         close(pipefd[1]);
         /*check if output is different than NULL*/
         if(t->output != NULL){
            output = open(t->output, OPEN_FLAGS, DEF_MODE);
            /*check to see if open worked*/
            if(output == -1){
               /*open did not work print an error*/
               perror("output open error");
               exit(EX_OSERR);
            }
            /*close output file*/
            close(output);
         }

         /*redirect input*/
         dup_result = dup2(pipefd[0], STDIN_FILENO);
         if(dup_result == -1){
            /*dup2 did not work print an error*/
            perror("dup2 error");
            exit(EX_OSERR);
         }

         /*call method on right node*/
         process_tree(t->right, pipefd[0], output);
         /*close read end of pipe*/
         close(pipefd[0]);
         /*wait for child process to finish*/
         wait(&status);

      } else {
         /*child process*/
         /*close the read end of the pipe*/
         close(pipefd[0]);
         
         /*check if input is null*/
         if(t->input != NULL){
            /*open input file and check to see if open worked*/
            input = open(t->input, O_RDONLY);
            if(input == -1){
               /*open did not work print an error*/
               perror("open error");
               exit(EX_OSERR);
            }
            /*close input file*/
            close(input);
         }

         /*redirect output*/
         dup_result = dup2(pipefd[1], STDOUT_FILENO);
         if(dup_result == -1){
            /*dup2 did not work print an error*/
            perror("dup2 error");
            exit(EX_OSERR);
         }
         /*call method on left node*/
         process_tree(t->left, input, output);

         /*close write end of pipe*/
         close(pipefd[1]);
         /*exit child process*/
         exit(0);
      }
      /*check to see if it is a subshell*/
   } else if (t->conjunction == SUBSHELL){
      /*fork and create child process*/
      pid = fork();
      /*check to see if fork worked*/
      if(pid == -1){
         /*fork did not work print an error*/
         perror("fork error");
         exit(EX_OSERR);
      }

      if(pid > 0){
         /*inside parent process*/
         /*wait for child process to finish*/
         wait(&status);
         /*check to see if child process exited normally*/
         if(WIFEXITED(status) != 0 && WEXITSTATUS(status) == 0){
            /*return 0*/
            return 0;
         }else{
            /*return 1*/
            return 1;
         }
      } else{
         /*inside child process*/
         /*check to see if input is different than NULL*/
         if(t->input != NULL){
            /*open input file and check to see if open worked*/
            input = open(t->input, O_RDONLY);
            if(input == -1){
               /*open did not work print an error*/
               perror("open error");
               exit(EX_OSERR);
            }
            
            /*redirect input*/
            dup_result = dup2(input, STDIN_FILENO);
            if(dup_result == -1){
               /*dup2 did not work print an error*/
               perror("dup2 error");
               exit(EX_OSERR);
            }
            /*close input file*/
            close(input);
         }

         /*check to see if output is different than NULL*/
         if(t->output != NULL){
            /*open output file and check to see if open worked*/
            output = open(t->output, OPEN_FLAGS, DEF_MODE);
            if(output == -1){
               /*open did not work print an error*/
               perror("output open error");
               exit(EX_OSERR);
            }
            /*redirect output*/
            dup_result = dup2(output, STDOUT_FILENO);
            if(dup_result == -1){
               /*dup2 did not work print an error*/
               perror("dup2 error");
               exit(EX_OSERR);
            }
            /*close output file*/
            close(output);
         }

         /*call method on left node*/
         process_tree(t->left, input, output);
      }
   }
   return 1;
}

static void print_tree(struct tree *t) {
   if (t != NULL) {
      print_tree(t->left);

      if (t->conjunction == NONE) {
         printf("NONE: %s, ", t->argv[0]);
      } else {
         printf("%s, ", conj[t->conjunction]);
      }
      printf("IR: %s, ", t->input);
      printf("OR: %s\n", t->output);

      print_tree(t->right);
   }
}

