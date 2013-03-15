#!/bin/bash

NAMES="com gnucash.org graphics.net maps.net marble.maps.net net office.org org raster.graphics.net results.search.com retailer.com search.com spreadsheet.office.org vector.graphics.net vendor-a vendor-b widgets.retailer.com wordprocessor.office.org zftp-zoog"

# Rebuild the root
./build/crypto_util --genkeys ./keys/root.keys --name " "

for n in $NAMES; do ../scripts/create_vendor.pl $n; done;
