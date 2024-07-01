//Pawel Richert 324903
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fstream> 
#include <iostream>
#include <netdb.h>
#include <cmath>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h> 
#include <chrono>
#include <sstream>
#include <vector>
using namespace std;
#define IP_MAXPACKET 1100
#define MAX_NUMBER_OF_DATAGRAMS 1000

#define ARRAY_SIZE 100
#define TIME_OUT 10
vector<string> paraFile;
bool isValidPortNumber(const std::string& portStr) {
    for (char c : portStr) {
        if (!std::isdigit(c)) {
            return false;
        }
    }

    int portNumber = std::atoi(portStr.c_str());
    return portNumber > 0 && portNumber <= 65535;
}

bool isValidSize(const std::string& sizeStr) {
    for (char c : sizeStr) {
        if (!std::isdigit(c)) {
            return false;
        }
    }

    int size = std::atoi(sizeStr.c_str());
    return size >= 0;
}

bool validateArguments(int argc, char const *argv[]) {
    if (argc != 5) {
        std::cerr << "Niepoprawna liczba argumentów." << std::endl;
        return false;
    }
    std::string fileName = argv[3];
    if (fileName.empty()) {
        std::cerr << "Pusta nazwa pliku." << std::endl;
        return false;
    }
    std::string portStr = argv[2];
    if (!isValidPortNumber(portStr)) {
        std::cerr << "Niepoprawny numer portu." << std::endl;
        return false;
    }
    std::string sizeStr = argv[4];
    if (!isValidSize(sizeStr)) {
        std::cerr << "Niepoprawny rozmiar." << std::endl;
        return false;
    }
    return true;
}
void ERROR(const char* str)
{
    fprintf(stderr, "%s: %s\n", str, strerror(errno));
    exit(EXIT_FAILURE);
}
void checkIpAndPort(const char* target, int port)
{
    struct hostent *host = gethostbyname(target);
    if (host == NULL){
        cerr << "Niepoprawny adres IP" << endl;
        exit(1);
    }
    if (port <= 0 || port > 65535)
    {
        cerr << "Niepoprawny port" << endl;
        exit(1);
    }
}
bool notTrash(u_int8_t packet[IP_MAXPACKET+1])
{
    return packet[0] == 'D' and packet[1] == 'A' and packet[2] == 'T' and packet[3] == 'A';
}
void wyzeruj(u_int8_t packets[ARRAY_SIZE][IP_MAXPACKET+1]) {
    for (size_t i = 0; i < ARRAY_SIZE; ++i) {
        for (size_t j = 0; j < IP_MAXPACKET+1; ++j) {
            packets[i][j] = 0;
        }
    }
}
void setServerAddressAndPort(struct sockaddr_in& server_address, string ipAddress, int portNumber)
{
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portNumber);
    inet_pton(AF_INET, ipAddress.c_str(), &server_address.sin_addr);
}
bool hasPacket(u_int8_t packet[IP_MAXPACKET+1])
{
    for (size_t i = 0; i < IP_MAXPACKET+1; i++)
    {
        if(packet[i]!= 0)
        {
            return true;
        }
    }
    return false;
    
}
void resendPackets(u_int8_t packets[ARRAY_SIZE][IP_MAXPACKET+1], int start, int arrayStart, int sock_fd, struct sockaddr_in& server_address, int size)
{
    for(int i = 0; i < ARRAY_SIZE; i++)
    {
        if(not hasPacket(packets[(arrayStart + i) % ARRAY_SIZE]))
        {
            if(start + i*1000 < size)
            {
                string message = "GET " + to_string(start + i*1000) + " " + to_string(min(1000,size-(start + i*1000))) + "\n";
                if (sendto(sock_fd, message.c_str(), strlen(message.c_str()), 0, (struct sockaddr*) &server_address, sizeof(server_address)) != (long int) strlen(message.c_str()))
                {
                    ERROR("sendto error");
                }
            }
        }
    }
}
void setDescriptors(int& sock_fd,fd_set& descriptors,struct timeval& tv)
{
    FD_ZERO( &descriptors );
    FD_SET(sock_fd, &descriptors );
    tv.tv_sec = 0 ; tv.tv_usec = TIME_OUT;
}
int packetNumber(u_int8_t buffer[IP_MAXPACKET+1])
{
    std::string text(reinterpret_cast<char*>(buffer));
    std::istringstream iss(text);
    std::string secondWord;
    iss >> secondWord; 
    iss >> secondWord;
    return stoi(secondWord);
}
int calculatePlace(u_int8_t buffer[IP_MAXPACKET+1], int start, int arrayStart)
{
    int val = packetNumber(buffer) - start;
    return (arrayStart + val/1000)%ARRAY_SIZE;
}
void writeToFile(u_int8_t packet[IP_MAXPACKET+1], std::ofstream& plik,int bits) {
    int place = 0;
    for (int i = 0; packet[i] != '\0'; ++i) {
        if (packet[i] == '\n') {
            place = i;
            break;
        }
    }
    plik.write(reinterpret_cast<char*>(packet+place+1),bits);
}
void sendNextPacket(int& sock_fd, struct sockaddr_in& server_address, int start, int size)
{
    if(start + ARRAY_SIZE*1000 < size)
    {
        string message = "GET " + to_string(start + ARRAY_SIZE*1000) + " " + to_string(min(1000,size-(start + ARRAY_SIZE*1000))) + "\n";
        if (sendto(sock_fd, message.c_str(), strlen(message.c_str()), 0, (struct sockaddr*) &server_address, sizeof(server_address)) != (long int) strlen(message.c_str()))
        {
            ERROR("sendto error");
        }
    }
}
int main(int argc, char const *argv[])
{
    auto begin = std::chrono::high_resolution_clock::now();

    if (!validateArguments(argc, argv)) {
        std::cerr << "Użycie: " << argv[0] << " ipAddress portNumber fileName size" << std::endl;
        return EXIT_FAILURE;
    }
    string ipAddress = argv[1],fileName = argv[3];
    int portNumber = atoi(argv[2]),size = atoi(argv[4]);
    
    checkIpAndPort(ipAddress.c_str(),portNumber);

    std::ofstream plik(fileName, std::ios::binary | std::ios::trunc);
    if (not plik.is_open()) {
        ERROR("Nie udalo sie otworzyc pliku");
    }

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0)
        ERROR("socket error");

    if (argc != 5) {
        fprintf (stderr, "usage: %s number_of_datagrams\n", argv[0]);
        return EXIT_FAILURE;
    }


    struct sockaddr_in server_address;
    setServerAddressAndPort(server_address, ipAddress, portNumber);
    
    struct sockaddr_in 	sender;
    socklen_t sender_len = sizeof(sender);
    u_int8_t buffer[IP_MAXPACKET+1];
    u_int8_t packets[ARRAY_SIZE][IP_MAXPACKET+1];
    int start = 0;
    wyzeruj(packets);
    auto point = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(TIME_OUT);
    int arrayStart = 0;
    int last = 0;
    while(start < size) {
        if(100*start/size != last)
        {
            cout<<"loading % : "<<100*start/size<<"%"<<endl;
            last = 100*start/size;
        }
        auto now = std::chrono::high_resolution_clock::now();
        if(now >= point) { //przekroczylismy czas oczekiwania na nastepny pakiet
            point = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(TIME_OUT);
            resendPackets(packets, start, arrayStart, sock_fd,server_address,size);
        }
        else
        {
            fd_set descriptors;
            struct timeval tv;
            setDescriptors(sock_fd,descriptors,tv);
            int ready = select(sock_fd + 1, &descriptors, NULL, NULL, &tv);
            if(ready < 0)
            {
                throw std::runtime_error("Read from socket error");
            }
            if(ready == 0) 
            {
            }
            else
            {
                ssize_t datagram_len = recvfrom(sock_fd, buffer, IP_MAXPACKET, 0, (struct sockaddr*)&sender, &sender_len);
                buffer[datagram_len] = 0;
                if(notTrash(buffer))
                {
                    //printf("%ld-byte message: +%s+\n", datagram_len, buffer);
                    int place = calculatePlace(buffer, start, arrayStart);
                    //cout<<"place: "<<place<<endl;
                    if(packetNumber(buffer) >= start and packetNumber(buffer) < start + ARRAY_SIZE*1000)
                    {
                        memcpy(packets[place], buffer, sizeof(buffer));
                    }
                    while(hasPacket(packets[arrayStart]))
                    {
                        writeToFile(packets[arrayStart],plik, min(1000,size-start));
                        memset(packets[arrayStart], 0, sizeof(packets[arrayStart]));
                        
                        arrayStart = (arrayStart + 1) % ARRAY_SIZE;
                        point = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(TIME_OUT);

                        sendNextPacket(sock_fd, server_address, start, size);
                        start = start + min(1000,size-start);
                    }
                }
            }
        } 
    }
    cout<<"loading % : "<<100*start/size<<"%"<<endl;
    close(sock_fd);
    plik.close();
    auto end2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_ms = end2 - begin;
    std::cout << "Czas trwania programu: " << duration_ms.count() << " ms" << std::endl;

    return EXIT_SUCCESS;
}
