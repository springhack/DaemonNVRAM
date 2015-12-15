#!/bin/sh

install()
{
	echo 'Install starting...'
	mkdir -p /Extra/NVRAM/
	cp -rvf DaemonNVRAM.kext /Extra/NVRAM/
	cp -rvf load /Extra/NVRAM/
	cp -rvf com.springhack.DaemonNVRAM.plist /Library/LaunchDaemons/
	cp -rvf com.springhack.DaemonNVRAM.Agent.plist /Library/LaunchAgents/
	chmod 755 /Library/LaunchDaemons/com.springhack.DaemonNVRAM.plist
	chown 0:0 /Library/LaunchDaemons/com.springhack.DaemonNVRAM.plist
	chmod 755 /Library/LaunchAgents/com.springhack.DaemonNVRAM.Agent.plist
	chown 0:0 /Library/LaunchAgents/com.springhack.DaemonNVRAM.Agent.plist
	chown 0:0 /Extra/NVRAM/load
	chmod 755 /Extra/NVRAM/load
	chmod u+s /Extra/NVRAM/load
	kextload /Extra/NVRAM/DaemonNVRAM.kext
	echo 'Installed.'
}

uninstall()
{
	echo 'Uninstall starting...'
	rm -rvf /Extra/NVRAM
	rm -rvf /Library/LaunchDaemons/com.springhack.DaemonNVRAM.plist
	rm -rvf /Library/LaunchAgents/com.springhack.DaemonNVRAM.Agent.plist
	echo 'Uninstalled.'
}

help()
{
	echo 'DaemonNVRAM module for Hackintosh'
	echo 'Create by SpringHack'
	echo 'https://github.com/springhack/DaemonNVRAM.git'
	echo '==> "install.sh install" to install'
	echo '==> "install.sh uninstall" to uninstall'
}

$1
