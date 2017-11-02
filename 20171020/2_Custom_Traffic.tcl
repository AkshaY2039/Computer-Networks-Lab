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

# Define a 'Record_n_Play' procedure
	proc Record_n_Play {} {
		global net_sim nam_file
		$net_sim flush-trace;	# Close the trace file
		close $nam_file;		# Execute nam on the trace file
		exec nam 2_Custom_Traffic.nam &
		exit 0
	}

# create nodes
	set Node0 [$net_sim node]
	set Node1 [$net_sim node]
	set Node2 [$net_sim node]
	set Node3 [$net_sim node]

# data flow colors for identification of tmix1, tmix2 and TCP segments
	$net_sim color 7 Red
	$net_sim color 8 Blue
	$net_sim color 5 Green

# create Tmix_DelayBox nodes
	set TMIX_DB1 [$net_sim Tmix_DelayBox]
	$TMIX_DB1 set-cvfile "$Incoming" [$Node0 id] [$Node1 id];	# set the Outgoing Connection Vector File between node2 node 2
	$TMIX_DB1 set-lossless
	set TMIX_DB2 [$net_sim Tmix_DelayBox]
	$TMIX_DB2 set-cvfile "$Outgoing" [$Node3 id] [$Node2 id];	# set the Outgoing Connection Vector File between node2 node 2
	$TMIX_DB2 set-lossless

# create links
	$net_sim duplex-link $Node0 $TMIX_DB1 1Mb 10ms DropTail
	$net_sim duplex-link $Node2 $TMIX_DB1 1Mb 10ms DropTail
	$net_sim duplex-link $TMIX_DB1 $TMIX_DB2 2Mb 100ms DropTail
	$net_sim duplex-link $TMIX_DB2 $Node1 1Mb 10ms DropTail
	$net_sim duplex-link $TMIX_DB2 $Node3 1Mb 10ms DropTail

# orient link positions for NAM
	$net_sim duplex-link-op $Node0 $TMIX_DB1 orient right-down
	$net_sim duplex-link-op $Node2 $TMIX_DB1 orient right-up
	$net_sim duplex-link-op $TMIX_DB1 $TMIX_DB2 orient right
	$net_sim duplex-link-op $TMIX_DB2 $Node1 orient right-up
	$net_sim duplex-link-op $TMIX_DB2 $Node3 orient right-down

# Setting up TCP Agent
	Agent/TCP/FullTcp set segsize_ 1200;	# set MSS to 1200 bytes
	Agent/TCP/FullTcp set nodelay_ true;	# disabling nagle
	Agent/TCP/FullTcp set segsperack_ 2;	# delayed ACKs
	Agent/TCP/FullTcp set interval_ 0.1;	# 100 ms
	Agent/TCP/FullTcp set fid_ 5;			# so that TCP segments are identified

# Setting up TMIX
	set TMIX_APP1 [new Tmix]
	$TMIX_APP1 set-init $Node0;	# name $Node0 as initiator
	$TMIX_APP1 set-acc $Node1;		# name $Node1 as acceptor
	$TMIX_APP1 set-ID 7
	$TMIX_APP1 set-cvfile "$Incoming"

	set TMIX_APP2 [new Tmix]
	$TMIX_APP2 set-init $Node3;	# name $Node3 as initiator
	$TMIX_APP2 set-acc $Node2;		# name $Node2 as acceptor
	$TMIX_APP2 set-ID 8
	$TMIX_APP2 set-cvfile "$Outgoing"

# Schedule the TMIX events
	$net_sim at 0.0 "$TMIX_APP1 start"
	$net_sim at 0.0 "$TMIX_APP2 start"
	$net_sim at $EndTransmit "$TMIX_APP1 stop"
	$net_sim at $EndTransmit "$TMIX_APP2 stop"

# Call the Record_n_Play procedure af 500ms of End Transmit
	$net_sim at [expr $EndTransmit + 0.5] "Record_n_Play"

# Run the simulation
	$net_sim run