//  sbpd SqueezeButtonPi Daemon
//
//  Copyright (c) 2017, Joerg Schwieder, PenguinLovesMusic.com
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of ickStream nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
//  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//


# SqeezeButtonPi Daemon
SqueezeButtonPi Daemon or sbpd is a controller tool to use buttons and rotary encoders to control an instance of SqueezeLite or SqueezePlay running on the same Raspberry Pi device in connection with a Logitech Media Server/Squeezebox Server instance.

Rotary encoders or rotary-push-encoders can be used for volume, buttons (and the push-function of a rotary encoder) can be used for play/pause, skip forward, skip back or toggle the power state.

## Dependencies
SqueezeButtonPi uses WiringPi

## Configuration

## Security

One issue with this code is that since it uses WiringPi it needs to be run with root privileges.
This is not a particularly good idea given that it also communicates over the network so if you run this on the Raspberry Pi controlling your nuclear powerplant in the backyard I would at least advice against exposing it to the internet.
A better architecture would probably be to fork a separate process running with more limited user rights for the networking stuff.

## Limitations

### IPv6
In the current state, this will probably not work in IPv6-only networks.

### Server Switching
The controller will follow the player if you switch the player to a new server. But this can sometimes be a bit slow and take up to a minute or so.
The reason is that the controller simply checks /pric/net/tcp for a tcp connection on port 3483 - that's a Squeezebox player. But it seems sometimes the old entry doesn't immediately go away when switching servers so it can take a while until the controler notices the change.
It's reliable overall, though.'

### Encoder Speed
Server commands are fed to the server using a very simple scheduler on the main thread. Up to 10 commands per second can be sent but since all requests are being sent synchronously this depends on the reaction speed of the server.
The result of this is that very fast command sequences can result in jumping volume levels and delayed volume changes.

### Multiple Players
Probably not a limitation on a Pi. Only a single instance of SqueezeLite should be running if autodetection is being used since the code only looks for the first connection on port 3483.
With more than one player the server being found will be random. In such a setup, manual server configuration will be required.

### Multiple Network Interfaces
The MAC address detection is borrowed from SqueezeLite so when running automatically the MAC found should be the same used by SqueezeLite.
If that's not the case in setups with more than one MAC or SqueezeLite uses a manually configured MAC manual MAC configuration should be used.

### MySqueezebox.com
This tool will only work with Logitech Media Server, it doesn't authenticate against MySqueezebox.com. While authentication against MySqueezebox.com should generally be possible for a controller it should be unneeded in this case because DIY and 3rd party players are not supposed to directly connect to MySqueezebox.com anyway and the design of this tool is to control a player running on the same device as this controller.
