# Create a simulator object
set ns [new Simulator]

# Define different colors for data flows (for NAM)
$ns color 1 Green
$ns color 2 Blue

# Open the NAM trace file
set nf [open 3_Star_5nodes.nam w]
$ns namtrace-all $nf

# Define a 'end_simulation' procedure
proc end_simulation {} {
	global ns nf
	$ns flush-trace
	# Close the NAM trace file
	close $nf
	# Execute NAM on the trace file
	exec nam 3_Star_5nodes.nam &
	exit 0
}

# Create four nodes
set Sender1 [$ns node]
set Sender2 [$ns node]
set Sender3 [$ns node]
set Router [$ns node]
set Receiver [$ns node]

# Create links between the nodes
$ns duplex-link $Sender1 $Router 2Mb 100ms DropTail
$ns duplex-link $Sender2 $Router 3Mb 100ms DropTail
$ns duplex-link $Sender3 $Router 1Mb 100ms DropTail
$ns duplex-link $Router $Receiver 2Mb 80ms DropTail

# Set Queue Size of link (Router-Receiver)
$ns queue-limit $Router $Receiver 5

# Give node position (for NAM)
$ns duplex-link-op $Sender1 $Router orient right-down
$ns duplex-link-op $Sender2 $Router orient right-up
$ns duplex-link-op $Sender3 $Router orient right
$ns duplex-link-op $Router $Receiver orient right

# Monitor the queue for link (Router-Receiver) (for NAM)
$ns duplex-link-op $Router $Receiver queuePos 0.5

#Setup a TCP connection
set tcp [new Agent/TCP]
$tcp set class_ 2
$ns attach-agent $Sender3 $tcp
set sink [new Agent/TCPSink]
$ns attach-agent $Receiver $sink
$ns connect $tcp $sink
$tcp set fid_ 1

#Setup a FTP over TCP connection
set ftp [new Application/FTP]
$ftp attach-agent $tcp
$ftp set type_ FTP

# Setup a UDP connection
set udp1 [new Agent/UDP]
$ns attach-agent $Sender1 $udp1
set udp2 [new Agent/UDP]
$ns attach-agent $Sender2 $udp2
set null [new Agent/Null]
$ns attach-agent $Receiver $null
$ns connect $udp1 $null
$ns connect $udp2 $null
$udp1 set fid_ 2

# Setup a CBR over UDP connection
set cbr1 [new Application/Traffic/CBR]
$cbr1 attach-agent $udp1
$cbr1 set type_ CBR
$cbr1 set packet_size_ 1000
$cbr1 set rate_ 1mb
$cbr1 set random_ false
set cbr2 [new Application/Traffic/CBR]
$cbr2 attach-agent $udp2
$cbr2 set type_ CBR
$cbr2 set packet_size_ 1000
$cbr2 set rate_ 1mb
$cbr2 set random_ false

#Detach tcp and sink agents (not really necessary)
$ns at 5.5 "$ns detach-agent $Sender3 $tcp ; $ns detach-agent $Router $sink"

# Schedule events for the CBR
$ns at 0.01 "$cbr1 start"
$ns at 0.015 "$cbr2 start"
$ns at 0.1 "$ftp start"
$ns at 4.3 "$cbr1 stop"
$ns at 4.0 "$ftp stop"
$ns at 4.5 "$cbr2 stop"

# Call the end_simulation procedure after 50 seconds of simulation time
$ns at 5.0 "end_simulation"

# Run the simulation
$ns run