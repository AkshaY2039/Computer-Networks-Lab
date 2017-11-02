# CED15I031 Akshay

# Computer Networks Lab Assignment 4 - Question 2
# Implement a script that is capable of generating a typical characteristic traffic both on an
# incoming and outgoing link. The NS script can make use of an existing library for the same in
# which the traffic could be “originally learnt”. The traffic simulated should pertain to a MSS size of
# 1200​ ​ bytes​ ​ and​ ​ also​ ​ disabling​ ​ Nagle’s​ ​ algorithm.

# Declaring variables
	set EndTransmit 7;			# length of traced simulation (s)
	set Incoming "2_in.cvec";	# Connection Vector file for Incoming Link
	set Outgoing "2_out.cvec";	# Connection Vector file for Outgoing Link

# Setup Simulator object
	remove-all-packet-headers;	# removes all packet headers
	add-packet-header IP TCP;	# adds TCP/IP headers
	set net_sim [new Simulator];

# Open the nam trace file
	set nam_file [open 2_Custom_Traffic.nam w]
	$net_sim namtrace-all $nam_file

# Define a 'finish' procedure
	proc finish {} {
		global net_sim nam_file
		$net_sim flush-trace;	# Close the trace file
		close $nam_file;		# Execute nam on the trace file
		exec nam 2_Custom_Traffic.nam &
		exit 0
	}

# create nodes
	set n(0) [$net_sim node]
	set n(1) [$net_sim node]
	set n(2) [$net_sim node]
	set n(3) [$net_sim node]

	$net_sim color 7 Red
	$net_sim color 8 Blue
	$net_sim color 5 Green

# create Tmix_DelayBox nodes
	set tmixNet(0) [$net_sim Tmix_DelayBox]
	$tmixNet(0) set-cvfile "$Incoming" [$n(0) id] [$n(1) id];	# set the Outgoing Connection Vector File between node2 node 2
	$tmixNet(0) set-lossless
	set tmixNet(1) [$net_sim Tmix_DelayBox]
	$tmixNet(1) set-cvfile "$Outgoing" [$n(3) id] [$n(2) id];	# set the Outgoing Connection Vector File between node2 node 2
	$tmixNet(1) set-lossless

# create links
	$net_sim duplex-link $n(0) $tmixNet(0) 1Mb 10ms DropTail
	$net_sim duplex-link $n(2) $tmixNet(0) 1Mb 10ms DropTail
	$net_sim duplex-link $tmixNet(0) $tmixNet(1) 2Mb 100ms DropTail
	$net_sim duplex-link $tmixNet(1) $n(1) 1Mb 10ms DropTail
	$net_sim duplex-link $tmixNet(1) $n(3) 1Mb 10ms DropTail

# orient link positions for NAM
	$net_sim duplex-link-op $n(0) $tmixNet(0) orient right-down
	$net_sim duplex-link-op $n(2) $tmixNet(0) orient right-up
	$net_sim duplex-link-op $tmixNet(0) $tmixNet(1) orient right
	$net_sim duplex-link-op $tmixNet(1) $n(1) orient right-up
	$net_sim duplex-link-op $tmixNet(1) $n(3) orient right-down

# Setting up TCP Agent
	Agent/TCP/FullTcp set segsize_ 1200;	# set MSS to 1200 bytes
	Agent/TCP/FullTcp set nodelay_ true;	# disabling nagle
	Agent/TCP/FullTcp set segsperack_ 2;	# delayed ACKs
	Agent/TCP/FullTcp set interval_ 0.1;	# 100 ms
	Agent/TCP/FullTcp set fid_ 5;			# so that TCP segments are identified

# Setting up TMIX
	set tmix(0) [new Tmix]
	$tmix(0) set-init $n(0);	# name $n(0) as initiator
	$tmix(0) set-acc $n(1);		# name $n(1) as acceptor
	$tmix(0) set-ID 7
	$tmix(0) set-cvfile "$Incoming"

	set tmix(1) [new Tmix]
	$tmix(1) set-init $n(3);	# name $n(3) as initiator
	$tmix(1) set-acc $n(2);		# name $n(2) as acceptor
	$tmix(1) set-ID 8
	$tmix(1) set-cvfile "$Outgoing"

# Schedule the TMIX events
	$net_sim at 0.0 "$tmix(0) start"
	$net_sim at 0.0 "$tmix(1) start"
	$net_sim at $EndTransmit "$tmix(0) stop"
	$net_sim at $EndTransmit "$tmix(1) stop"

# Call the finish procedure af 500ms of End Transmit
	$net_sim at [expr $EndTransmit + 0.5] "finish"

# Run the simulation
	$net_sim run