# CED15I031 Akshay

# Computer Networks Lab Assignment 4 - Question 1
# We have learnt in TCP class that the retransmission timout interval is not calculated with the
# ACKs for retransmission messages. However in a lossy network, if most of the packets had to be
# retransmitted, this would prevent the frequent recalculations of the timeout interval which would be
# reflecting the actual network state. Using the NS simulation, implement the enhancement in logic
# for​ ​ this​ ​ rule​ ​ and​ ​ justify​ ​ your​ ​ implementation.

# Declaring variables for use
	set start_transmit1 0.1;	# Time to start transmit for FTP Application1
	set start_transmit2 0.1;	# Time to start transmit for FTP Application2
	set stop_transmit 19.5;		# Time to stop transmit for all FTP Applications
	set quelim 2;				# Queue Limit for the router in between
	set Snd_Rtr_BW 1Mb;			# Bandwidth for the Senders to Router links
	set Rtr_Rcvr_BW 1Mb;		# Bandwidth for the Router to Receiver link
	set Snd_Rtr_Delay 50ms;		# Delay from Sender to Router links (Note: if changing here... also change in line number 68 & 76)
	set Rtr_Rcvr_Delay 20ms;	# Delay from Router to Receiver link
	set QuePosPi 1.0;			# Position of Queue in terms of Pi with Respect to the link orientation

# Create a simulator object
	set net_sim [new Simulator];	# A new object name net_sim for class Simulator

# Define color for data flows (for NAM)
	$net_sim color 1 Blue;		# Color Blue for data from the Agents with flow_id set as 1
	$net_sim color 2 Magenta;	# Color Magenta for data from the Agents with flow_id set as 2
	$net_sim color 3 Red;		# Color Red for data from the Agents with flow_id set as 3
	$net_sim color 4 Green;		# Color Green for data from the Agents with flow_id set as 4

# Open the nam trace file
	set nam_file [open 1_Lossy_TCP_Retransmitter.nam w]; 	# a new file pointer for nam trace
	$net_sim namtrace-all $nam_file;						# record the trace in the associated file with nam_file pointer

# Define a 'Record_n_Play' procedure
	proc Record_n_Play {} {
		global net_sim nam_file
		$net_sim flush-trace;						# write all trace records
		close $nam_file;							# Close the trace file
		exec nam 1_Lossy_TCP_Retransmitter.nam &;	# Execute nam on the trace file
		exit 0;										# exit
	}

# Creating nodes
	set Sender1 [$net_sim node];	# 1st Sender node under the net_sim object
	set Sender2 [$net_sim node];	# 2nd Sender node under the net_sim object
	set Router [$net_sim node];		# Router node under the net_sim object
	set Receiver [$net_sim node];	# Receiver node under the net_sim object

# Create a duplex link between the Sender(s) -- Router -- Receiver because TCP needs duplex
	$net_sim duplex-link $Sender1 $Router $Snd_Rtr_BW $Snd_Rtr_Delay DropTail
	$net_sim duplex-link $Sender2 $Router $Snd_Rtr_BW $Snd_Rtr_Delay DropTail
	$net_sim duplex-link $Router $Receiver $Rtr_Rcvr_BW $Rtr_Rcvr_Delay DropTail

# Give positon of node for NAM
	$net_sim duplex-link-op $Sender1 $Router orient right;		# so that Router appears to the Right of Sender1 in NAM
	$net_sim duplex-link-op $Sender2 $Router orient left;		# so that Router appears to the Left of Sender2 in NAM
	$net_sim duplex-link-op $Router $Receiver orient down;		# so that Receiver appears to the Down of Router in NAM

# Monitor the queue for link (Router-Receiver) (for NAM)
	$net_sim duplex-link-op $Router $Receiver queuePos $QuePosPi;		# the queue is shown in QuePosPi times Pi with the orientation of given link

# Set Queue Size of link (Router-Receiver)
	$net_sim queue-limit $Router $Receiver $quelim;		# quelimit at Router for packets towards Receiver is set to quelim

# Attaching a loss block 1
	set loss_block1 [new ErrorModel];
	$loss_block1 unit pkt;
	$net_sim at [expr $start_transmit1 + 0.051] "$loss_block1 set rate_ 0.5";	# Error rate in fractions
	$loss_block1 ranvar [new RandomVariable/Uniform];
	$loss_block1 drop-target [new Agent/Null];
	$net_sim link-lossmodel $loss_block1 $Sender1 $Router

# Attaching a loss block 2
	set loss_block2 [new ErrorModel];
	$loss_block2 unit pkt;
	$net_sim at [expr $start_transmit2 + 0.051] "$loss_block2 set rate_ 0.5";	# Error rate in fractions
	$loss_block2 ranvar [new RandomVariable/Uniform];
	$loss_block2 drop-target [new Agent/Null];
	$net_sim link-lossmodel $loss_block2 $Sender2 $Router
	
# Create a TCP agent and attach to Sender1 as tcp1 and its sink port on the Receiver
	set tcp1 [new Agent/TCP]
	$tcp1 set fid_ 1;			# Seperate flow IDs for identification and colors
	$net_sim attach-agent $Sender1 $tcp1
	set sink1 [new Agent/TCPSink];		# Seperate Sinks are needed else only the last connection will be maintained (acts like seperate ports on Receiver side)
	$net_sim attach-agent $Receiver $sink1
	$tcp1 set ts_resetRTO false;		# To not reset the RTO for Transport Segment
# Create a TCP agent and attach to Sender2 as tcp2
	set tcp2 [new Agent/TCP]
	$tcp2 set fid_ 2
	$net_sim attach-agent $Sender2 $tcp2
	set sink2 [new Agent/TCPSink]
	$net_sim attach-agent $Receiver $sink2
	$tcp2 set ts_resetRTO_ true;		# To learn and reset the RTO for Transport Segment

# Connect Sender(tcp agent) to Router(tcpsink agent)
	$net_sim connect $tcp1 $sink1
	$net_sim connect $tcp2 $sink2

# Setup a FTP application over all TCP connections
	set ftp1 [new Application/FTP]
	$ftp1 attach-agent $tcp1
	set ftp2 [new Application/FTP]
	$ftp2 attach-agent $tcp2

# Schedule events for the FTP agents
	$net_sim at $start_transmit1 "$ftp1 start"
	$net_sim at $start_transmit2 "$ftp2 start"
	$net_sim at $stop_transmit "$ftp1 stop"
	$net_sim at $stop_transmit "$ftp2 stop"

# Call the Record_n_Play procedure af 500ms of Stop transmit
	$net_sim at [expr $stop_transmit + 0.5] "Record_n_Play"

# Run the simulation object
	$net_sim run