//
//  control.h
//  SqueezeButtonPi
//
//  Created by Jörg Schwieder on 02.02.17.
//  Copyright © 2017 PenguinLovesMusic.com. All rights reserved.
//

#ifndef control_h
#define control_h

#include "sbpd.h"
#include "GPIO.h"

//
//  Store command parameters for each button used
//
struct button_ctrl
{
    struct button * gpio_button;
    volatile bool waiting;
    char * fragment;
};

//
//  Setup button control
//  Parameters:
//      cmd: Command. One of
//                  PLAY    - play/pause
//                  VOL+    - increment volume
//                  VOL-    - decrement volume
//                  PREV    - previous track
//                  NEXT    - next track
//      pin: the GPIO-Pin-Number
//      edge: one of
//                  1 - falling edge
//                  2 - rising edge
//                  0, 3 - both
//
int setup_button_ctrl(char * cmd, int pin, int edge);

//
//  Polling function: handle button commands
//  Parameters:
//      server: the server to send commands to
//
void handle_buttons(struct sbpd_server * server);

//
//  Store command parameters for each button used
//
struct encoder_ctrl
{
    struct encoder * gpio_encoder;
    volatile long last_value;
    char * fragment;
};
//
//  Setup encoder control
//  Parameters:
//      cmd: Command. Currently only
//                  VOLU    - volume
//          Can be NULL for volume or actually anything since it's ignored
//      pin1: the GPIO-Pin-Number for the first pin used
//      pin2: the GPIO-Pin-Number for the second pin used
//      edge: one of
//                  1 - falling edge
//                  2 - rising edge
//                  0, 3 - both
//
int setup_encoder_ctrl(char * cmd, int pin1, int pin2, int edge);

//
//  Polling function: handle encoders
//  Parameters:
//      server: the server to send commands to
//
void handle_encoders(struct sbpd_server * server);


#endif /* control_h */
