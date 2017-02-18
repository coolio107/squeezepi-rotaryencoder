//
//  discovery.h
//  SqueezeButtonPi
//
//  Created by Jörg Schwieder on 02.02.17.
//  Copyright © 2017 PenguinLovesMusic.com. All rights reserved.
//

#ifndef discovery_h
#define discovery_h

#include <stdio.h>
#include "sbpd.h"

//
//  Polling function for server discovery
//  Call from main loop
//  Handles internal scheduling between different
//
//  Parameters:
//  config: defines which parameters are preconfigured and will not be discovered
//  discovered: the discovered parameters
//  server: server configuration
//
void poll_discovery(sbpd_config_parameters_t config,
                    sbpd_config_parameters_t *discovered,
                    struct sbpd_server * server);


//
// MAC address search
//
// mac address. From SqueezeLite so should match that behaviour.
// search first 4 interfaces returned by IFCONF
//
//  Parameters:
//  config: defines which parameters are preconfigured and will not be discovered
//  discovered: the discovered parameters
//  server: server configuration
//
char * find_mac();

#endif /* discovery_h */
