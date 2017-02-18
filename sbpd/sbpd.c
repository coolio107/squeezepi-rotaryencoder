//
//  main.c
//  SqueezeButtonPi
//
//  Squeezebox Button Control for Raspberry Pi - Daemon.
//
//  Use buttons and rotary encoders to control SqueezeLite or SqueezePlay running on a Raspberry Pi
//  Works with Logitech Media Server
//
//  Created by Jörg Schwieder on 02.02.17.
//  Copyright © 2017 PenguinLovesMusic.com. All rights reserved.
//

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <argp.h>
#include <sys/time.h>
#include <sys/param.h>
#include "sbpd.h"
#include "discovery.h"
#include "servercomm.h"
#include "control.h"

//
//  Server configuration
//
static sbpd_config_parameters_t configured_parameters = 0;
static sbpd_config_parameters_t discovered_parameters = 0;
static struct sbpd_server server;
static char * MAC;

//
//  signal handling
//
static volatile int stop_signal;
static void sigHandler( int sig, siginfo_t *siginfo, void *context );

//
//  Logging
//
static int streamloglevel = LOG_NOTICE;
static int sysloglevel = LOG_ALERT;

//
//  Argument Parsing
//
const char *argp_program_version = USER_AGENT " " VERSION;
const char *argp_program_bug_address = "<coolio@penguinlovesmusic.com>";
static error_t parse_opt(int key, char *arg, struct argp_state *state);
//
//  OPTIONS.  Field 1 in ARGP.
//  Order of fields: {NAME, KEY, ARG, FLAGS, DOC, GROUP}.
//
static struct argp_option options[] =
{
    { "mac",       'M', "MAC-Address", 0,
        "Set MAC address of player. Deafult: autodetect", 0 },
    { "address",   'A', "Server-Address", 0,
        "Set server address. Default: autodetect", 0 },
    { "port",      'P', "xxxx", 0, "Set server control port. Default: autodetect", 0 },
    { "username",  'u', "user name", 0, "Set user name for server. Default: none", 0 },
    { "password",  'p', "password", 0, "Set password for server. Default: none", 0 },
    { "verbose",   'v', 0, 0, "Produce verbose output", 1 },
    { "silent",    's', 0, 0, "Don't produce output", 1 },
    { "daemonize", 'd', 0, 0, "Daemonize", 1 },
    { "kill",      'k', 0, 0, "Kill daemon", 1 },
    {0}
};
//
//  ARGS_DOC. Field 3 in ARGP.
//  Non-Option arguments.
//  At least one needs to be specified for the daemon to do anything useful
//  Arguments are a comma-separated list of configuration parameters:
//  For rotary encoders (one, volume only):
//      e,pin1,pin2,CMD[,edge]
//          "e" for "Encoder"
//          p1, p2: GPIO PIN numbers in BCM-notation
//          CMD: Command. Unused for encoders, always VOLM for Volume
//          edge: Optional. one of
//                  1 - falling edge
//                  2 - rising edge
//                  0, 3 - both
//  For buttons:
//      b,pin,CMD[,edge]
//          "b" for "Button"
//          pin: GPIO PIN numbers in BCM-notation
//          CMD: Command. One of:
//              PLAY:   Play/pause
//              PREV:   Jump to previous track
//              NEXT:   Jump to next track
//              VOL+:   Increase volume
//              VOL-:   Decrease volume
//              POWR:   Toggle power state
//          edge: Optional. one of
//                  1 - falling edge
//                  2 - rising edge
//                  0, 3 - both
//
static char args_doc[] = "[e,pin1,pin2,CMD,edge] [b,pin,CMD,edge...]";
//
//  DOC.  Field 4 in ARGP.
//  Program documentation.
//
static char doc[] = "sbpd - SqueezeButtinPiDaemon is a button and rotary encoder handling daemon for Raspberry Pi and a Squeezebox player software.\nsbpd connects to a Squeezebox server and sends the configured control commands on behalf of a player running on the RPi.\n<C>2017 Joerg Schwieder/PenguinLovesMusic.com";
//
//  ARGP parsing structure
//
static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char * argv[]) {
    //
    // Configure signal handling
    //
    struct sigaction act;
    memset( &act, 0, sizeof(act) );
    act.sa_sigaction = &sigHandler;
    act.sa_flags     = SA_SIGINFO;
    sigaction( SIGINT, &act, NULL );
    sigaction( SIGTERM, &act, NULL );
    
    //
    // Configure pid file
    //
    FILE            *fp;
    const char      *pid_fname      = "/var/run/sbpd.pid";
    fp = fopen( pid_fname, "w" );
    if( fp ) {
        fprintf( fp, "%d\n", getpid() );
        fclose( fp );
    }
    
    //
    //  Parse Arguments
    //
    argp_parse (&argp, argc, argv, 0, 0, 0);
    
    //
    // Find MAC
    //
    if (!(configured_parameters & SBPD_cfg_MAC)) {
        MAC = find_mac();
        if (!MAC)
            return -1;  // no MAC, no control
        discovered_parameters |= SBPD_cfg_MAC;
    }
    
    //
    //  Initialize server communication
    //
    init_comm(MAC);

    //
    //
    // Main Loop
    //
    //
    while( !stop_signal ) {
        //
        //  Poll the server discovery
        //
        poll_discovery(configured_parameters,
                       &discovered_parameters,
                       &server);
        handle_buttons(&server);
        handle_encoders(&server);
        //
        // Just sleep...
        //
        usleep( SCD_SLEEP_TIMEOUT ); // 0.1s
        
    } // end of: while( !stop_signal )
    
    //
    //  Shutdown server communication
    //
    shutdown_comm();
    
    return 0;
}

//
//
//  Argument parsing
//
//
//
//  PARSER. Field 2 in ARGP.
//  Order of parameters: KEY, ARG, STATE.
//
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    switch (key)
    {
            //
            //  Output Modes and Misc.
            //
            //  Verbose mode
        case 'v':
            streamloglevel = LOG_DEBUG;
            break;
            //  Silent Mode
        case 's':
            streamloglevel = 0;
            break;
            //  Daemonize
        case 'd':
            break;
            
            //
            //  Player parameter
            //
            //  MAC Address
        case 'M':
            MAC = arg;
            configured_parameters |= SBPD_cfg_MAC;
            break;
            
            //
            // Server Parameters
            //
            //  Server address
        case 'A':
            server.host = arg;
            configured_parameters |= SBPD_cfg_host;
            break;
            //  Server port
        case 'P':
            server.port = (uint32_t)strtoul(arg, NULL, 10);
            configured_parameters |= SBPD_cfg_port;
            break;
            //  Server user name
        case 'u':
            server.user = arg;
            configured_parameters |= SBPD_cfg_user;
            break;
            //  Server user name
        case 'p':
            server.password = arg;
            configured_parameters |= SBPD_cfg_password;
            break;
            
            //
            //  GPIO devices
            //
            //  Arguments are a comma-separated list of configuration parameters:
            //  For rotary encoders (one, volume only):
            //      e,pin1,pin2,CMD[,edge]
            //          "e" for "Encoder"
            //          p1, p2: GPIO PIN numbers in BCM-notation
            //          CMD: Command. Unused for encoders, always VOLM for Volume
            //          edge: Optional. one of
            //                  1 - falling edge
            //                  2 - rising edge
            //                  0, 3 - both
            //  For buttons:
            //      b,pin,CMD[,edge]
            //          "b" for "Button"
            //          pin: GPIO PIN numbers in BCM-notation
            //          CMD: Command. One of:
            //              PLAY:   Play/pause
            //              PREV:   Jump to previous track
            //              NEXT:   Jump to next track
            //              VOL+:   Increase volume
            //              VOL-:   Decrease volume
            //              POWR:   Toggle power state
            //          edge: Optional. one of
            //                  1 - falling edge
            //                  2 - rising edge
            //                  0, 3 - both
            //
        case ARGP_KEY_ARG: {
            char * code = strtok(arg, ",");
            if (strlen(code) != 1)
                return ARGP_ERR_UNKNOWN;
            switch (code[0]) {
                case 'e': {
                    char * string = strtok(NULL, ",");
                    int p1 = 0;
                    if (string)
                        p1 = (int)strtol(string, NULL, 10);
                    string = strtok(NULL, ",");
                    int p2 = 0;
                    if (string)
                        p2 = (int)strtol(string, NULL, 10);
                    char * cmd = strtok(NULL, ",");
                    string = strtok(NULL, ",");
                    int edge = 0;
                    if (string)
                        edge = (int)strtol(string, NULL, 10);
                    setup_encoder_ctrl(cmd, p1, p2, edge);
                }
                    break;
                case 'b': {
                    char * string = strtok(NULL, ",");
                    int pin = 0;
                    if (string)
                        pin = (int)strtol(string, NULL, 10);
                    char * cmd = strtok(NULL, ",");
                    string = strtok(NULL, ",");
                    int edge = 0;
                    if (string)
                        edge = (int)strtol(string, NULL, 10);
                    setup_button_ctrl(cmd, pin, edge);
                }
                    break;
                    
                default:
                    break;
            }
        }
            break;
        case ARGP_KEY_END:
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}


//
//
//  Misc. code
//
//


//
// Handle signals
//
static void sigHandler( int sig, siginfo_t *siginfo, void *context )
{
    //
    // What sort of signal is to be processed ?
    //
    switch( sig ) {
            //
            // A normal termination request
            //
        case SIGINT:
        case SIGTERM:
            stop_signal = sig;
            break;
            //
            // Ignore broken pipes
            //
        case SIGPIPE:
            break;
    }
}

//
//  Logging facility
//
void _mylog( const char *file, int line,  int prio, const char *fmt, ... )
{
    //
    // Init arguments, lock mutex
    //
    va_list a_list;
    va_start( a_list, fmt );
    
    //
    //  Log to stream
    //
    if( prio <= streamloglevel) {
        
        // select stream due to priority
        FILE *f = (prio < LOG_INFO) ? stderr : stdout;
        //FILE *f = stderr;
        
        // print timestamp, prio and thread info
        struct timeval tv;
        gettimeofday( &tv, NULL );
        double time = tv.tv_sec+tv.tv_usec*1E-6;
        fprintf( f, "%.4f %d", time, prio);
        
        // prepend location to message (if available)
        if( file )
            fprintf( f, " %s,%d: ", file, line );
        else
            fprintf( f, ": " );
        
        // the message itself
        vfprintf( f, fmt, a_list );
        
        // New line and flush stream buffer
        fprintf( f, "\n" );
        fflush( f );
    }
    
    //
    //  use syslog facility, hide debugging messages from syslog
    //
    if( prio <= sysloglevel && prio < LOG_DEBUG)
        vsyslog( prio, fmt, a_list );
    
    //
    //  Clean variable argument list, unlock mutex
    //
    va_end ( a_list );
}

int loglevel() {
    return MAX(sysloglevel, streamloglevel);
}
