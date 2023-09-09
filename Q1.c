
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

int packetsCaptured = 0;



struct Flow {
    char source_ip[INET_ADDRSTRLEN];      // Character array to store source IP
    char destination_ip[INET_ADDRSTRLEN]; // Character array to store destination IP
    int source_port;
    int destination_port;
    char tcp_check_sum[10];
    unsigned char data[7000];
    int length;
};

struct Flow BufferFlow[16000];





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
    sprintf(BufferFlow[packetsCaptured - 1].tcp_check_sum, "%d", ntohs(tcph->check));
    int tcp_size = size - sizeof(struct ethhdr) - iphdrlen;
    BufferFlow[packetsCaptured-1].length = tcp_size;
    unsigned char * tcp_packet = buffer + sizeof(struct ethhdr) + iphdrlen;
        
    for(int i = 0; i < tcp_size; i++){
        BufferFlow[packetsCaptured - 1].data[i] = (unsigned char)tcp_packet[i];
    };
       

    strcpy(BufferFlow[packetsCaptured - 1].destination_ip, inet_ntoa(dest_ip));
    strcpy(BufferFlow[packetsCaptured - 1].source_ip, inet_ntoa(src_ip));
    BufferFlow[packetsCaptured - 1].source_port = ntohs(tcph->source);
    BufferFlow[packetsCaptured - 1].destination_port = ntohs(tcph->dest);
    

    
    printf("Source IP: %s\n", inet_ntoa(src_ip));
    printf("Destination IP: %s\n", inet_ntoa(dest_ip));
    printf("Source Port: %d\n", ntohs(tcph->source));
    printf("Destination Port: %d\n", ntohs(tcph->dest));
};


void signalhandler() {
    
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
    int noofflows = 0 ;
    // Iterate through each combination of src_ip, src_port, dest_ip, dest_port
    for (int i = 0; i < packetsCaptured; i++) {
        if (processedPackets[i]) {
            continue;
        };
        noofflows++;
        char src_ip[INET_ADDRSTRLEN];
        char dest_ip[INET_ADDRSTRLEN];
        int src_port = BufferFlow[i].source_port;
        int dest_port = BufferFlow[i].destination_port;

        strcpy(src_ip, BufferFlow[i].source_ip);
        strcpy(dest_ip, BufferFlow[i].destination_ip);
        fprintf(fd_combinations,"******************************************************************************************************\n");
       fprintf(fd_combinations, "Flow %d : ", noofflows);
        fprintf(fd_combinations, "Source IP: %s  ", src_ip);
        fprintf(fd_combinations, "Source Port: %d  ", src_port);
        fprintf(fd_combinations, "Destination IP: %s  ", dest_ip);
        fprintf(fd_combinations, "Destination Port: %d\n", dest_port);

        printf("Combination: ");
        printf("Source IP: %s  ", src_ip);
        printf("Source Port: %d  ", src_port);
        printf("Destination IP: %s  ", dest_ip);
        printf("Destination Port: %d\n", dest_port);

        fprintf(fd_combinations, "----------------------------------------\n");
        fprintf(fd_combinations, "Packet 0 - TCP Check Sum: %s\n", BufferFlow[i].tcp_check_sum);

        fprintf(fd_combinations, "        Data: ");
        for (int k = 0 ; k < BufferFlow[i].length; k++){
            fprintf(fd_combinations,"%c",BufferFlow[i].data[k]);
        };
        fprintf(fd_combinations,"\n");

    
        
    

        // Iterate through all captured packets
        int packetno = 1; 
        for (int j = i+1; j < packetsCaptured; j++) {
            if (processedPackets[j] == 0){
                if ((strcmp(BufferFlow[j].source_ip, src_ip) == 0 &&
                BufferFlow[j].source_port == src_port &&
                strcmp(BufferFlow[j].destination_ip, dest_ip) == 0 &&
                BufferFlow[j].destination_port == dest_port ) || 
                (strcmp(BufferFlow[j].source_ip, dest_ip) == 0 &&
                BufferFlow[j].source_port == dest_port &&
                strcmp(BufferFlow[j].destination_ip, src_ip) == 0 &&
                BufferFlow[j].destination_port == src_port 
               )) {
                fprintf(fd_combinations, "Packet %d - TCP Check Sum: %s\n", packetno, BufferFlow[j].tcp_check_sum);
                 fprintf(fd_combinations, "        Data: ");
                    for (int k = 0 ; k < BufferFlow[j].length; k++){
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
    }
    //printing no of flows captured
    printf("No of Flows : %d \n", noofflows);
    //closing the file 
    fclose(fd_combinations);
    exit(0);
}


int main() {
    signal(SIGINT, signalhandler);
    unsigned char* buffer = (unsigned char*)malloc(BufferSize);
    int sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_raw < 0) {
        perror("Socket Error");
        return 1;
    }

  

    struct sockaddr socketaddress;
    while (1) {
        int saddr_size = sizeof(socketaddress);
        int data_size = recvfrom(sock_raw, buffer, BufferSize, 0, &socketaddress, (socklen_t*)&saddr_size);
        packetsCaptured += 1;
        process_packet(buffer, data_size);
    }
    return 0;
}
