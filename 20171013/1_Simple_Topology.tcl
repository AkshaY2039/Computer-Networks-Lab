# Create a simulator object
set ns [new Simulator]

# Define different colors for data flows (for NAM)
$ns color 2 Green

# Open the nam trace file
set nf [open 1_Simple.nam w]
$ns namtrace-all $nf

# Define a 'ends_simulation' procedure
proc ends_simulation {} {
	global ns nf
	$ns flush-trace
	# Close the trace file
	close $nf
	# Execute nam on the trace file
	exec nam 1_Simple.nam &
	exit 0
}

# Create two nodes
set NodeA [$ns node]
set NodeB [$ns node]

# Create a simplex link between the NodeA and NodeB
$ns simplex-link $NodeA $NodeB 1Mb 100ms DropTail

# Give positon of node for NAM
$ns simplex-link-op $NodeA $NodeB orient right

# Create UDP agent and attach to NodeA
set Sender [new Agent/UDP]
$ns attach-agent $NodeA $Sender

# Create a CBR traffic source and attach to Sender
set cbr [new Application/Traffic/CBR]
$cbr set packetSize_ 1000
$cbr set interval_ 0.05
$cbr attach-agent $Sender

# Setup a Null agent and attach to NodeB
set Receiver [new Agent/Null]
$ns attach-agent $NodeB $Receiver
$ns connect $Sender $Receiver

# Schedule events for the CBR agents
$ns at 1.0 "$cbr start"
$ns at 3.0 "$cbr stop"

# Call the ends_simulation procedure afetr 50sec of Simulation
$ns at 5.0 "ends_simulation"

#Run the simulation
$ns run