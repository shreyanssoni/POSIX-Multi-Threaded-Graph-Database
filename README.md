# POSIX-Multi-Threaded-Graph-Database

Simulation of an application for a distributed graph database system involving a load balancer process, a primary server process, two secondary server processes (secondary server 1 and secondary server 2), a cleanup process and several clients. The overall system architecture is shown below.

![image](https://github.com/shreyanssoni/POSIX-Multi-Threaded-Graph-Database/assets/88244485/b6fda913-6c76-4978-9f33-d8d37fae98a7)

The overall workflow is as follows:
● Clients send requests (read or write) to the load balancer.
● Load balancer forwards the write requests to the primary server.
● Load balancer forwards the odd numbered read requests to secondary server 1 and the even 
numbered read requests to secondary server 2. Request numbering starts from 1.
● On receiving a request from the load balancer, the server (primary or secondary) creates a new thread 
to process the request. Once the processing is complete, this thread (not the server process) sends a 
message or an output back to the client.
● For terminating the application, a relevant input (explained later) needs to be given to the cleanup 
process. The cleanup process informs the load balancer. The load balancer performs the relevant 
cleanup activity, informs the servers to exit and terminates itself. On receiving the intimation from the 
load balancer, the servers perform the relevant cleanup activities and terminate
