# Create a simulator object
set ns [new Simulator]

# Define different colors for data flows (for NAM)
$ns color 2 Blue

# Open the NAM trace file
set nf [open 2_Star.nam w]
$ns namtrace-all $nf

# Define a 'end_simulation' procedure
proc end_simulation {} {
	global ns nf
	$ns flush-trace
	# Close the NAM trace file
	close $nf
	# Execute NAM on the trace file
	exec nam 2_Star.nam &
	exit 0
}

# Create four nodes
set Sender1 [$ns node]
set Sender2 [$ns node]
set Router [$ns node]
set Receiver [$ns node]

# Create links between the nodes
$ns duplex-link $Sender1 $Router 2Mb 10ms DropTail
$ns duplex-link $Sender2 $Router 1Mb 20ms DropTail
$ns duplex-link $Router $Receiver 2Mb 10ms DropTail

# Give node position (for NAM)
$ns duplex-link-op $Sender1 $Router orient right-down
$ns duplex-link-op $Sender2 $Router orient right-up
$ns duplex-link-op $Router $Receiver orient right

# Monitor the queue for link (Router-Receiver) (for NAM)
$ns duplex-link-op $Router $Receiver queuePos 0.5

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

# Schedule events for the CBR
$ns at 0.1 "$cbr1 start"
$ns at 0.15 "$cbr2 start"
$ns at 0.55 "$cbr1 stop"
$ns at 0.6 "$cbr2 stop"

# Call the end_simulation procedure after 7 seconds of simulation time
$ns at 0.7 "end_simulation"

# Run the simulation
$ns run