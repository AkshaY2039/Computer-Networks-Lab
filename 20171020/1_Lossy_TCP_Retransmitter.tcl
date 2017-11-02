# CED15I031 Akshay

# Computer Networks Lab Assignment 4 - Question 1
# We have learnt in TCP class that the retransmission timout interval is not calculated with the
# ACKs for retransmission messages. However in a lossy network, if most of the packets had to be
# retransmitted, this would prevent the frequent recalculations of the timeout interval which would be
# reflecting the actual network state. Using the NS simulation, implement the enhancement in logic
# for​ ​ this​ ​ rule​ ​ and​ ​ justify​ ​ your​ ​ implementation.

# Declaring variables for use
set start_transmit1 0.1
set start_transmit2 0.1
set start_transmit3 0.1
set start_transmit4 0.1
set stop_transmit 19.5
set quelim 4
set Snd_Rtr_BW 1Mb
set Rtr_Rcvr_BW 2Mb
set Snd_Rtr_Delay 50ms
set Rtr_Rcvr_Delay 20ms

# Create a simulator object
set ns [new Simulator]

# Define color for data flows (for NAM)
$ns color 1 Blue
$ns color 2 Magenta
$ns color 3 Red
$ns color 4 Green

# Open the nam trace file
set nf [open 1_Lossy_TCP_Retransmitter.nam w]
$ns namtrace-all $nf

# Define a 'finish' procedure
proc finish {} {
	global ns nf
	$ns flush-trace
	# Close the trace file
	close $nf
	# Execute nam on the trace file
	exec nam 1_Lossy_TCP_Retransmitter.nam &
	exit 0
}

# Create two nodes
set Sender1 [$ns node]
set Sender2 [$ns node]
set Sender3 [$ns node]
set Sender4 [$ns node]
set Router [$ns node]
set Receiver [$ns node]

# Create a duplex link between the Sender(s) -- Router -- Receiver because TCP needs duplex
$ns duplex-link $Sender1 $Router $Snd_Rtr_BW $Snd_Rtr_Delay DropTail
$ns duplex-link $Sender2 $Router $Snd_Rtr_BW $Snd_Rtr_Delay DropTail
$ns duplex-link $Sender3 $Router $Snd_Rtr_BW $Snd_Rtr_Delay DropTail
$ns duplex-link $Sender4 $Router $Snd_Rtr_BW $Snd_Rtr_Delay DropTail
$ns duplex-link $Router $Receiver $Rtr_Rcvr_BW $Rtr_Rcvr_Delay DropTail

# Give positon of node for NAM
$ns duplex-link-op $Sender1 $Router orient right
$ns duplex-link-op $Sender2 $Router orient right-down
$ns duplex-link-op $Sender3 $Router orient left-down
$ns duplex-link-op $Sender4 $Router orient left
$ns duplex-link-op $Router $Receiver orient down

# Monitor the queue for link (Router-Receiver) (for NAM)
$ns duplex-link-op $Router $Receiver queuePos 1.0

# Set Queue Size of link (Router-Receiver)
$ns queue-limit $Router $Receiver $quelim

# Create a TCP agent and attach to Sender1 as tcp1
set tcp1 [new Agent/TCP]
$tcp1 set bugFix_ true
# To fix bugs in ACK
$tcp1 set bugFix_ack_ true
# To fix the bugs of Transport Segment
$tcp1 set bugFix_ts_ true
# To learn and reset the RTO for Transport Segment
$tcp1 set ts_resetRTO_ true
$tcp1 set class_ 1
$tcp1 set fid_ 1
$ns attach-agent $Sender1 $tcp1

# Create a TCP agent and attach to Sender 2,3,4 as tcp 2,3,4
set tcp3 [new Agent/TCP]
$tcp3 set bugFix_ true
$tcp3 set bugFix_ack_ true
$tcp3 set bugFix_ts_ true
$tcp3 set ts_resetRTO_ true
$tcp3 set class_ 3
$tcp3 set fid_ 3
$ns attach-agent $Sender3 $tcp3
set tcp2 [new Agent/TCP]
$tcp3 set bugFix_ true
$tcp3 set bugFix_ack_ true
$tcp3 set bugFix_ts_ true
$tcp3 set ts_resetRTO_ true
$tcp2 set class_ 2
$tcp2 set fid_ 2
$ns attach-agent $Sender2 $tcp2
set tcp4 [new Agent/TCP]
$tcp3 set bugFix_ true
$tcp3 set bugFix_ack_ true
$tcp3 set bugFix_ts_ true
$tcp3 set ts_resetRTO_ true
$tcp4 set class_ 4
$tcp4 set fid_ 4
$ns attach-agent $Sender4 $tcp4

# Create a TCP Sink agent and attach to Receiver as sink s1,2,3,4
set sink1 [new Agent/TCPSink]
$ns attach-agent $Receiver $sink1
set sink2 [new Agent/TCPSink]
$ns attach-agent $Receiver $sink2
set sink3 [new Agent/TCPSink]
$ns attach-agent $Receiver $sink3
set sink4 [new Agent/TCPSink]
$ns attach-agent $Receiver $sink4

# Connect Sender(tcp agent) to Router(tcpsink agent)
$ns connect $tcp1 $sink1
$ns connect $tcp2 $sink2
$ns connect $tcp3 $sink3
$ns connect $tcp4 $sink4

# Setup a FTP application over TCP connection
set ftp1 [new Application/FTP]
$ftp1 attach-agent $tcp1
$ftp1 set type_ FTP
set ftp2 [new Application/FTP]
$ftp2 attach-agent $tcp2
$ftp2 set type_ FTP
set ftp3 [new Application/FTP]
$ftp3 attach-agent $tcp3
$ftp3 set type_ FTP
set ftp4 [new Application/FTP]
$ftp4 attach-agent $tcp4
$ftp4 set type_ FTP

# Schedule events for the FTP agents
$ns at $start_transmit1 "$ftp1 start"
$ns at $start_transmit2 "$ftp2 start"
$ns at $start_transmit3 "$ftp3 start"
$ns at $start_transmit4 "$ftp4 start"
$ns at $stop_transmit "$ftp1 stop"
$ns at $stop_transmit "$ftp2 stop"
$ns at $stop_transmit "$ftp3 stop"
$ns at $stop_transmit "$ftp4 stop"

# Call the finish procedure afetr 5sec of Simulation
$ns at [expr $stop_transmit + 0.5] "finish"

#Run the simulation
$ns run