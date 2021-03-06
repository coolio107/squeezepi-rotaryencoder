# squeezepi-rotaryencoder
This is a tool to use a rotary-push encoder to control volume and play/pause for a SqueezeLite or other Squeezebox compatible software player on Raspberry Pi

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
