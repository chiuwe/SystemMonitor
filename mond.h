#define THOUSAND 1000
#define SIZE 27
#define THREAD_TOTAL 11
#define TIME 24
#define SYS 1
#define PID 2
#define EXE 3

#ifndef STR
#define STR

struct str {
   char sys[SIZE][SIZE];
   char job[SIZE][SIZE];
};

struct str word = {{"cpuusermode",
                    "cpusystemmode",
                    "idletaskrunning",
                    "iowaittime",
                    "irqservicetime",
                    "softirqservicetime",
                    "intr",
                    "ctxt",
                    "forks",
                    "runnable",
                    "blocked",
                    "memtotal",
                    "memfree",
                    "cached",
                    "swapcached",
                    "active",
                    "inactive",
                    "1min",
                    "5min",
                    "15min",
                    "totalnoreads",
                    "totalsectorsread",
                    "nomsread",
                    "totalnowrites",
                    "nosectorswritten",
                    "nomswritten"},
                    {"executable",
                    "stat",
                    "minorfaults",
                    "majorfaults",
                    "usermodetime",
                    "kernelmodetime",
                    "priority",
                    "nice",
                    "nothreads",
                    "vsize",
                    "rss",
                    "program",
                    "residentset",
                    "share",
                    "text",
                    "data"}};

#endif

#ifndef MONITOR_H
#define MONITOR_H

#include <unistd.h>

typedef struct {
   pid_t pid;
   int interval;
   char output[SIZE];
   char start[SIZE];
   char stop[SIZE];
   int active;
   int mode;
} StatConfig;

#endif