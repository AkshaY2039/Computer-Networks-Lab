from pox.core import core
import pox.openflow.libopenflow_01 as of
import random

log = core.getLogger()

class router (object):
	def __init__ (self, connection):
		# Keep track of the connection to the switch so that we can
		# send it messages!
		self.connection = connection

		# This binds our PacketIn event listener
		connection.addListeners(self)

		# Use this table to keep track of which ethernet address is on
		# which switch port (keys are MACs, values are ports).
		self.mac_to_port = {}


	def resend_packet (self, packet_in, out_port):
		msg = of.ofp_packet_out()
		msg.data = packet_in

		# Add an action to send to the specified port
		action = of.ofp_action_output(port = out_port)
		msg.actions.append(action)

		# Send message to switch
		self.connection.send(msg)


	def prob_route (self, packet, packet_in, ports_list):
		# pick a random index
		r = random.randint(0, len(ports_list)-1)
		# get that port
		port_out = ports_list[r]
		port_in = packet_in.in_port
		print("from: " + str(port_in))
		print("sendto: " + str(port_out))

		# resend to selected output port
		self.resend_packet(packet_in, port_out)


	def _handle_PacketIn (self, event):
		ports_list = list(event.connection.ports)
		ports_list.remove(ports_list[-1])

		packet = event.parsed # This is the parsed packet data.
		if not packet.parsed:
			log.warning("Ignoring incomplete packet")
			return

		packet_in = event.ofp # The actual ofp_packet_in message.

		# Pass input packet to the router
		self.prob_route(packet, packet_in, ports_list)

def launch ():
	def start_switch (event):
		log.debug("Controlling %s" % (event.connection,))
		router(event.connection)
	core.openflow.addListenerByName("ConnectionUp", start_switch)