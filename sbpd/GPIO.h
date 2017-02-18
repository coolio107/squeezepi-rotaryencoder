//
//  GPIO.h
//  SqueezeButtonPi
//
//  Created by Jörg Schwieder on 02.02.17.
//  Copyright © 2017 PenguinLovesMusic.com. All rights reserved.
//

#ifndef GPIO_h
#define GPIO_h

#include <stdio.h>
#include <stdbool.h>


//
//
//  Init GPIO functionality
//  Essentially just initialized WiringPi to use GPIO pin numbering
//
//
void init_GPIO();

//
// Buttons and Rotary Encoders
// Rotary Encoder taken from https://github.com/astine/rotaryencoder
// http://theatticlight.net/posts/Reading-a-Rotary-Encoder-from-a-Raspberry-Pi/
//

//17 pins / 2 pins per encoder = 8 maximum encoders
#define max_encoders 8
//17 pins / 1 pins per button = 17 maximum buttons
#define max_buttons 17

struct button;

//
//  A callback executed when a button gets triggered. Button struct and change returned.
//  Note: change might be "0" indicating no change, this happens when buttons chatter
//  Value in struct already updated.
//
typedef void (*button_callback_t)(const struct button * button, int change);

struct button {
    int pin;
    volatile bool value;
    button_callback_t callback;
};


//
//
//  Configuration function to define a button
//  Should be run for every button you want to control
//  For each button a button struct will be created
//
//  Parameters:
//      pin: GPIO-Pin used in BCM numbering scheme
//      callback: callback function to be called when button state changed
//      edge: edge to be used for trigger events,
//            one of INT_EDGE_RISING, INT_EDGE_FALLING or INT_EDGE_BOTH (the default)
//  Returns: pointer to the new button structure
//           The pointer will be NULL is the function failed for any reason
//
//
struct button *setupbutton(int pin,
                           button_callback_t callback,
                           int edge);


struct encoder;

//
//  A callback executed when a rotary encoder changes it's value.
//  Encoder struct and change returned.
//  Value in struct already updated.
//
typedef void (*rotaryencoder_callback_t)(const struct encoder * encoder, long change);

struct encoder
{
    int pin_a;
    int pin_b;
    volatile long value;
    volatile int lastEncoded;
    rotaryencoder_callback_t callback;
};

//
//
//  Configuration function to define a rotary encoder
//  Should be run for every rotary encoder you want to control
//  For each encoder a button struct will be created
//
//  Parameters:
//      pin_a, pin_b: GPIO-Pins used in BCM numbering scheme
//      callback: callback function to be called when encoder state changed
//      edge: edge to be used for trigger events,
//            one of INT_EDGE_RISING, INT_EDGE_FALLING or INT_EDGE_BOTH (the default)
//  Returns: pointer to the new encoder structure
//           The pointer will be NULL is the function failed for any reason
//
//
struct encoder *setupencoder(int pin_a,
                             int pin_b,
                             rotaryencoder_callback_t callback,
                             int edge);





#endif /* GPIO_h */
