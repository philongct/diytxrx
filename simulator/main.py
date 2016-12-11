import os
import atexit

from ctypes import *
from joystick import *

# Load libraries
vjoy = CDLL( os.sep.join([os.getcwd(), "SDK", "lib", "amd64", "vJoyInterface.dll"]))

# Axis IDs
HID_USAGE_X = 0x30
HID_USAGE_Y = 0x31
HID_USAGE_RX = 0x33
HID_USAGE_RY = 0x34

class VJoyFeeder:
	def __init__(self):
		# Device ID to work with
		self.dev_id = 0

		# axis bound
		self.axis_bound = {HID_USAGE_X: {'min':0, 'max':0},
				   HID_USAGE_Y: {'min':0, 'max':0},
				   HID_USAGE_RX: {'min':0, 'max':0},
				   HID_USAGE_RY: {'min':0, 'max':0}}

	def exit_program(self, dev_id):
		vjoy.RelinquishVJD(dev_id)
		print "Released device " + str(dev_id)

	def init_vjoy(self):
		"""
		Init vjoy device. Call this fore most
		"""
		print "Vjoy ", str(vjoy.GetvJoyVersion()), " Enabled " + str(vjoy.vJoyEnabled())

		vjd = c_int(0)
		vjoy.GetNumberExistingVJD(byref(vjd))

		if vjd.value > 0:
			print "Device 1 selected"
			self.dev_id = 1
		else:
			print "No devices configure!"
			return False

		status = vjoy.GetVJDStatus(self.dev_id)
		# Device free. See header file for enums
		if status == 1 and vjoy.AcquireVJD(self.dev_id):
			print "Device acquired"
			atexit.register(self.exit_program, self.dev_id)
		else:
			print "Couldn't acquire device. Status code: " + str(status)
			return False

		self.__get_axis_bound();
		return True

	def __get_axis_bound(self):
		for axis, bound in self.axis_bound.iteritems():
			val = c_long(0)

			# Get max
			if vjoy.GetVJDAxisMax(self.dev_id, axis, byref(val)):
				self.axis_bound[axis]['max'] = val.value
			else:
				print "Error getting axis bound max"

			# Get min
			if vjoy.GetVJDAxisMin(self.dev_id, axis, byref(val)):
				self.axis_bound[axis]['min'] = val.value
			else:
				print "Error getting axis bound min"

		print "Vjoy axis boundary:"
		print self.axis_bound

	def reset_axis(self):
		print "Reset device"
		vjoy.ResetVJD(self.dev_id)

	def set_axis(self, axis_id, percent):
		value = round(percent*(self.axis_bound[axis_id]['max'] - self.axis_bound[axis_id]['min']))
		#print "Set ", str(axis_id), " value ", str(value), "(", str(percent * 100), ")"
		
		return vjoy.SetAxis(int(value), self.dev_id, axis_id)

def mapVal(input, min, max):
	return (input - min)/(max - min);
	
def run():
	feeder = VJoyFeeder()
	
	if feeder.init_vjoy():
		feeder.reset_axis()
	else:
		os.exit(1)
		
	com = raw_input('Enter COM port: ').strip();
	if len(com) == 0: com = "COM6"

	input = HubsanScanner(com)

	print "Input scan begin. Press ctrl+c to exit"
	while input.scan() != 3:
		feeder.set_axis(HID_USAGE_X, mapVal(input.get_x(), input.min_value, input.max_value))
		feeder.set_axis(HID_USAGE_Y, mapVal(input.get_y(), input.min_value, input.max_value))
		feeder.set_axis(HID_USAGE_RX, mapVal(input.get_rx(), input.min_value, input.max_value))
		feeder.set_axis(HID_USAGE_RY, mapVal(input.get_ry(), input.min_value, input.max_value))
		
		
if __name__ == "__main__":
	run()
