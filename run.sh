#!/bin/sh

find_linux_proc_banner() {
	$2 sed -n -re 's/^([0-9a-f]*[1-9a-f][0-9a-f]*) .* linux_proc_banner$/\1/p' $1
}

echo "looking for linux_proc_banner in /proc/kallsyms"
linux_proc_banner=$(find_linux_proc_banner /proc/kallsyms)
if test -z $linux_proc_banner; then
	echo "protected. requires root"
	set -x
	linux_proc_banner=$(\
		find_linux_proc_banner /proc/kallsyms sudo)

	set +x
fi
if test -z $linux_proc_banner; then
	echo "not found. reading /boot/System.map-$(uname -r)"
	set -x
	linux_proc_banner=$(\
		find_linux_proc_banner /boot/System.map-$(uname -r) sudo)
	set +x
fi
if test -z $linux_proc_banner; then
	echo "not found. reading /lib/modules/$(uname -r)/build/System.map"
	set -x
	linux_proc_banner=$(\
		find_linux_proc_banner /lib/modules/$(uname -r)/build/System.map sudo)
	set +x
fi
if test -z $linux_proc_banner; then
	echo "not found. reading /boot/System.map"
	set -x
	linux_proc_banner=$(\
		find_linux_proc_banner /boot/System.map sudo)
	set +x
fi
if test -z $linux_proc_banner; then
	echo "can't find linux_proc_banner, unable to test at all"
	exit 0
fi

if [ ! -e ./scsb ]; then
	echo "'scsb' program not found, did you forgot to run 'make' ?"
	exit 0
fi
./scsb $linux_proc_banner 20
vuln=$?
