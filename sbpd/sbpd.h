//
//  sbpd.h
//  SqueezeButtonPi
//
//  General state and data structure definitions
//
//  Created by Jörg Schwieder on 02.02.17.
//  Copyright © 2017 PenguinLovesMusic.com. All rights reserved.
//

#ifndef sbpd_h
#define sbpd_h

#include <ctype.h>
#include <syslog.h>

#define USER_AGENT  "SqueezeButtonPi"
#define VERSION     "1.0"

// configuration parameters.
// used to flag pre-configured or detected parameters
typedef enum {
    // server
    SBPD_cfg_host = 0x1,
    SBPD_cfg_port = 0x2,
    SBPD_cfg_user = 0x4,
    SBPD_cfg_password = 0x8,
    
    // player
    SBPD_cfg_MAC = 0x1000,
} sbpd_config_parameters_t;


// supported commands
// need to be configured
typedef enum {
    SBPD_cmd_volume = 0x1,
    SBPD_cmd_pause = 0x2,
    SBPD_cmd_next = 0x4,
    SBPD_cmd_prev = 0x8,
    SBPD_cmd_power = 0x10,
} sbpd_commands_t;


// server configuration data structure
// contains address and user/password
struct sbpd_server {
    char *      host;
    uint32_t    port;
    char *      user;
    char *      password;
};

//
//  Define scheduling behavior
//  We use usleep so this is in µs
//
#define SCD_SLEEP_TIMEOUT   100000
#define SCD_SECOND          1000000

//
//  Helpers
//
#define STRTOU32(x) (*((uint32_t *)x))  // make a 32 bit integer from a 4 char string

//
//  Logging
//
#define logerr( args... )     _mylog( __FILE__, __LINE__, LOG_ERR, args )
#define logwarn( args... )    _mylog( __FILE__, __LINE__, LOG_WARNING, args )
#define lognotice( args... )  _mylog( __FILE__, __LINE__, LOG_NOTICE, args )
#define loginfo( args... )    _mylog( __FILE__, __LINE__, LOG_INFO, args )
#define logdebug( args... )    _mylog( __FILE__, __LINE__, LOG_DEBUG, args )
void _mylog( const char *file, int line, int prio, const char *fmt, ... );
int loglevel();

#endif /* sbpd_h */
