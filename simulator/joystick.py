import re
from msvcrt import getch
import serial

# axis code map
x = 'yaw'
y = 'gas'
rx = 'roll'
ry = 'pitch'

class KeyboardScanner:
	MAX = 100
	DELTA = 10
        
	# Key code map
	key_funcs = {
                77: lambda keys: [x, +KeyboardScanner.DELTA], # Right
                75: lambda keys: [x, -KeyboardScanner.DELTA], # Left
                72: lambda keys: [y, +KeyboardScanner.DELTA], # Up
                80: lambda keys: [y, -KeyboardScanner.DELTA], # Down
                97: lambda keys: [rx, -KeyboardScanner.DELTA], # a
                100: lambda keys: [rx, +KeyboardScanner.DELTA], # d
                119: lambda keys: [ry, +KeyboardScanner.DELTA], # w
                115: lambda keys: [ry, -KeyboardScanner.DELTA] # s
	}
        
	def __init__(self):
		# available keys
		self.keys = {x:0, y:0, rx:0, ry:0}

	def __calc(self, char, f):
		k, v = f(self.keys)
		self.keys[k] += v
		
		if self.keys[k] > KeyboardScanner.MAX: self.keys[k] = KeyboardScanner.MAX
		elif self.keys[k] < 0: self.keys[k] = 0
	
        def getMax(self):
			"""
			Get maximum value an axis can reach
			"""
			return KeyboardScanner.MAX

        def get_x(self):
			return self.keys[x]

        def get_y(self):
			return self.keys[y]

        def get_rx(self):
			return self.keys[rx]

        def get_ry(self):
			return self.keys[ry]

	def scan(self):
		"""
		Scan function, do the same for serial
		"""
		char = ord(getch())
		func = KeyboardScanner.key_funcs.get(char)
		func != None and self.__calc(char, func)
			
		return char
		
class HubsanScanner:
	MAX = 1791
	MIN = 193
	
	def __init__(self, com):
		self.ser = serial.Serial(com, 115200, timeout=0.02)
		
		self.set_scale(1.0)
		
		# available keys
		self.keys = {x:0, y:0, rx:0, ry:0}
	
	def getMax(self):
		"""
		Get maximum value an axis can reach
		"""
		return HubsanScanner.MAX/self.scale
		
	def set_scale(self, value):
		self.scale = value * 1.0
		
		self.max_value = HubsanScanner.MAX/self.scale
		self.min_value = HubsanScanner.MIN/self.scale

	def get_x(self):
		return self.keys[x]

	def get_y(self):
		return self.keys[y]

	def get_rx(self):
		return self.keys[rx]

	def get_ry(self):
		return self.keys[ry]
		
	def __scale(self, value):
		return value * 1.0 / self.scale

	def scan(self):
		"""
		Scan function, do the same for serial
		"""
		line = self.ser.readline()
		# valid lines always start with ">>"
		if line.startswith(">>"):
			arrs = re.split("\\s*,\\s*", line.rstrip().replace(">>", ""));
			if len(arrs) < 4: return 0
			for el in arrs:
				el = el.strip()
				key, val = re.split("\\s*:\\s*", el)
				self.keys[key] = self.__scale(int(val))

			print self.keys
		return 0
		
	def __readSerialLines(self):
		line = ""
		available = False
		while(True):
			ch = self.ser.read(1)
			if (len(ch) > 0):
				line += ch[0]
				available = True
			else: break
		
		return available and line.strip().split("\n") or None
			