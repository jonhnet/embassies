#!/usr/bin/perl
# Create the keys, certs, and cert chains for a vendor, specified by DNS name

$vendor_name = $ARGV[0]; 
$base = $ENV{"HOME"} . "/zoog/toolchains/linux_elf/crypto/";

@names = split("\\.", $vendor_name);
@names = reverse @names;

$parent_key = "$base/keys/root.keys";
#$parent_certs = "$base/certs/root.cert";
$parent_certs = "";

$vendor_name = "";

foreach $name (@names) {
	if ($vendor_name eq "") {
		$vendor_name = $name;
	} else {
		$vendor_name = "$name.$vendor_name";
	}

	$keyfile = "$base/keys/$vendor_name.keys";
	$certfile = "$base/certs/$vendor_name.cert";
	$chainfile = "$base/certs/$vendor_name.chain";

	unless (-e $keyfile) {
		$cmd = "$base/build/crypto_util --genkeys $keyfile --name $vendor_name";
		print "Creating key for: $vendor_name\n";
		print "$cmd\n";
		system($cmd);
	} 

	unless (-e $certfile) {
		$cmd = "$base/build/crypto_util --ekeypair $keyfile --cert $certfile";
		$cmd .= " --name $vendor_name";
		$cmd .= " --skeypair $parent_key";
		$thedate = time();
		$cmd .= " --inception $thedate";
		$cmd .= " --expires " . ($thedate + 31536000);
		print "Creating cert for: $vendor_name\n";
		#print "$cmd\n";
		system($cmd);
	}

	unless (-e $chainfile) {
		$cmd = "$base/build/crypto_util --genchain $chainfile $parent_certs $certfile ";
		print "Creating chain for: $vendor_name\n";
		#print "$cmd\n";
		system($cmd);
	}

	$parent_key = $keyfile;
	$parent_certs = "$parent_certs $certfile";
}

#print "Done!\n";
