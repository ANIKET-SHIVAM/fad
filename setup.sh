#!/bin/sh

#compile and save program that will run as a service in /usr/sbin

sudo g++ -std=c++11 -pthread fad.cpp -o /usr/sbin/fad

#move daemon script to /etc/init.d

sudo cp fad /etc/init.d/fad

#give this script executable permission

sudo chmod 775 /etc/init.d/fad

#starting the daemon

sudo /etc/init.d/fad start

#starting the daemon at startup

#sudo update-rc.d /etc/init.d/fad defaults
