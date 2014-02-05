#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "mond.h"

int sflag;
int active;
pid_t pid;
StatConfig *stats;
time_t rawtime;
pthread_t threads[THREAD_TOTAL];
pthread_mutex_t lock;
int tCount;
int mCount;
char *exe[2];

void deactivate(int num) {
   time(&rawtime);
   (stats + num)->active = -1;
   strcpy((stats + num)->stop, ctime(&rawtime));
   active--;
}
/*
  This function contains a large critical section largely due to reading and
  writing to files.
  This function can possibly share the same logfile as another monitor thread.
  Each line of code is either reading or writing to a file.
  We used a mutex lock because we only want one thread to access it at a time.
*/
void *sysStat() {
   char *curtime, temp[THOUSAND];
   int len, i, val, cur = 0;
   float fl;
   
   tCount++;
      
   while (1) {
      time(&rawtime);
      curtime = ctime(&rawtime);
      len = strlen(curtime);
      cur = 0;
   
      pthread_mutex_lock(&lock);
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      FILE *f = fopen((stats + sflag)->output, "a");
      system("egrep 'cpu |intr|ctxt|proc*' /proc/stat >> temp");
      system("egrep 'Mem|*Cach|*ctive:' /proc/meminfo >> temp");
      system("cat /proc/loadavg >> temp");
      system("grep 'sda ' /proc/diskstats >> temp");
      FILE *t = fopen("temp", "r");
   
      fprintf(f, "[");
      for (i = 0; i < len - 1; i++) {
         fprintf(f, "%c", curtime[i]);
      }
      fprintf(f, "] System  [PROCESS] ");
   
      fscanf(t, "%s %d %d", temp, &val, &i);
      fprintf(f, "%s %d ", word.sys[cur++], val);
   
      for (i = 0; i < 5; i++) {
         fscanf(t, "%d", &val);
         fprintf(f, "%s %d ", word.sys[cur++], val);
      }
      fgets(temp, THOUSAND, t);
   
      fscanf(t, "%s %d", temp, &val);
      fprintf(f, "%s %d ", word.sys[cur++], val);
      fgets(temp, THOUSAND, t);
   
      for (i = 0; i < 4; i++) {
         fscanf(t, "%s %d", temp, &val);
         fprintf(f, "%s %d ", word.sys[cur++], val);
      }
   
      fprintf(f, "[MEMORY] ");
      for (i = 0; i < 6; i++) {
         fscanf(t, "%s %d %s", temp, &val, temp);
         fprintf(f, "%s %d ", word.sys[cur++], val);
      }
   
      fprintf(f, "[LOADAVG] ");
      for (i = 0; i < 3; i++) {
         fscanf(t, "%f", &fl);
         fprintf(f, "%s %.06f ", word.sys[cur++], fl);
      }
      fgets(temp, THOUSAND, t);
   
      fprintf(f, "[DISKSTATS(sda)] ");
      fscanf(t, "%d %d %s %d", &i, &i, temp, &val);
      fprintf(f, "%s %d ", word.sys[cur++], val);
      fscanf(t, "%d %d", &i, &val);
      fprintf(f, "%s %d ", word.sys[cur++], val);
      fscanf(t, "%d", &val);
      fprintf(f, "%s %d ", word.sys[cur++], val);
      fscanf(t, "%d", &val);
      fprintf(f, "%s %d ", word.sys[cur++], val);
      fscanf(t, "%d %d", &i, &val);
      fprintf(f, "%s %d ", word.sys[cur++], val);
      fscanf(t, "%d", &val);
      fprintf(f, "%s %d\n", word.sys[cur], val);
   
      fclose(f);
      fclose(t);
   
      system("rm temp");
      pthread_mutex_unlock(&lock);
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   }
}

/*
  This function contains a large critical section largely due to reading and
  writing to files.
  This function can possibly share the same logfile as another system thread.
  Each line of code is either reading or writing to a file.
  We used a mutex lock because we only want one thread to access it at a time.
*/
void *monStat() {
   int val, i, len, cur = 0, status = 0, num = tCount++;
   char temp[THOUSAND], c, *curtime, path[SIZE];
   struct stat sb;
   
   mCount++;
   while (1) {
      pthread_mutex_lock(&lock);
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      sprintf(temp, "cat /proc/%d/stat >> temp2", (stats + num)->pid);
      system(temp);
      sprintf(temp, "cat /proc/%d/statm >> temp2", (stats + num)->pid);
      system(temp);
   
      time(&rawtime);
      curtime = ctime(&rawtime);
      len = strlen(curtime);
      cur = 0;
   
      FILE *f = fopen((stats + num)->output, "a");
      FILE *t = fopen("temp2", "r");
   
      fprintf(f, "[");
      for (i = 0; i < len - 1; i++) {
         fprintf(f, "%c", curtime[i]);
      }
   
      fprintf(f, "] Process(%d)  [STAT] ", (stats + num)->pid);
      fscanf(t, "%d %s %c %d %d %d %d %d", &i, temp, &c, &i, &i, &i, &i, &i);
      fprintf(f, "%s %s ", word.job[cur++], temp);
      fprintf(f, "%s %c ", word.job[cur++], c);
      for (i = 0; i < 3; i++) {
         fscanf(t, "%d %d", &len, &val);
         fprintf(f, "%s %d ", word.job[cur++], val);
      }
      fscanf(t, "%d %d %d", &val, &i, &i);
      fprintf(f, "%s %d ", word.job[cur++], val);
      for (i = 0; i < 3; i++) {
         fscanf(t, "%d", &val);
         fprintf(f, "%s %d ",word.job[cur++], val);
      }
      fscanf(t, "%d %d %d", &i, &i, &val);
      fprintf(f, "%s %d ", word.job[cur++], val);
      fscanf(t, "%d", &val);
      fprintf(f, "%s %d ", word.job[cur++], val);
      fgets(temp, THOUSAND, t);
   
      fprintf(f, "[STATM] ");
      for (i = 0; i < 4; i++) {
         fscanf(t, "%d", &val);
         fprintf(f, "%s %d ", word.job[cur++], val);
      }
      fscanf(t, "%d %d", &i, &val);
      fprintf(f, "%s %d\n", word.job[cur], val);
   
      fclose(f);
      fclose(t);
      
      system("rm temp2");
      pthread_mutex_unlock(&lock);
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
      usleep((stats + num)->interval);
      
      int x;
      
      if ((stats + num)->mode == EXE) {
         x = waitpid((stats + num)->pid, &status, WNOHANG || WUNTRACED);
         if (x == (stats + num)->pid) {
            deactivate(num);
            pthread_exit(NULL);
         }
      }
      if ((stats + num)->mode == PID) {
         sprintf(path, "/proc/%d/stat", (stats + num)->pid);
         if (stat(path, &sb) == -1) {
            deactivate(num);
            pthread_exit(NULL);
         }
      }
   }
}

int errorHandler(char *err){
   char c = err[0];
   
   switch (c) {
      case 'c':
         printf("ERROR: Invalid command.\n");
         if (sflag == tCount) {
            sflag = -1;
         }
         break;
      case 'i':
         printf("ERROR: Default interval is not set.\n");
         if (sflag == tCount) {
            sflag = -1;
         }
         break;
      case 'l':
         printf("ERROR: Default logfile is not set.\n");
         if (sflag == tCount) {
            sflag = -1;
         }
         break;
      case 's':
         printf("ERROR: A system thread has already been created.\n");
         break;
      case 't':
         printf("ERROR: Cannot create more than 10 command threads.\n");
         break;
      case 'p':
         printf("ERROR: PID does not exist.\n");
         break;
      default:
         printf("ERROR: Invalid command.\n");
         break;
   }
   
   return 1;
}

void listActive() {
   int i, j, len;
   
   printf("%8s %9s \t%24s %8s %10s\n", "ThreadID", "ProcessID", "Start Time", "Interval", "Logfile");
   printf("%8s %9s \t%24s %8s %10s\n", "N/A", "command", "N/A", "N/A", "N/A");
   
   for (i = 0; i < THREAD_TOTAL; i++) {
      if ((stats + i)->active > 0) {
         printf("%8d", i);
         if ((stats + i)->mode == SYS) {
            printf(" %9s \t", "system");
         } else {
            printf(" %9d \t", (stats + i)->pid);
         }
         len = strlen((stats + i)->start);
          for (j = 0; j < TIME; j++) {
             printf("%c", (stats + i)->start[j]);
          }
         printf(" %8d %10s\n", (stats + i)->interval, (stats + i)->output);
      }
   }
}

void listCompleted() {
   int i, j, len;
   
   printf("%8s %9s \t%24s\t%24s %8s %10s\n", "ThreadID", "ProcessID", "Start Time", "End Time", "Interval", "Logfile");
   
   for (i = 0; i < THREAD_TOTAL; i++) {
      if ((stats + i)->active < 0) {
         printf("%8d", i);
         if ((stats + i)->mode == SYS) {
            printf(" %9s \t", "system");
         } else {
            printf(" %9d \t", (stats + i)->pid);
         }
         len = strlen((stats + i)->start);
          for (j = 0; j < TIME; j++) {
             printf("%c", (stats + i)->start[j]);
          }
          printf("\t");
          for (j = 0; j < TIME; j++) {
             printf("%c", (stats + i)->stop[j]);
          }
         printf(" %8d %10s\n", (stats + i)->interval, (stats + i)->output);
      }
   }
}

int main(int argc, char *argv[]) {
   int deInterval = 0, error, i, c;
   char command[THOUSAND], input[SIZE], deOutput[SIZE], path[SIZE];
   char temp[SIZE], *token;
   struct stat sb;
   
   tCount = 0;
   mCount = 0;
   sflag = -1;
   active = 0;
   stats = calloc(sizeof(StatConfig), THREAD_TOTAL + 1);
   deOutput[0] = '\0';
   pthread_mutex_init(&lock, NULL);
   
   while (1) {
      error = 0;
      printf(">> ");
      fgets(command, THOUSAND, stdin);
      token = strtok(command, " \n");
      if (!token) {
         error = errorHandler("command");
      } else if (!strcmp(token, "add")) {
         token = strtok(NULL, " \n");
         if (!token) {
            error = errorHandler("command");
         } else if (!strcmp(token, "-s")) {
            if (sflag >= 0) {
               error = errorHandler("system");
            } else {
               sflag = tCount;
               (stats + tCount)->mode = SYS;
            }
         } else if (!strcmp(token, "-p")) {
            token = strtok(NULL, " \n");
            if (!token) {
               error = errorHandler("command");
            } else {
               (stats + tCount)->pid = atoi(token);
               (stats + tCount)->mode = PID;
            }
         } else if (!strcmp(token, "-e")) {
            token = strtok(NULL, " \n");
            if (!token) {
               error = errorHandler("command");
            } else {
               sprintf(temp, "%s", token);
               exe[0] = temp;
               exe[1] = 0;
               (stats + tCount)->mode = EXE;
            }
         } else {
            error = errorHandler("command");
         }
         
         if (!error) {
            token = strtok(NULL, " \n");
            if (token) {
               if (!strcmp(token, "-i")) {
                  token = strtok(NULL, " \n");
                  if (!token) {
                     error = errorHandler("command");
                  } else {
                     (stats + tCount)->interval = atoi(token);
                  }
               } else if (!strcmp(token, "-f")) {
                  token = strtok(NULL, " \n");
                  if (!token) {
                     error = errorHandler("command");
                  } else {
                     sprintf((stats + tCount)->output, "%s", token);
                     if (strtok(NULL, " \n")) {
                        error = errorHandler("command");
                     }
                  }
               }  else {
                  error = errorHandler("command");
               }
            }
         }
         
         if (!error) {
            token = strtok(NULL, " \n");  
            if (token) {
               if (!strcmp(token, "-f")) {
                  token = strtok(NULL, " \n");
                  if (!token) {
                     error = errorHandler("command");
                  } else {
                     sprintf((stats + tCount)->output, "%s", token);
                     if (strtok(NULL, " \n")) {
                        error = errorHandler("command");
                     }
                  }
               } else {
                  error = errorHandler("command");
               }
            }
         }
         
         if (!error) {
            if (!(stats + tCount)->interval && !deInterval) {
               error = errorHandler("interval");
            } else if (!(stats + tCount)->interval) {
               (stats + tCount)->interval = deInterval;
            }

            if (!strlen((stats + tCount)->output) && !strlen(deOutput)) {
               error = errorHandler("logfile");
            } else if (!strlen((stats + tCount)->output)) {
               strcpy((stats + tCount)->output, deOutput);
            }
         
            if ((stats + tCount)->mode == PID) {
               sprintf(path, "/proc/%d/stat", (stats + tCount)->pid);
               if (stat(path, &sb) == -1) {
                  error = errorHandler("pID");
               }
            }
         
            if (mCount > THREAD_TOTAL - 2 && (stats + tCount)->mode != SYS) {
               error = errorHandler("threads");
            }
         
            if (!error) {
               time(&rawtime);
               strcpy((stats + tCount)->start, ctime(&rawtime));
               (stats + tCount)->active = 1;
               if ((stats + tCount)->mode == SYS) {
                  pthread_create(threads + tCount, NULL, sysStat, NULL);
               } else {
                  if ((stats + tCount)->mode == EXE) {
                     pid = fork();
                     if (!pid) {
                        execvp(exe[0], exe);
                        perror("execvp() failed\n");
                     }
                     (stats + tCount)->pid = pid;
                  }
               pthread_create(threads + tCount, NULL, monStat, NULL);
               }
               active++;
            } else {
               (stats + tCount)->interval = 0;
               strcpy((stats + tCount)->output, "\0");
            }
         }
         
      } else if (!strcmp(token, "set")) {
         token = strtok(NULL, " \n");
         if (!token) {
            errorHandler("command");
         } else if (!strcmp(token, "interval")) {
            token = strtok(NULL, " \n");
            if (!token) {
               errorHandler("command");
            } else {
               deInterval = atoi(token);
               if (strtok(NULL, " \n")) {
                  errorHandler("command");
                  deInterval = 0;
               }
            }
         } else if (!strcmp(token, "logfile")) {
            token = strtok(NULL, " \n");
            if (!token) {
               errorHandler("command");
            } else {
               sprintf(deOutput, "%s", token);
               if (strtok(NULL, " \n")) {
                  errorHandler("command");
                  deOutput[0] = '\0';
               }
            }
         } else {
            errorHandler("command");
         }
      } else if (!strcmp(token, "listactive")) {
         token = strtok(NULL, " \n");
         if (token) {
            errorHandler("command");
         } else {
            listActive();
         }
      } else if (!strcmp(token, "listcompleted")) {
         token = strtok(NULL, " \n");
         if (token) {
            errorHandler("command");
         } else {
            listCompleted();
         }
      } else if (!strcmp(token, "remove")) {
         token = strtok(NULL, " \n");
         if (!token) {
            errorHandler("command");
         } else if (!strcmp(token, "-s")) {
            if (strtok(NULL, " \n")) {
               errorHandler("command");
            } else {
               if ((stats + sflag)->active) {
                  pthread_cancel(threads[sflag]);
                  deactivate(sflag);
               }
            }
         } else if (!strcmp(token, "-t")) {
            token = strtok(NULL, " \n");
            if (!token) {
               errorHandler("command");
            } else {
               c = atoi(token);
               if (strtok(NULL, " \n")) {
                  errorHandler("command");
               } else {
                  if (c < THREAD_TOTAL - 1) {
                     if ((stats + c)->active) {
                        pthread_cancel(threads[c]);
                        deactivate(c);
                     }
                  }
               }
            }
         } else {
            errorHandler("command");
         }
      } else if (!strcmp(token, "kill")) {
         token = strtok(NULL, " \n");
         if (!token) {
            errorHandler("command");
         } else {
            c = atoi(token);
            if (strtok(NULL, " \n")) {
               errorHandler("command");
            } else {
               for (i = 0; i < THREAD_TOTAL; i++) {
                  if ((stats + i)->active && c == (stats + i)->pid) {
                     if ((stats + i)->mode == EXE) {
                        kill((stats + i)->pid, SIGKILL);
                     }
                     pthread_cancel(threads[i]);
                     deactivate(i);
                  }
               }
            }
         }
      } else if (!strcmp(token, "exit")) {
         if (strtok(NULL, " \n")) {
            errorHandler("command");
         } else {
            input[0] = 'y';
            if (active) {
               printf("You still have threads actively monitoring. ");
               printf("Do you really want to exit? (y/n)\n");
               fgets(input, THOUSAND, stdin);
            }
            if (input[0] == 'y') {
               for (i = 0; i < THREAD_TOTAL; i++) {
                  if ((stats + i)->active) {
                     if ((stats + i)->mode == EXE) {
                        kill((stats + i)->pid, SIGKILL);
                     }
                     pthread_cancel(threads[i]);
                  }
               }
               free(stats);
               exit(EXIT_SUCCESS);
            }
         }
      } else {
         errorHandler("command");
      }
   }
   
   return 0;
}
