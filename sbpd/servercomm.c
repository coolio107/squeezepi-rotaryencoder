//
//  servercomm.c
//  SqueezeButtonPi
//
//  Send CLI commands to the server
//
//  Created by Jörg Schwieder on 02.02.17.
//  Copyright © 2017 PenguinLovesMusic.com. All rights reserved.
//

#include "servercomm.h"
#include "sbpd.h"
#include <curl/curl.h>

//
// lock for asynchronous sending of commands - we don't do this right now
//
/*#include <pthread.h>

static volatile bool commLock;
static pthread_mutex_t lock;*/

static CURL *curl;
static char * MAC = NULL;
static struct curl_slist * headerList = NULL;

#define JSON_CALL_MASK	"{\"id\":%ld,\"method\":\"slim.request\",\"params\":[\"%s\",%s]}"
#define SERVER_ADDRESS_TEMPLATE "http://localhost/jsonrpc.js"

//
//
//  Send CLI command fragment to Logitech Media Server/Squeezebox Server
//  This command blocks (synchronously communicates".
//  In timing critical situations this should be called from a separate thread
//
//  Parameters:
//      server: the server information structure defining host, port etc.
//      frament: the command fragment to be sent as JSON array
//               e.g. "[\"mixer\”,\"volume\",\"+2\"]"
//               optionally: some CLI commands can take parameter hashes as "params:{}"
//  Returns: success flag
//
//
bool send_command(struct sbpd_server * server, char * fragment) {
    if (!curl)
        return false;
    
    //
    //  Sending commands asynchronously? Secure with a mutex
    //  But right now we are actually not doing that, so comment out
    //
    /*pthread_mutex_lock(&lock);
    if (commLock) {
        pthread_mutex_unlock(&lock);
        return false;
    }
    commLock = true;
    pthread_mutex_unlock(&lock);*/
    
    //
    //  target setup. We call an IPv4 ip so we need to replace a default host
    //
    struct curl_slist * targetList = NULL;
    curl_easy_setopt(curl, CURLOPT_URL, SERVER_ADDRESS_TEMPLATE);
    char target[100];
    snprintf(target, sizeof(target), "::%s:%d", server->host, server->port);
    //logdebug("Command Target: %s", target);
    targetList = curl_slist_append(targetList, target);
    curl_easy_setopt(curl, CURLOPT_CONNECT_TO, targetList);
    
    //
    //  username/password?
    //
    char secret[255];
    if (server->user && server->password) {
        snprintf(secret, sizeof(secret), "%s:%s", server->user, server->password);
        curl_easy_setopt(curl, CURLOPT_USERPWD, secret);
    }
    
    //
    //  setup payload (JSON/RPC CLI command) for POST command
    //
    char jsonFragment[256];
    snprintf(jsonFragment, sizeof(jsonFragment), JSON_CALL_MASK, 1l, MAC, fragment);
    logdebug("Server %s command: %s", target, jsonFragment);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonFragment);
    if (headerList)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    
    //
    //  Send command and clean up
    //  Note: one could retrieve a result here since all communication is synchronous!
    //
    CURLcode res = curl_easy_perform(curl);
    logdebug("Curl result: %d", res);
    curl_slist_free_all(targetList);
    targetList = NULL;
    
    //commLock = false;
    return true;
}

//
//  Curl reply callback
//  Replies from the server go here.
//  We could handle return values here but we just log.
//
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    if (size)
        logdebug("Server rely %s", buffer);
    return size;
}

//
//
//  Initialize CURL for server communication and set MAC address
//
//
int init_comm(char * use_mac) {
    loginfo("Initializing CURL");
    MAC = use_mac;
    //
    //  Setup mutex lock for comm - unused
    //
    /*if (pthread_mutex_init(&lock, NULL) != 0) {
        logerr("comm lock mutex init failed");
        return -1;
    }*/
    
    //
    //  Initialize curl comm
    //
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl) {
        curl_global_cleanup();
        return -1;
    }
    
    //
    //  Set verbose mode for communication debugging
    //
    if (loglevel() == LOG_DEBUG)
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    headerList = curl_slist_append(headerList, "Content-Type: application/json");
    char userAgent[50];
    snprintf(userAgent, sizeof(userAgent), "User-Agent: %s/%s)", USER_AGENT, VERSION);
    headerList = curl_slist_append(headerList, userAgent);
    //
    //  Add session-ID? Only needed for MySB which is not supported
    //
    //headerList = curl_slist_append(headerList, "x-sdi-squeezenetwork-session: ...")
    return 0;
}

//
//
//  Shutdown CURL
//
//
void shutdown_comm() {
    curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

