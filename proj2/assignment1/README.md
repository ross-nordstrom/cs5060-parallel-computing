# Assignment-1 (30pts):
Use read-write locks to implement a producer-consumer algorithm. 

## Details
Assume one producer and multiple consumers. The producer reads characters one by one from 
a string stored in a file named “string.txt”, and then writes sequentially these characters into a 
circular queue. The queue has multiple slots for the stored string (e.g., 5 slots). Meanwhile, the
consumers read sequentially from the queue and print them in the same order. Use the code 
presented in page 37 in Lecture-6 to implement the read-write locks with mutexes and 
conditional variables.


