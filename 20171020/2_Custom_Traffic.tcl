# CED15I031 Akshay

# Computer Networks Lab Assignment 4 - Question 2
# Implement a script that is capable of generating a typical characteristic traffic both on an
# incoming and outgoing link. The NS script can make use of an existing library for the same in
# which the traffic could be “originally learnt”. The traffic simulated should pertain to a MSS size of
# 1200​ ​ bytes​ ​ and​ ​ also​ ​ disabling​ ​ Nagle’s​ ​ algorithm.

#::::::::::: Useful Variables ::::::::::::::::::::::
set end 7;
# length of traced simulation (s)
set Incoming "2_incoming.cvec"
set Outgoing "2_incoming.cvec"
#::::::::::: Setup Simulator ::::::::::::::::::::::
remove-all-packet-headers;
# removes all packet headers
add-packet-header IP TCP;
# adds TCP/IP headers
set ns [new Simulator];

# Open the nam trace file
set nf [open 2_Custom_Traffic.nam w]
$ns namtrace-all $nf

# Define a 'finish' procedure
proc finish {} {
	global ns nf
	$ns flush-trace
	# Close the trace file
	close $nf
	# Execute nam on the trace file
	exec nam 2_Custom_Traffic.nam &
	exit 0
}

# instantiate the Simulator
$ns use-scheduler Heap
#::::::::::: Setup Topology ::::::::::::::::::::::
# create nodes
set n(0) [$ns node]
set n(1) [$ns node]
set n(2) [$ns node]
set n(3) [$ns node]
# create Tmix_DelayBox nodes
set tmixNet(0) [$ns Tmix_DelayBox]
$tmixNet(0) set-cvfile "$Incoming" [$n(0) id] [$n(1) id]
$tmixNet(0) set-lossless
set tmixNet(1) [$ns Tmix_DelayBox]
$tmixNet(1) set-cvfile "$Outgoing" [$n(3) id] [$n(2) id]
$tmixNet(1) set-lossless
# create links
$ns duplex-link $n(0) $tmixNet(0) 1000Mb 10ms DropTail
$ns duplex-link $n(2) $tmixNet(0) 1000Mb 10ms DropTail
$ns duplex-link $tmixNet(0) $tmixNet(1) 1000Mb 10ms DropTail
$ns duplex-link $tmixNet(1) $n(1) 1000Mb 10ms DropTail
$ns duplex-link $tmixNet(1) $n(3) 1000Mb 10ms DropTail

# orient link positions for NAM
$ns duplex-link-op $n(0) $tmixNet(0) orient right-down
$ns duplex-link-op $n(2) $tmixNet(0) orient right-up
$ns duplex-link-op $tmixNet(0) $tmixNet(1) orient right
$ns duplex-link-op $tmixNet(1) $n(1) orient right-up
$ns duplex-link-op $tmixNet(1) $n(3) orient right-down

# set queue buffer sizes (in packets) (default is 20 packets)
$ns queue-limit $n(0) $tmixNet(0) 500
$ns queue-limit $tmixNet(0) $n(0) 500
$ns queue-limit $n(2) $tmixNet(0) 500
$ns queue-limit $tmixNet(0) $n(2) 500
$ns queue-limit $tmixNet(0) $tmixNet(1) 500
$ns queue-limit $tmixNet(1) $tmixNet(0) 500
$ns queue-limit $tmixNet(1) $n(1) 500
$ns queue-limit $n(1) $tmixNet(1) 500
$ns queue-limit $tmixNet(1) $n(3) 500
$ns queue-limit $n(3) $tmixNet(1) 500
#::::::::::: Setup TCP ::::::::::::::::::::::
Agent/TCP/FullTcp set segsize_ 1200;
# set MSS to 1200 bytes
Agent/TCP/FullTcp set nodelay_ true;
Agent/TCP/FullTcp set segsperack_ 2;
Agent/TCP/FullTcp set interval_ 0.1;
# disabling nagle
# delayed ACKs
# 100 ms
#::::::::::: Setup Tmix ::::::::::::::::::::::
set tmix(0) [new Tmix]
$tmix(0) set-init $n(0);
# name $n(0) as initiator
$tmix(0) set-acc $n(1);
# name $n(1) as acceptor
$tmix(0) set-ID 7
$tmix(0) set-cvfile "$Incoming"
set tmix(1) [new Tmix]
$tmix(1) set-init $n(3);
$tmix(1) set-acc $n(2);
$tmix(1) set-ID 8
$tmix(1) set-cvfile "$Outgoing"
# name $n(3) as initiator
# name $n(2) as acceptor
#::::::::::: Setup Schedule ::::::::::::::::::::::
$ns at 0.0 "$tmix(0) start"
$ns at 0.0 "$tmix(1) start"
$ns at $end "$tmix(0) stop"
$ns at $end "$tmix(1) stop"
#$ns at [expr $end + 1] "$ns halt"
$ns at [expr $end + 1] "finish"
$ns run