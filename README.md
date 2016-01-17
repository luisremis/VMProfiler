# VMProfiler

Design and Implementation

We use the write function as the API for registration for application. This way, the user’s application can easily send an integer (4 bytes) with the process id. Inside the module, this integer is handled and a new element in the list is created. The user also send one letter indicating the action: Register or Deregister. 

We implemented the write function in a way that it returns -1 when there was an error with the process trying to do some action. In this way, when the process register itself, it can check whether it was successful or not. Of course, it can also read all the process using this module. 
This is tested using cat command. 

In the timer interruption we only do two things: 

1. Enqueue the work into the work queue
2. Re-shoot the timer 50ms later, if there is at least one task in the list.

This way, the routine is short and exhibits good performance. For setting up the timer, we used the method that converts milliseconds to jiffies. This way we can be sure that we are setting the timer with the correct interval without worrying about jiffies conversions. Since this method is implemented within the kernel, we are sure its performance is good. 

We used the vmalloc for the buffer shared with the monitor application. We also used some macros to implement the finite states machine of our module. 

We finally made a check for each allocation to have its proper deallocation at the end, in order to make sure that no memory leaks are present. 


How to run our program

To test the kernel module, just run:
	
sh test.sh

This script file will:

1. clear the DMESG (to make it easy to read after the test)
2. clean and recompile both the module and the user app
3. load the module
4. run some user applications
5. print using cat
6. remove the module
7. print DMESG to see the internal checkpoints of the module

The module was tested this way. 

We also created sh scripts to obtain all the data for the plot and analysis. 

For Case of Study 1, we need to run profile1.sh and profile2.sh
For Case of Study 2, we need to run profileSeveral.sh
Each script will generate the output files that will be read by a matlab script to do the plotting, as can be seen in the next page. 


Analysis of the results

Case of Study 1:

We can see a big difference in the number of page faults for the first experiment (P1 and P2) and the second experiment (P3 and P4). The first experiment exhibits a bigger number of page faults, both minor and major, than the second experiment. This is because the first experiment has both processes that do a random access to memory. This incurs in higher chances of generating a page fault. One of the process of experiment 2 has a smaller amount of allocated memory and a sequential access, and both characteristics will result in less page faults, because in this case the process is very likely to access the same page most of the time. 

Also, as a result of more page faults, experiment 1 takes longer time to complete. A page fault introduce a huge delay in the execution since it involves i/o operations which are orders of magnitude slower than the processor. 

It can also be seen that there is a break in the number of page faults in both experiments. This is due to the completion of one of the process involved in the experiment. 


Case of Study 2

For this case, we can see how the CPU utilization increases when adding more process to the system, until a point where there are so many processes accessing memory and generating page faults (because the lack of physical memory in the system), that the utilization decrease since most of the time, the process is waiting for i/o. If we add more memory to the system, the degree of multiprogramming where the system start to do trashing will increase, but there is some point where the same effect will take place.


