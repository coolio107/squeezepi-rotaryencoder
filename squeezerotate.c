/*

SqueezeRotate - Sets the volume of a squeezebox player running on a raspberry pi

<C> 2017 Jörg Schwieder

*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <signal.h>

#include <wiringPi.h>

#include "squeezerotate.h"
#include "rotaryencoder/rotaryencoder.h"

static volatile int stop_signal;
//static void sigHandler( int sig, siginfo_t *siginfo, void *context );

static struct encoder *encoder

void reactEncoderInterrupt() {
    printf("Interrupt: encoder value: %d\n", encoder->value);
}

void buttonPressF() {
    int bit = digitalRead(6);
    printf("Interrupt, falling edge: button value: %d\n", bit);
}

void buttonPressR() {
    int bit = digitalRead(6);
    printf("Interrupt, rising edge: button value: %d\n", bit);
}

int main( int argc, char *argv[] ) {

    //------------------------------------------------------------------------
    // OK, from here on we catch some terminating signals and ignore others
    //------------------------------------------------------------------------
    /*struct sigaction act;
    memset( &act, 0, sizeof(act) );
    act.sa_sigaction = &sigHandler;
    act.sa_flags     = SA_SIGINFO;
    sigaction( SIGINT, &act, NULL );
    sigaction( SIGTERM, &act, NULL );*/

    //------------------------------------------------------------------------
    // Setup PID file, ignore errors...
    //------------------------------------------------------------------------
    /*fp = fopen( pid_fname, "w" );
    if( fp ) {
        fprintf( fp, "%d\n", getpid() );
        fclose( fp );
    }*/
    

    //------------------------------------------------------------------------
    // Setup Rotary Encoder
    //------------------------------------------------------------------------
    //fprintf (stderr, "start err\n");
    printf ("start out\n");
    wiringPiSetup() ;
    //struct encoder *encoder = setupencoder(5, 4);
    encoder = setupencoder(5, 4);
    wiringPiISR(5,INT_EDGE_BOTH, updateEncoders);
    wiringPiISR(4,INT_EDGE_BOTH, updateEncoders);
    
    // button
    pinMode(6, INPUT);
    pullUpDnControl(6, PUD_UP);
    wiringPiISR(6,INT_EDGE_FALLING, buttonPressF);
    wiringPiISR(6,INT_EDGE_RISING, buttonPressR);

    //------------------------------------------------------------------------
    // Mainloop:
    //------------------------------------------------------------------------
    while( 1 /*!stop_signal*/ ) {
        printf("Polling: encoder value: %d\n", encoder->value);
        //------------------------------------------------------------------------
        // Just sleep...
        //------------------------------------------------------------------------
        sleep( 1 );
        
    } /* end of: while( !stopflag ) */
}

//=========================================================================
// Handle signals
//=========================================================================
/*static void sigHandler( int sig, siginfo_t *siginfo, void *context )
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
} */
