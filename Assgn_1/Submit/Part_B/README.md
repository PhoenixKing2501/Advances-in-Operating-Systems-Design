# Steps to run code and tests
* Go to folder Assgn_1/Part_B/lkm
* Run make
* insert module into kernel using ```insmod lkm_deq.ko```
* Go to folder Assgn_1/Part_B/test
* Run make to compile all tests at once
* Run individual tests executables
* Can check live output with 
```dmesg --follow -HP```
* To look at the deque of a process after every read and write, put ```#define DEBUG``` in lkm_deq.c and make