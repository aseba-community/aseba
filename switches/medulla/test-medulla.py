#!/usr/bin/python
# -*- coding: utf-8 -*-

import dbus
import dbus.mainloop.glib
import gobject
from optparse import OptionParser

def get_variables_reply(r):
	print 'variables:'
	print str(r)

def get_variables_error(e):
	print 'error:'
	print str(e)
	loop.quit()

if __name__ == '__main__':
	parser = OptionParser()
	parser.add_option("-s", "--system", action="store_true", dest="system", default=False,
			help="use the system bus instead of the session bus")

	(options, args) = parser.parse_args()

	dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
	
	if options.system:
		bus = dbus.SystemBus()
	else:
		bus = dbus.SessionBus()
	
	network = dbus.Interface(bus.get_object('ch.epfl.mobots.Aseba', '/'), dbus_interface='ch.epfl.mobots.AsebaNetwork')
	print network.GetNodesList()

	#network.SetVariable("e-puck 1", "leftSpeed", [10])
	#network.SetVariable("e-puck 1", "rightSpeed", [-10])

	network.SendEvent(0, [])

	#print network.GetVariable("e-puck 1", "leftSpeed",reply_handler=get_variables_reply,error_handler=get_variables_error)

	print 'starting loop'
	loop = gobject.MainLoop()
	loop.run()
