//
//  servercomm.h
//  SqueezeButtonPi
//
//  Created by Jörg Schwieder on 02.02.17.
//  Copyright © 2017 PenguinLovesMusic.com. All rights reserved.
//

#ifndef servercomm_h
#define servercomm_h

#include "sbpd.h"

//
//
//  Initialize CURL for server communication and set MAC address
//
//
int init_comm(char * use_mac);

//
//
//  Shutdown CURL
//
//
void shutdown_comm();

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
//  Returns: success flag
//
//
bool send_command(struct sbpd_server * server, char * fragment);

#endif /* servercomm_h */
