/*

SqueezeRotate - Sets the volume of a squeezebox player running on a raspberry pi

<C> 2017 Jörg Schwieder

*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <curl/curl.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <wiringPi.h>

#include "squeezerotate.h"
//#include "rotaryencoder/rotaryencoder.h"






// Headers


// Buttons and Rotary Encoders
// Rotary Encoder taken from https://github.com/astine/rotaryencoder
// http://theatticlight.net/posts/Reading-a-Rotary-Encoder-from-a-Raspberry-Pi/

//17 pins / 2 pins per encoder = 8 maximum encoders
#define max_encoders 8
//17 pins / 1 pins per button = 17 maximum buttons
#define max_buttons 17

struct button;

// A callback executed when a button gets triggered. Button struct and change returned.
// Note: change might be "0" indicating no change, this happens when buttons chatter
// Value in struct already updated.
typedef void (*button_callback_t)(const struct button * button, int change);

struct button {
    int pin;
    volatile bool value;
    button_callback_t callback;
};


//Pre-allocate encoder objects on the stack so we don't have to
//worry about freeing them
struct button buttons[max_buttons];

/*
 Should be run for every rotary encoder you want to control
 Returns a pointer to the new rotary encoder structer
 The pointer will be NULL is the function failed for any reason
 */
struct button *setupbutton(int pin, button_callback_t callback, int edge);


struct encoder;

// A callback executed when a rotary encoder changes it's value. Encoder struct and change returned.
// Value in struct already updated.
typedef void (*rotaryencoder_callback_t)(const struct encoder * encoder, long change);

struct encoder
{
    int pin_a;
    int pin_b;
    volatile long value;
    volatile int lastEncoded;
    rotaryencoder_callback_t callback;
};


//Pre-allocate encoder objects on the stack so we don't have to
//worry about freeing them
struct encoder encoders[max_encoders];

/*
 Should be run for every rotary encoder you want to control
 Returns a pointer to the new rotary encoder structer
 The pointer will be NULL is the function failed for any reason
 */
struct encoder *setupencoder(int pin_a, int pin_b, rotaryencoder_callback_t callback, int edge);






// address search

// mac address. From SqueezeLite so should match that behaviour.
// search first 4 interfaces returned by IFCONF
void get_mac(uint8_t mac[]) {
    char *utmac;
    struct ifconf ifc;
    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifreq ifs[4];
    
    utmac = getenv("UTMAC");
    if (utmac)
    {
        if ( strlen(utmac) == 17 )
        {
            if (sscanf(utmac,"%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
                       &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]) == 6)
            {
                return;
            }
        }
        
    }
    
    mac[0] = mac[1] = mac[2] = mac[3] = mac[4] = mac[5] = 0;
    
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    
    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;
    
    if (ioctl(s, SIOCGIFCONF, &ifc) == 0) {
        ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
        
        for (ifr = ifc.ifc_req; ifr < ifend; ifr++) {
            if (ifr->ifr_addr.sa_family == AF_INET) {
                
                strncpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));
                if (ioctl (s, SIOCGIFHWADDR, &ifreq) == 0) {
                    memcpy(mac, ifreq.ifr_hwaddr.sa_data, 6);
                    if (mac[0]+mac[1]+mac[2] != 0) {
                        break;
                    }
                }
            }
        }
    }
    
    close(s);
}





// returns true if server IP was found and changed
bool get_serverIPv4(uint32_t *ip) {
    uint32_t foundIp;
    FILE * procTcp = fopen("/proc/net/tcp", "r");
    if (!procTcp)
        return false;
    char line[256];
    if (!fgets(line, 255, procTcp)) {
        fclose(procTcp);
        return false;
    }
    while (fgets(line, 255, procTcp)) {
        //printf("line: %s\n", line);
        strtok(line, " "); // line number
        strtok(NULL, " "); // source address
        char * target = strtok(NULL, " "); // target address
        //printf("target: %s\n", target);
        if (!target) {
            //printf("no target\n");
            fclose(procTcp);
            return false;
        }
        char * ipString = strtok(target, ":");
        char * portString = strtok(NULL, ":");
        if (!ipString || !portString) {
            //printf((portString) ? "no ipString\n" : "no portString");
            fclose(procTcp);
            return false;
        }
        char * portComp = "0D9B";
        // just be sure...
        char * sTemp = portString;
        while (*sTemp = toupper(*sTemp))
            sTemp++;
        // port 3483?
        if (*((uint32_t *)portComp) == *((uint32_t *)portString)) {
            fclose(procTcp);
            printf ("found %s ", ipString);
            foundIp = strtoul(ipString, NULL, 16);
            if (foundIp != *ip) {
                *ip = foundIp;
                printf("is new address\n");
                return true;
            }
            printf("same as before\n");
            return false;
        }
    }
    fclose(procTcp);
}

static int udpSocket = 0;
static uint32_t udpAddress;
# define SIZE_SERVER_DISCOVERY_LONG 23
# define SBS_UDP_PORT 3483

// get port through server discovery

// send server discovery
void sendDicovery(uint32_t address) {
    if (udpSocket)
        close(udpSocket);
    // create discovery socket
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    int yes = 1;
    setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(int));
    setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes));
    
    // send packet
    udpAddress = address;
    struct sockaddr_in addr4;
    memset(&addr4, 0, sizeof(addr4));
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(SBS_UDP_PORT);
    addr4.sin_addr.s_addr = address;
    
    char * data = "eIPAD\0NAME\0JSON\0UUID\0\0\0";
    
    ssize_t error;
    error = sendto(udpSocket, data, SIZE_SERVER_DISCOVERY_LONG, 0, (struct sockaddr*)&addr4, sizeof(addr4));
}

// poll udp port for discovery reply
#define BUFSIZE 1600
#define STRTOU32(x) (*((uint32_t *)x))
uint32_t readDiscovery(uint32_t address) {
    char buffer[BUFSIZE];
    static struct sockaddr_in returnAddr;
    memset(&returnAddr, 0, sizeof(returnAddr));
    returnAddr.sin_family = AF_INET;
    //returnAddr.sin_port = htons(SBS_UDP_PORT);
    returnAddr.sin_addr.s_addr = address;
    //returnAddr.sin_len = sizeof(returnAddr);
    socklen_t addrSize = sizeof(returnAddr);
    
    ssize_t size = recvfrom(udpSocket,
                            (void *)buffer,
                            sizeof(buffer),
                            MSG_DONTWAIT,
                            (struct sockaddr *)&returnAddr,
                            &addrSize);
    
    if ((size <= 0) ||
        (buffer[0] != 'E')) {
        //printf("discovery: no reply, yet\n");
        return 0;
    }
    
    unsigned int pos = 1;
    char code[5];
    code[4] = 0;
    // this is the whole packet interpretation for debug purposes
    // not needed in final code
    char port[6];
    strncpy(port, "9000\0", 6);
    char name[256];
    memset(name, 0, sizeof(name));
    char UUID[256];
    memset(UUID, 0, sizeof(UUID));
    while (pos < (size - 5)) {
        unsigned int fieldLen = buffer[pos + 4];
        uint32_t * selector = (uint32_t *)(buffer + pos);
        if (*selector == STRTOU32("NAME")) {
            strncpy(name, buffer + pos + 5, MIN(fieldLen, sizeof(name)));
        } else if (*selector == STRTOU32("JSON")) {
            strncpy(port, buffer + pos + 5, MIN(fieldLen, sizeof(port)));
        } else if (*selector == STRTOU32("UUID")) {
            strncpy(UUID, buffer + pos + 5, MIN(fieldLen, sizeof(UUID)));
        }
        pos += fieldLen + 5;
    }
    printf("port: %s, name: %s, uuid: %s\n", port, name, UUID);
    close(udpSocket);
    
    return strtoul(port, NULL, 10);
}








static volatile int stop_signal;
static void sigHandler( int sig, siginfo_t *siginfo, void *context );

static struct encoder *encoder = NULL;
static struct button *button = NULL;

// buttons

int numberofbuttons = 0;

void updateButtons()
{
    struct button *button = buttons;
    for (; button < buttons + numberofbuttons; button++)
    {
        bool bit = digitalRead(button->pin);
        
        int increment = 0;
        // same? no increment
        if (button->value != bit)
            increment = (bit) ? 1 : -1; // Increemnt and current state true: positive increment
        
        button->value = bit;
        
        if (button->callback)
            button->callback(button, increment);
    }
}

struct button *setupbutton(int pin, button_callback_t callback, int edge)
{
    if (numberofbuttons > max_buttons)
    {
        printf("Maximum number of buttons exceded: %i\n", max_buttons);
        return NULL;
    }
    
    if (edge != INT_EDGE_FALLING && edge != INT_EDGE_RISING)
        edge = INT_EDGE_BOTH;
    
    struct button *newbutton = buttons + numberofbuttons++;
    newbutton->pin = pin;
    newbutton->value = 0;
    newbutton->callback = callback;
    
    pinMode(pin, INPUT);
    pullUpDnControl(pin, PUD_UP);
    wiringPiISR(pin,edge, updateButtons);
    
    return newbutton;
}


// Buttons and Encoders
// Rotary Encoder taken from https://github.com/astine/rotaryencoder
// http://theatticlight.net/posts/Reading-a-Rotary-Encoder-from-a-Raspberry-Pi/

int numberofencoders = 0;

void updateEncoders()
{
    struct encoder *encoder = encoders;
    for (; encoder < encoders + numberofencoders; encoder++)
    {
        int MSB = digitalRead(encoder->pin_a);
        int LSB = digitalRead(encoder->pin_b);
        
        int encoded = (MSB << 1) | LSB;
        int sum = (encoder->lastEncoded << 2) | encoded;
        
        int increment = 0;
        
        if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) increment = 1;
        if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) increment = -1;
        
        encoder->value += increment;
        
        encoder->lastEncoded = encoded;
        if (encoder->callback)
            encoder->callback(encoder, increment);
    }
}

struct encoder *setupencoder(int pin_a, int pin_b, rotaryencoder_callback_t callback, int edge)
{
    if (numberofencoders > max_encoders)
    {
        printf("Maximum number of encodered exceded: %i\n", max_encoders);
        return NULL;
    }

    if (edge != INT_EDGE_FALLING && edge != INT_EDGE_RISING)
        edge = INT_EDGE_BOTH;

    struct encoder *newencoder = encoders + numberofencoders++;
    newencoder->pin_a = pin_a;
    newencoder->pin_b = pin_b;
    newencoder->value = 0;
    newencoder->lastEncoded = 0;
    newencoder->callback = callback;
    
    pinMode(pin_a, INPUT);
    pinMode(pin_b, INPUT);
    pullUpDnControl(pin_a, PUD_UP);
    pullUpDnControl(pin_b, PUD_UP);
    wiringPiISR(pin_a,edge, updateEncoders);
    wiringPiISR(pin_b,edge, updateEncoders);
    
    return newencoder;
}



static CURL *curl;
static volatile bool commLock;
static pthread_mutex_t lock;

static char * server = "192.168.0.25";
static uint32_t port = 9200;
static char * MAC = "7c:dd:90:a3:fd:6a";
static struct curl_slist * headerList = NULL;
static struct curl_slist * targetList = NULL;
static char * password = NULL;
static char * user = NULL;

#define JSON_CALL_MASK	"{\"id\":%ld,\"method\":\"slim.request\",\"params\":[\"%s\",%s]}"
//#define SERVER_ADDRESS_MASK "http://%s:%d/jsonrpc.js"
//#define SERVER_ADDRESS_MASK "http://%s/jsonrpc.js"

bool sendCommand(char * fragment) {
    if (!curl)
        return false;
    
    pthread_mutex_lock(&lock);
    if (commLock) {
        pthread_mutex_unlock(&lock);
        return false;
    }
    commLock = true;
    pthread_mutex_unlock(&lock);
    
    //char address[256];
    //snprintf(address, 256, SERVER_ADDRESS_MASK, server, port);
    //printf("server address: %s\n", address);
    //curl_easy_setopt(curl, CURLOPT_URL, server);
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost/jsonrpc.js");
    char target[100];
    snprintf(target, 100, "::%s:%d", server, port);
    targetList = curl_slist_append(targetList, target);
    curl_easy_setopt(curl, CURLOPT_CONNECT_TO, targetList);
    //curl_easy_setopt(curl, CURLOPT_PATH_AS_IS, 1);
    
    user = "Apple.Test";
    password = "iPeng.Test";
    
    char secret[255];
    if (user && password) {
        snprintf(secret, 255, "%s:%s", user, password);
        curl_easy_setopt(curl, CURLOPT_USERPWD, secret);
    }

    //curl_easy_setopt(curl, CURLOPT_PORT, port);
    char jsonFragment[256];
    snprintf(jsonFragment, 256, JSON_CALL_MASK, 1, MAC, fragment);
    printf("server command: %s\n", jsonFragment);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonFragment);
    if (headerList)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    CURLcode res = curl_easy_perform(curl);
    printf("curl result: %d\n", res);
    curl_slist_free_all(targetList);
    targetList = NULL;

    commLock = false;
    return true;
}




static unsigned long lasttimeVol = 0;
static long lastVolume = 0;
void handleVolume() {
    struct timespec thistime;
    clock_gettime(0, &thistime);
    unsigned long time = thistime.tv_sec * 10 + thistime.tv_nsec / (1e8);
    // chatter filter 200ms - disabled... called every 100ms anyway plus waits fro network action to complete
    /*if (lasttimeVol - time < 2) {
        return;
    }*/
    if (lastVolume != encoder->value) {
        long delta = encoder->value - lastVolume;
        printf("Time: %u Volume Change: %d\n", time, delta);
        char fragment[50];
        char * prefix = (delta > 0) ? "+" : "-";
        snprintf(fragment, 50, "[\"mixer\",\"volume\",\"%s%d\"]", prefix, abs(delta));
        
        // accumulate non-sent commands. Is this what we want?
        if (sendCommand(fragment)) {
            lastVolume = encoder->value;
            lasttimeVol = time;
        }
    }
}

void reactEncoderInterrupt(const struct encoder * encoder, long change) {
    printf("Interrupt: encoder value: %d change: %d\n", encoder->value, change);
    //handleVolume();
}

static unsigned long lasttimePause = 0;
static bool pauseWaiting = false;

void handlePlayPause() {
    if (pauseWaiting) {
        sendCommand("[\"pause\"]");
        pauseWaiting = false;
    }
}

void buttonPress(const struct button * button, int change) {
    if (button->value) {
        pauseWaiting = true;
    }
    return;
    
    struct timespec thistime;
    clock_gettime(0, &thistime);
    unsigned long time = thistime.tv_sec * 10 + thistime.tv_nsec / (1e8);
    //chatter filter 500ms
    if (lasttimePause - time < 5) {
        return;
    }
    
    printf("Interrupt, button value: %d change: %d\n", button->value, change);
    if (button->value) {
        pauseWaiting = true;
        //if (sendCommand("[\"pause\"]")) {
            lasttimePause = time;
        //}
    }
}

static bool waiting4port = false;
static in_addr_t foundAddr = 0;

void updateServer () {
    in_addr_t addr = inet_addr(server);
    bool change = get_serverIPv4(&addr);
    if (!change) {
        return;
    }
    waiting4port = true;
    foundAddr = addr;
    sendDicovery(addr);
}

void pollPort() {
    if (!waiting4port || !foundAddr)
        return;
    //in_addr_t addr = inet_addr(server);
    uint32_t foundPort = readDiscovery(foundAddr);
    
    // now we have it all....
    if (port) {
        static char foundServer[16];
        struct in_addr addr;
        addr.s_addr = foundAddr;
        char * aAddr = inet_ntoa(addr);
        printf("Address found: %s", aAddr);
        strncpy(foundServer, aAddr, 16);
        server = foundServer;
        port = foundPort;
        waiting4port = false;
    }
}


int main( int argc, char *argv[] ) {
    FILE            *fp;
    const char      *pid_fname      = "/var/run/squeezerotate.pid";


    //------------------------------------------------------------------------
    // OK, from here on we catch some terminating signals and ignore others
    //------------------------------------------------------------------------
    struct sigaction act;
    memset( &act, 0, sizeof(act) );
    act.sa_sigaction = &sigHandler;
    act.sa_flags     = SA_SIGINFO;
    sigaction( SIGINT, &act, NULL );
    sigaction( SIGTERM, &act, NULL );

    //------------------------------------------------------------------------
    // Setup PID file, ignore errors...
    //------------------------------------------------------------------------
    fp = fopen( pid_fname, "w" );
    if( fp ) {
        fprintf( fp, "%d\n", getpid() );
        fclose( fp );
    }
    
    
    // setup comm
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("comm lock mutex init failed\n");
        return -1;
    }
    curl_global_init(CURL_GLOBAL_ALL);
    // get a curl handle
    curl = curl_easy_init();
    if (!curl) {
        curl_global_cleanup();
        return -1;
    }
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    //curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
    headerList = curl_slist_append(headerList, "Content-Type: application/json");
    // session-ID?
    //headerList = curl_slist_append(headerList, "x-sdi-squeezenetwork-session: ...")
    
    
    
    // Find MAC
    uint8_t mac[6];
    get_mac(mac);
    static char macBuf[18];
    sprintf(macBuf, "%X:%X:%X:%X:%X:%X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    MAC = macBuf;
    printf("MAC: %s", MAC);
    
    

    //------------------------------------------------------------------------
    // Setup Rotary Encoder
    //------------------------------------------------------------------------
    //fprintf (stderr, "start err\n");
    printf ("start out\n");
    wiringPiSetup() ;
    //struct encoder *encoder = setupencoder(5, 4);
    encoder = setupencoder(5, 4, reactEncoderInterrupt, INT_EDGE_BOTH);
    button = setupbutton(6, buttonPress, INT_EDGE_RISING);
    
    // button
    //pinMode(6, INPUT);
    //pullUpDnControl(6, PUD_UP);
    //wiringPiISR(6,INT_EDGE_BOTH, buttonPressF);

    //------------------------------------------------------------------------
    // Mainloop:
    //------------------------------------------------------------------------
#define IP_SEARCH_TIMEOUT 30 // every 3 s
    int ipSearchCnt = 1;
    while( !stop_signal ) {
        if (!ipSearchCnt--) {
            ipSearchCnt = IP_SEARCH_TIMEOUT;
            updateServer();
        }
        pollPort();
        printf("Polling: encoder value: %d\n", encoder->value);
        handlePlayPause();
        handleVolume();
        //------------------------------------------------------------------------
        // Just sleep...
        //------------------------------------------------------------------------
        usleep( 100000 ); // 0.1s
        
    } /* end of: while( !stopflag ) */
    
    // clean up curl;
    curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

//=========================================================================
// Handle signals
//=========================================================================
static void sigHandler( int sig, siginfo_t *siginfo, void *context )
{
    
    //------------------------------------------------------------------------
    // What sort of signal is to be processed ?
    //------------------------------------------------------------------------
    switch( sig ) {
            
            //------------------------------------------------------------------------
            // A normal termination request
            //------------------------------------------------------------------------
        case SIGINT:
        case SIGTERM:
            stop_signal = sig;
            break;
            
            //------------------------------------------------------------------------
            // Ignore broken pipes
            //------------------------------------------------------------------------
        case SIGPIPE:
            break;
    }
    
    //------------------------------------------------------------------------
    // That's it.
    //------------------------------------------------------------------------
}
