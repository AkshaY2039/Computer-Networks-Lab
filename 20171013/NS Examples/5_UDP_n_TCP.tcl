# Create a simulator object
set ns [new Simulator]

#Define different colors for data flows (for NAM)
$ns color 1 Blue

# Open the nam trace file
set nf [open 5_UDP_n_TCP.nam w]
$ns namtrace-all $nf

# Define a 'finish' procedure
proc finish {} {	
	global ns nf
	$ns flush-trace
	# Close the trace file
	close $nf
	# Execute nam on the trace file
	exec nam 5_UDP_n_TCP.nam &
	exit 0
}

# Create two nodes
set NodeA [$ns node]
set NodeB [$ns node]
set NodeC [$ns node]
set NodeD [$ns node]

# Create a duplex link between the NodeA and NodeB because TCP needs duplex
$ns duplex-link $NodeA $NodeB 1Mb 100ms DropTail

# Create a simplex link between the NodeC and NodeD
$ns simplex-link $NodeC $NodeD 1Mb 100ms DropTail

# Give positon of node for NAM
$ns duplex-link-op $NodeA $NodeB orient right

# Create a TCP agent and attach to NodeA as Sender
set tcp [new Agent/TCP]
$tcp set class_ 2
$ns attach-agent $NodeA $tcp

# Create a TCP Sink agent and attach to NodeB as Receiver
set sink [new Agent/TCPSink]
$ns attach-agent $NodeB $sink
$tcp set fid_ 1

# Connect NodeA(tcp agent) to NodeB(tcpsink agent)
$ns connect $tcp $sink

# Setup a FTP application over TCP connection
set ftp [new Application/FTP]
$ftp attach-agent $tcp
$ftp set type_ FTP

# Schedule events for the FTP agents
$ns at 1.0 "$ftp start"
$ns at 3.0 "$ftp stop"

#Detach tcp and sink agents (not really necessary)
$ns at 4.5 "$ns detach-agent $NodeA $tcp ; $ns detach-agent $NodeB $sink"

# Call the finish procedure afetr 5sec of Simulation
$ns at 5.0 "finish"

#Run the simulation
$ns run