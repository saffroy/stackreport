# stackreport

Report stack usage for each thread in a program.

# Build

```
$ make 
gcc -Wall -O2 -g -D_GNU_SOURCE -fPIC -shared -pthread -o stackreport.so stackreport.c -ldl
```

# Usage

Use `LD_PRELOAD` to override `pthread_create` at program start. A
report is written when the program terminates, either to `stderr`
(default) or to a file (if `SR_OUTPUT_FILE` is set):

```
$ SR_OUTPUT_FILE=report.txt LD_PRELOAD=./stackreport.so ./testprog $((4096*40)) $((4096*80)) $((4096*20)) 
```
```
$ cat report.txt 
tid 6152 name thread-2 stack_addr 0x7f29ef8e7000 stack_lim 0x7f29f00e8000 guard_size 4096 stack_used 94208
tid 6151 name thread-1 stack_addr 0x7f29f00e8000 stack_lim 0x7f29f08e9000 guard_size 4096 stack_used 339968
tid 6150 name testprog stack_addr 0x7ffec7e92000 stack_lim 0x7ffec868f000 guard_size 0 stack_used 172032
```
