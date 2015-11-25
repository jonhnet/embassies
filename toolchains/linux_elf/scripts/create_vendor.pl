#!/usr/bin/perl
# Create the keys, certs, and cert chains for a vendor, specified by DNS name

$vendor_name = $ARGV[0]; 
use Cwd 'abs_path';
use File::Basename;
$base = dirname(abs_path($0)) . "/../../../toolchains/linux_elf/crypto/";

@names = split("\\.", $vendor_name);
@names = reverse @names;

$parent_key = "$base/keys/root.keys";
#$parent_certs = "$base/certs/root.cert";
$parent_certs = "";

$vendor_name = "";

sub safe_system {
	#print "safe_system: $cmd\n";
	$rc = system(@_);
	if ($rc != 0) {
		print "Cmd failed:\n$cmd\n";
		exit -1;
	}
}

foreach $name (@names) {
	if ($vendor_name eq "") {
		$vendor_name = $name;
	} else {
		$vendor_name = "$name.$vendor_name";
	}

	$keyfile = "$base/keys/$vendor_name.keys";
	$certfile = "$base/certs/$vendor_name.cert";
	$chainfile = "$base/certs/$vendor_name.chain";

	print "keyfile $keyfile\n";
	unless (-e $keyfile) {
		$cmd = "$base/build/crypto_util --genkeys $keyfile --name $vendor_name";
		print "Creating key for: $vendor_name\n";
		safe_system($cmd);
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
		safe_system($cmd);
	}

	unless (-e $chainfile) {
		$cmd = "$base/build/crypto_util --genchain $chainfile $parent_certs $certfile ";
		print "Creating chain for: $vendor_name\n";
		#print "$cmd\n";
		safe_system($cmd);
	}

	$parent_key = $keyfile;
	$parent_certs = "$parent_certs $certfile";
}

#print "Done!\n";
