# CN_Assignment1

## Group Members

- Gali Sunny - 20110067
- Balu Karthik Ram - 20110036

## Files

There are two source code files for this assignment:

1. **Q1.c**: This program sniffs network packets and extracts necessary information from them.
2. **Q3.c**: The program sniffs packets and finds the PID of the client application linked to the port.

## Pcap Files

The following pcap files are used for the respective questions:

- **For Question 1 (Q1.c):** `0.pcap`
- **For Question 3 (Q3.c):** `3.pcap`

## Q1

**Brief on Q1.c:**
This program sniffs network packets and extracts the necessary information from them.

**Executing Q1:**
1. Compilation: `gcc Q1.c -o Q1`
2. Execution: `sudo ./Q1`
3. For packet replay: `tcpreplay eth0 -v <PathToPcapFile>`

**Output:**
The TCP flow information will be stored in `flow.txt`.
The required information for the Q2 will be stored in Q2.txt

## Q3

**Brief on Q3:**
The program sniffs packets and finds the PID of the client application linked to the port. After 30 seconds, it  will prompts the user to enter the port, and the program outputs the PID linking the port, the program will prompt the user until the program terminated by user.

**Executing Q3.c:**
1. Compilation: `gcc Q3.c -o Q3`
2. Running: `sudo ./Q3`

**Output:**
The TCP flow information, including the corresponding PIDs, will be stored in `flowdetails.txt`.
