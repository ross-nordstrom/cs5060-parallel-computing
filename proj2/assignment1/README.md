# Assignment-1 (30pts):
Use read-write locks to implement a producer-consumer algorithm. 

## Details
Assume one producer and multiple consumers. The producer reads characters one by one from 
a string stored in a file named “string.txt”, and then writes sequentially these characters into a 
circular queue. The queue has multiple slots for the stored string (e.g., 5 slots). Meanwhile, the
consumers read sequentially from the queue and print them in the same order. Use the code 
presented in page 37 in Lecture-6 to implement the read-write locks with mutexes and 
conditional variables.

# Usage
## Building
Run `./build.sh`

## Running
Run `./run.sh`, or run `./pc_lock.o <QueueSize> <NumConsumers>`
Expects a file `string.txt` as the input buffer.

## Testing
Run our python script to test out the program with a variety of string sizes, queue sizes, and consumers:
```bash
python test.py
```



# Implementation

## Queueing
The queue logic is simple: there is a single, shared buffer, with two accessor methods: `enqueue` and `dequeue`. They are "naive" in that they assume the caller is benign and ensures calls are only made at appropriate times. They use two pointers: one for the producer pointing to where the next enqueue'd item should go, and another for the consumer(s) pointing to where the next item should be dequeue'd from.

## Locking and Synchronization
Using a R/W lock allows all the consumers to read simultaneously. We also use a simple mutex around the consumer index, `idxConsumer`, to ensure only one consumer accesses and prints an item from the buffer at a time.

We modified the base R/W lock provided in the lectures slides to enforce state and synchronization specific to the Producer/Consumer problem. Some things we added are the number of items in the queue and smarts to prevent more consumers to access the queue than there are items.

To support synchronizing the consumers, we added a `pending_readers` count in the R/W lock. Additionally, we use a `done` flag (guarded by mutex locks within a getter and setter function) to communicate when the producer is done. If the producer is done and the queue is empty, the consumers can safely exit.
