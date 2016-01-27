#!/bin/sh

#compile and save program that will run as a service in /usr/sbin

sudo gcc fad.cpp -o /usr/sbin/fad

#move daemon script to /etc/init.d

sudo cp fad /etc/init.d/fad

#give this script executable permission

sudo chmod 775 /etc/init.d/fad

#starting the daemon

sudo /etc/init.d/fad start