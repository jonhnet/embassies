#!/bin/bash

if [ ! -e keys/root.keys ]; then
	echo "No root keys defined. I don't want to create new ones automatically,"
	echo "to avoid a broken script stomping your keys and certs."
	echo "If this is a fresh Embassies environment and you want certs created, "
	echo "please type 'yes'"
	read answer
	if [ X$answer == X'yes' ]; then
		echo "Generating keys and certs..."
	else
		echo "Quitting without keys present."
		exit 1
	fi
else
	echo Keys already present
	exit 0
fi

NAMES="com gnucash.org graphics.net maps.net marble.maps.net net office.org org raster.graphics.net results.search.com retailer.com search.com spreadsheet.office.org vector.graphics.net vendor-a vendor-b widgets.retailer.com wordprocessor.office.org zftp-zoog craigslist ebay microsoft reddit craigslist-warm ebay-warm microsoft-warm reddit-warm cpuid-test"

# Rebuild the root
./build/crypto_util --genkeys ./keys/root.keys --name " "

for n in $NAMES; do ../scripts/create_vendor.pl $n; done;
