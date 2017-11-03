# CED15I031 Akshay

# Computer Networks Lab Assignment 4 - Question 3
# A topology​ is​ supposed​ ​ to​ ​ extrapolate​ ​ the​ ​ following​ ​ parameters:
# ---> Access-link​ ​ bandwidths​ ​ – ​ ​ 1000​ ​ Mbps
# ---> Access-link​ ​ delays​ ​ –[10ms,100ms]​ ​ ,40ms​ ​ spacing
# ---> Flow-level​ ​ load:​ ​ 0.8
# ---> Bottleneck-link​ ​ bandwidth​ ​ – ​ ​ 20​ ​ Mbps
# ---> Bottleneck-link​ ​ delay​ ​ – ​ ​ 5 ​ ​ ms
# ---> Queue​ ​ limits:
# ​ ​ ​ ​ ​ ​ ​ ​ Bottleneck​ ​ queue:1500​ ​ packets
# ​ ​ ​ ​ ​ ​ ​ Access​ ​ queue:1500​ ​ packets
# ---> Packet​ ​ loss​ ​ in​ ​ the​ ​ loss​ ​ module,​ ​ p ​ : ​ ​ [0.1,​ ​ 5]​ ​ %
# In the above scenario, simulated transfer time and analytical flow transfer time are to be
# compared.(Use​ ​ NS2).
# Equal loss rates for different RTT classes are extracted by introducing separate loss module in the
# bottleneck​ ​ link.The artificial loss module in NS-2 generates losses randomly in the bottleneck link queue.The
# packets​ ​ in​ ​ the​ ​ queue​ ​ are​ ​ dropped​ ​ with​ ​ equal​ ​ probability,p.
# Compare​ ​ the​ ​ simulated​ ​ and​ ​ theoretical​ ​ model.
# Hint:
# ​ ​ ​ ​ Inter-arrival​ ​ time:0.8
# ​ ​ ​ ​ Introduce​ ​ for​ ​ RTT​ ​ classes
# ​ ​ ​ ​ The​ ​ topology​ ​ followed​ ​ is​ ​ an​ ​ isomorphic​ ​ class​ ​ of​ ​ trees​ ​ with​ ​ 6 ​ ​ nodes,out​ ​ of​ ​ which​ ​ 4 ​ ​ are​ ​ leaves.

# Declaring variables for use
	set start_transmit1 0.1;		# Time to start transmit for FTP Application1
	set start_transmit2 0.1;		# Time to start transmit for FTP Application2
	set start_transmit3 0.1;		# Time to start transmit for FTP Application3
	set start_transmit4 0.1;		# Time to start transmit for FTP Application4
	set stop_transmit 19.5;			# Time to stop transmit for all FTP Applications
	set quelim 1500;				# Queue Limit for the router in between
	set Access_Link_BW 1000Mb;		# Bandwidth for the AccessHosts to Router links
	set BottleNeck_BW 20Mb;			# Bandwidth for the Router to FinalDest link
	set Access_Link_Delay1 [expr { 10 + floor( rand() * 91 )}]ms;		# Delay from AccessHost1 to Router link
	set Access_Link_Delay2 [expr { 10 + floor( rand() * 91 )}]ms;		# Delay from AccessHost2 to Router link
	set Access_Link_Delay3 [expr { 10 + floor( rand() * 91 )}]ms;		# Delay from AccessHost3 to Router link
	set Access_Link_Delay4 [expr { 10 + floor( rand() * 91 )}]ms;		# Delay from AccessHost4 to Router link
	set BottleNeck_Delay 5ms;		# Delay from Router to FinalDest link
	set QuePosPi 1.0;				# Position of Queue in terms of Pi with Respect to the link orientation
	set error_rate [expr {0.001 + rand() * 0.05}];	# error rate between 0.1% and 5%

# Create a simulator object
	set net_sim [new Simulator];	# A new object name net_sim for class Simulator

# Define color for data flows (for NAM)
	$net_sim color 1 Blue;		# Color Blue for data from the Agents with flow_id set as 1
	$net_sim color 2 Magenta;	# Color Magenta for data from the Agents with flow_id set as 2
	$net_sim color 3 Red;		# Color Red for data from the Agents with flow_id set as 3
	$net_sim color 4 Green;		# Color Green for data from the Agents with flow_id set as 4

# Open the nam trace file
	set nam_file [open 3BN.nam w]; 		# a new file pointer for nam trace
	$net_sim namtrace-all $nam_file;	# record the trace in the associated file with nam_file pointer

# Define a 'Record_n_Play' procedure
	proc Record_n_Play {} {
		global net_sim nam_file
		$net_sim flush-trace;	# write all trace records
		close $nam_file;		# Close the trace file
		exec nam 3BN.nam &;		# Execute nam on the trace file
		exit 0;					# exit
	}

# Creating nodes
	set AccessHost1 [$net_sim node];	# 1st AccessHost node under the net_sim object
	set AccessHost2 [$net_sim node];	# 2nd AccessHost node under the net_sim object
	set AccessHost3 [$net_sim node];	# 3rd AccessHost node under the net_sim object
	set AccessHost4 [$net_sim node];	# 4th AccessHost node under the net_sim object
	set Router [$net_sim node];			# Router node under the net_sim object
	set FinalDest [$net_sim node];		# FinalDest node under the net_sim object

# Create a duplex link between the AccessHost(s) -- Router -- FinalDest because TCP needs duplex
	$net_sim duplex-link $AccessHost1 $Router $Access_Link_BW $Access_Link_Delay1 DropTail
	$net_sim duplex-link $AccessHost2 $Router $Access_Link_BW $Access_Link_Delay2 DropTail
	$net_sim duplex-link $AccessHost3 $Router $Access_Link_BW $Access_Link_Delay3 DropTail
	$net_sim duplex-link $AccessHost4 $Router $Access_Link_BW $Access_Link_Delay4 DropTail
	$net_sim duplex-link $Router $FinalDest $BottleNeck_BW $BottleNeck_Delay DropTail

# Give positon of node for NAM
	$net_sim duplex-link-op $AccessHost1 $Router orient right;		# so that Router appears to the Right of AccessHost1 in NAM
	$net_sim duplex-link-op $AccessHost2 $Router orient right-down;	# so that Router appears to the Right-Down of AccessHost2 in NAM
	$net_sim duplex-link-op $AccessHost3 $Router orient left-down;	# so that Router appears to the Left-down of AccessHost3 in NAM
	$net_sim duplex-link-op $AccessHost4 $Router orient left;		# so that Router appears to the Left of AccessHost4 in NAM
	$net_sim duplex-link-op $Router $FinalDest orient down;			# so that FinalDest appears to the Down of Router in NAM

# Monitor the queue for link (Router-FinalDest) (for NAM)
	$net_sim duplex-link-op $Router $FinalDest queuePos $QuePosPi;		# the queue is shown in QuePosPi times Pi with the orientation of given link
	$net_sim duplex-link-op $AccessHost1 $Router queuePos $QuePosPi;
	$net_sim duplex-link-op $AccessHost2 $Router queuePos $QuePosPi;
	$net_sim duplex-link-op $AccessHost3 $Router queuePos $QuePosPi;
	$net_sim duplex-link-op $AccessHost4 $Router queuePos $QuePosPi;

# Set Queue Size of link (Router-FinalDest)
	$net_sim queue-limit $Router $FinalDest $quelim;					# quelimit at Router for packets towards FinalDest is set to quelim
	$net_sim queue-limit $Router $AccessHost1 $quelim;					# quelimit at Router for packets towards AccessHost1 is set to quelim
	$net_sim queue-limit $Router $AccessHost2 $quelim;					# quelimit at Router for packets towards AccessHost2 is set to quelim
	$net_sim queue-limit $Router $AccessHost3 $quelim;					# quelimit at Router for packets towards AccessHost3 is set to quelim
	$net_sim queue-limit $Router $AccessHost4 $quelim;					# quelimit at Router for packets towards AccessHost4 is set to quelim

# Attaching a loss block 1
	set loss_block1 [new ErrorModel];
	$loss_block1 unit pkt;
	$net_sim at [expr $start_transmit1 + 0.101] "$loss_block1 set rate_ $error_rate";	# Error rate in fractions
	$loss_block1 ranvar [new RandomVariable/Uniform];
	$loss_block1 drop-target [new Agent/Null];
	$net_sim link-lossmodel $loss_block1 $FinalDest $Router

# Create a TCP agent and attach to AccessHost1 as tcp1 and its sink port on the FinalDest
	set tcp1 [new Agent/TCP]
	$tcp1 set fid_ 1;			# Seperate flow IDs for identification and colors
	$net_sim attach-agent $AccessHost1 $tcp1
	set sink1 [new Agent/TCPSink];		# Seperate Sinks are needed else only the last connection will be maintained (acts like seperate ports on FinalDest side)
	$net_sim attach-agent $FinalDest $sink1
# Create a TCP agent and attach to AccessHost2 as tcp2
	set tcp2 [new Agent/TCP]
	$tcp2 set fid_ 2
	$net_sim attach-agent $AccessHost2 $tcp2
	set sink2 [new Agent/TCPSink]
	$net_sim attach-agent $FinalDest $sink2
# Create a TCP agent and attach to AccessHost3 as tcp3
	set tcp3 [new Agent/TCP]
	$tcp3 set fid_ 3
	$net_sim attach-agent $AccessHost3 $tcp3
	set sink3 [new Agent/TCPSink]
	$net_sim attach-agent $FinalDest $sink3
# Create a TCP agent and attach to AccessHost4 as tcp4
	set tcp4 [new Agent/TCP]
	$tcp4 set fid_ 4
	$net_sim attach-agent $AccessHost4 $tcp4
	set sink4 [new Agent/TCPSink]
	$net_sim attach-agent $FinalDest $sink4

# Connect AccessHost(tcp agent) to Router(tcpsink agent)
	$net_sim connect $tcp1 $sink1
	$net_sim connect $tcp2 $sink2
	$net_sim connect $tcp3 $sink3
	$net_sim connect $tcp4 $sink4

# Setup a FTP application over all TCP connections
	set ftp1 [new Application/FTP]
	$ftp1 attach-agent $tcp1
	set ftp2 [new Application/FTP]
	$ftp2 attach-agent $tcp2
	set ftp3 [new Application/FTP]
	$ftp3 attach-agent $tcp3
	set ftp4 [new Application/FTP]
	$ftp4 attach-agent $tcp4

# Schedule events for the FTP agents
	$net_sim at $start_transmit1 "$ftp1 start"
	$net_sim at $start_transmit2 "$ftp2 start"
	$net_sim at $start_transmit3 "$ftp3 start"
	$net_sim at $start_transmit4 "$ftp4 start"
	$net_sim at $stop_transmit "$ftp1 stop"
	$net_sim at $stop_transmit "$ftp2 stop"
	$net_sim at $stop_transmit "$ftp3 stop"
	$net_sim at $stop_transmit "$ftp4 stop"

# Output in Terminal
	# for {set i start_transmit1} {$i < $stop_transmit} {incr $i} {
	# 	puts "AccessHost1 RTT = [$tcp1 rtt_]";
	# 	puts "AccessHost2 RTT = [$tcp2 rtt_]";
	# 	puts "AccessHost3 RTT = [$tcp3 rtt_]";
	# 	puts "AccessHost4 RTT = [$tcp4 rtt_]";
	# }

# Call the Record_n_Play procedure af 500ms of Stop transmit
	$net_sim at [expr $stop_transmit + 0.5] "Record_n_Play"

# Run the simulation object
	$net_sim run