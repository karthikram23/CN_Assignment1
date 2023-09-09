#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define BufferSize 10000
#define MAX_IP_LENGTH 16
char *IP;


int packetsCaptured = 0;
int distinctflows = 0;

//arrary to map port to pid
int portToPid[65536];


struct Flow {
    char source_ip[INET_ADDRSTRLEN];      // Character array to store source IP
    char destination_ip[INET_ADDRSTRLEN]; // Character array to store destination IP
    int source_port;
    int destination_port;
    char tcp_check_sum[10];
    unsigned char data[2000];
    int length;
    int pid;
};


struct Flow BufferFlow[16000];
int getipaddress() {
    
    char buffer[256];
    char* token;

  
    FILE* fp = popen("ifconfig", "r");
    if (fp == NULL) {
        perror("Failed to run ifconfig");
        exit(1);
    };

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if ((token = strstr(buffer, "inet ")) != NULL) {
            IP = strtok(token + 5, " "); // Extract the IP address
            break;
        }
    }

   
    if (IP != NULL) {
        printf("IP Address is %s\n", IP);
    } else {
        printf("ip address not found .\n");
        return -1;
    };

    // Close the command output
    pclose(fp);

    return 0;
};


int getpidofport(int port) {
    char buffer[128];  // Buffer to store the output
    FILE *fp;
    char command[128];

    // Create the command string with the specified port
    snprintf(command, sizeof(command), "sudo netstat -p | grep %d | grep -oP '\\d+' | tail -n 1", port);

    // Run the command and open its output for reading
    fp = popen(command, "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            int pid = atoi(buffer);
            pclose(fp);
            printf("PID for port %d: %d\n", port, pid);
            return pid;
        } else {
            // No output captured or an error occurred
            pclose(fp);
            printf("Error: No PID found for port %d\n", port);
            return -1;
        }
    } else {
        perror("popen");
        return -1;
    }
}




void process_packet(unsigned char* buffer, int size) {
    printf("Received %d bytes\n", size);

    struct iphdr* iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));

    struct in_addr src_ip;
    src_ip.s_addr = iph->saddr;

    struct in_addr dest_ip;
    dest_ip.s_addr = iph->daddr;

    /* Getting the actual size of the IP header */
    int iphdrlen = iph->ihl * 4;
    
    


    if (iph->protocol != 6){
        packetsCaptured--;
        return;
    };


    struct tcphdr* tcph = (struct tcphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));

    // Extract the tcp check sum
    sprintf(BufferFlow[packetsCaptured - 1].tcp_check_sum, "%d", tcph->check);
    int tcp_size = size - sizeof(struct ethhdr) - sizeof(struct iphdr)-sizeof(struct tcphdr);
    BufferFlow[packetsCaptured-1].length = tcp_size;
    unsigned char * tcp_packet = buffer + sizeof(struct ethhdr) + sizeof(struct iphdr) +sizeof(struct tcphdr);
        
    for(int i = 0; i < tcp_size; i++){
        BufferFlow[packetsCaptured - 1].data[i] = (unsigned char )tcp_packet[i];
    };
       
   //code to get the pid of the packet and store it in the array

    int port = ntohs(tcph->dest);
    if (strcmp(inet_ntoa(src_ip),IP) == 0){
        port = ntohs(tcph->source);
    };
    if (strcmp(inet_ntoa(dest_ip),IP) == 0){
        port = ntohs(tcph->dest);
    };
    //printf("port : %d\n",port);
    portToPid[port] =    getpidofport(port);
    BufferFlow[packetsCaptured-1].pid = portToPid[port];

   //code to store the packet in the buffer


    strcpy(BufferFlow[packetsCaptured - 1].destination_ip, inet_ntoa(dest_ip));
    strcpy(BufferFlow[packetsCaptured - 1].source_ip, inet_ntoa(src_ip));
    BufferFlow[packetsCaptured - 1].source_port = ntohs(tcph->source);
    BufferFlow[packetsCaptured - 1].destination_port = ntohs(tcph->dest);
    
 // printing sone necessary informationof captured packet
    printf("********************************************\n");
    printf("Source IP: %s\n", inet_ntoa(src_ip));
    printf("Destination IP: %s\n", inet_ntoa(dest_ip));
    printf("Source Port: %d\n", ntohs(tcph->source));
    printf("Destination Port: %d\n", ntohs(tcph->dest));
    printf("PID: %d \n",portToPid[port]);
};


void signalhandler() {
    //printf("No of distinctflows : %d \n", distinctflows);
    printf("No of packetsCaptured : %d \n", packetsCaptured);

    // Create a file to store packet information for each combination
    printf("storing flows in a file");
    FILE* fd_combinations = fopen("flows.txt", "w");
    if (fd_combinations == NULL) {
        perror("File open error");
        return;
    };

    // Initialize an array to keep track of processed packets
    int processedPackets[packetsCaptured];
    memset(processedPackets, 0, sizeof(processedPackets));

    // Iterate through each combination of src_ip, src_port, dest_ip, dest_port
    for (int i = 0; i < packetsCaptured; i++) {
        if (processedPackets[i]) {
            continue;
        };
        char src_ip[INET_ADDRSTRLEN];
        char dest_ip[INET_ADDRSTRLEN];
        int src_port = BufferFlow[i].source_port;
        int dest_port = BufferFlow[i].destination_port;

        strcpy(src_ip, BufferFlow[i].source_ip);
        strcpy(dest_ip, BufferFlow[i].destination_ip);

        fprintf(fd_combinations, "Combination: Source IP: %s  Source Port: %d  Destination IP: %s  Destination Port: %d \n",
                src_ip, src_port, dest_ip, dest_port);
        fprintf(fd_combinations,"Pid : %d \n",BufferFlow[i].pid);
        printf("Combination: Source IP: %s  Source Port: %d  Destination IP: %s  Destination Port: %d\n",
               src_ip, src_port, dest_ip, dest_port);

        fprintf(fd_combinations, "----------------------------------------\n");
        fprintf(fd_combinations, "Packet 0 - TCP Check Sum: %s\n", BufferFlow[i].tcp_check_sum);
        fprintf(fd_combinations,"Pid : %d \n",BufferFlow[i].pid);
        fprintf(fd_combinations, "        Data: ");
        for (int k = 0 ; k < BufferFlow[i].length; k++){
            fprintf(fd_combinations,"%c",BufferFlow[i].data[k]);
        };
        fprintf(fd_combinations,"\n");

    
        
        // Iterate through all captured packets
        int packetno = 1; 
        for (int j = i+1; j < packetsCaptured; j++) {
            if (strcmp(BufferFlow[j].source_ip, src_ip) == 0 &&
                BufferFlow[j].source_port == src_port &&
                strcmp(BufferFlow[j].destination_ip, dest_ip) == 0 &&
                BufferFlow[j].destination_port == dest_port &&
                !processedPackets[j]) {
                fprintf(fd_combinations, "Packet %d - TCP Check Sum: %s\n", packetno, BufferFlow[j].tcp_check_sum);
                 fprintf(fd_combinations, "        Data: ");
                    for (int k = 0 ; k < BufferFlow[i].length; k++){
                        fprintf(fd_combinations,"%c",BufferFlow[j].data[k]);
                    };
                fprintf(fd_combinations, "----------------------------------------\n");
                packetno++;
                // Mark this packet as processed for this combination
                processedPackets[j] = 1;
                fflush(fd_combinations);
            }
        }
    }

    fclose(fd_combinations);
    exit(0);
}


int main() {
    signal(SIGINT, signalhandler);
    int x  = getipaddress();
    if (x  == -1){
        printf("error in getting ipaddress");
        return 1;
    };

    memset(portToPid,-1,sizeof(portToPid));

    unsigned char* buffer = (unsigned char*)malloc(BufferSize);

    int sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_raw < 0) {
        perror("Socket Error");
        return 1;
    }

    struct sockaddr socketaddress;
    while (1) {
        time_t start_time = time(NULL);
        while (time(NULL)-start_time <= 10){
                int saddr_size = sizeof(socketaddress);
                int data_size = recvfrom(sock_raw, buffer, BufferSize, MSG_DONTWAIT , &socketaddress, (socklen_t*)&saddr_size);
                
                if (data_size > 0){
                     process_packet(buffer, data_size);
                     packetsCaptured += 1;
                 };
               
        }
        int port;
        printf("enter the port number\n");
        scanf("%d",&port);
        int pid  = portToPid[port];
        if (pid == -1){
            printf("no process is running on this port");
        }else{
            printf("pid of the process  on this port is %d \n",pid);
        };       
        sleep(10);
    };
    return 0;
}
