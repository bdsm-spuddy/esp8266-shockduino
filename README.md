# esp8266-shockduino

ESP8266 to provide a webserver to control a shock collar

The core code to send to the collar is based on 

I've taken this and just kludged it, and then put an ESP8266 web interface
in front.

The NOTES file explains how the data needs to be sent.

---

A common form of [shock collar](https://www.amazon.com/gp/product/B00W6ZHZMI))
is controlled via a 433Mhz signal.  Some clever people reverse engineered
the transmission protocol and wrote code to let an [Arduino send these
messages](https://github.com/smouldery/shock-collar-control/blob/master/Arduino%20Modules/transmitter_vars.ino), using a [433Mhz transmitter](https://www.amazon.com/gp/product/B01DKC2EY4).

I took this idea, ported it to an ESP8266 and added a simple webUI to it.
So now the collar can be controlled over the network.

The transmitter has three connections; Ground, Power, Data.  These can be
simply connected

    Transmitter   ESP8266
            Gnd---Gnd
          Power---3.3V
           Data---D0

The built in antenna only has short distance; for increased range an additional
antenna of about 17cm can be added.

After flashing the firmware onto the ESP8266 the device will reboot and
go into "AP" mode.  The AP will be something like Collar-ABCDEF (the exact
name will depend on the board's MAC address).  You should connect your phone
or laptop to the access point.

This will let you go to http://192.168.4.1 and that will display the
WiFi configuration screen.  Once you enter the WiFi details it will reboot
and attempt to connect to your network and get an address by DHCP.  If it
can't connect after 60 seconds then it will go back to AP mode so you can
try again.

You can also set a name for this device (defaults to "shockcollar") and
a transmission key.  You only need to change these if you plan on having
multiple shockduinos.

Once connected it will attempt to use mDNS to register "shockcollar.local"
(or whatever name you picked).  If that name doesn't work then you'll
need to check your router settings to see what IP address it received.

## Pairing with the collar

Each shock collar and transmitter need to be paired.  The instructions
that came with the device should tell you how to do this; e.g. hold down
the power button for 5 seconds.

To pair the collar with the shockduino, put the collar into pairing mode
and then press the "Beep" button.  That should be all that's needed!

To verify it works you can press the Beep button again, or the vibrate
button.  

## Authentication

Normally you don't want anyone on your network to be able to use the
shockduino, so you can set a username and password.  This isn't _strong_
security, because we don't use TLS, but it's probably "good enough".

## Use as an API

Although the device was designed to be driven from the WebUI, it is possible
to access it via simple GET requests.

The base URL is `http://user:pass@shockcollar.local/send/?`  (if
username/password are not defined then skip that part).

Next are the possible commands

    vibrate=1&v_str=100&v_dur=2
    shock=1&s_str=1&s_dur=1
    beep=1&b_dur=1

e.g.

    http://user:pass@shockcollar.local/send/?vibrate=1&v_str=100&v_dur=1
    http://shockcollar.local/send/?shock=1&s_str=1&s_dur=1
