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
#include <time.h>

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






static unsigned long lasttime = 0;
static long lastVolume = 0;
void handleVolume() {
    struct timespec thistime;
    clock_gettime(0, &thistime);
    unsigned long time = thistime.tv_sec * 10 + thistime.tv_nsec / (1e8);
    if (lasttime - time < 2) {
        return;
    }
    if (lastVolume != encoder->value) {
        printf("Time: %ul Volume Change: %d\n", time, encoder->value - lastVolume);
        lastVolume = encoder->value;
        lasttime = time;
    }
}

void reactEncoderInterrupt(const struct encoder * encoder, long change) {
    printf("Interrupt: encoder value: %d change: %d\n", encoder->value, change);
    handleVolume();
}

void buttonPress(const struct button * button, int change) {
    printf("Interrupt, button value: %d change: %d\n", button->value, change);
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
    

    //------------------------------------------------------------------------
    // Setup Rotary Encoder
    //------------------------------------------------------------------------
    //fprintf (stderr, "start err\n");
    printf ("start out\n");
    wiringPiSetup() ;
    //struct encoder *encoder = setupencoder(5, 4);
    encoder = setupencoder(5, 4, reactEncoderInterrupt, INT_EDGE_RISING);
    button = setupbutton(6, buttonPress, INT_EDGE_RISING);
    
    // button
    //pinMode(6, INPUT);
    //pullUpDnControl(6, PUD_UP);
    //wiringPiISR(6,INT_EDGE_BOTH, buttonPressF);

    //------------------------------------------------------------------------
    // Mainloop:
    //------------------------------------------------------------------------
    while( !stop_signal ) {
        printf("Polling: encoder value: %d\n", encoder->value);
        handleVolume();
        //------------------------------------------------------------------------
        // Just sleep...
        //------------------------------------------------------------------------
        usleep( 200000 ); // 0.2s
        
    } /* end of: while( !stopflag ) */
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
