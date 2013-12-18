"""vbammerTime.py: Script to run executables in a smart way."""

__author__      = "dbsima"
__copyright__   = "Copyright 2013, iPixel"

from os import walk, system
import subprocess
import sys
from PIL import Image
import os

def timeout_command(command, timeout):
	"""
	Execute process with timeout

	If the runtime of the execution is over the amount of milliseconds
	from timeout the process is killed and the exit code is -9.

	Otherwise, it is returned the exit code of the process.
	"""
	import subprocess, datetime, os, time, signal

	start = datetime.datetime.now()
	# create process
	process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	process_name = str((command.split("\\")[-1]).split(".")[0])

	print "\nprocess " + process_name + " has started"
	while process.poll() is None:
		# the process has not finished
		time.sleep(0.1)
		now = datetime.datetime.now()
		t = now - start

		milliseconds = (t.days * 24 * 60 * 60 + t.seconds) * 1000 + t.microseconds / 1000.0

		if milliseconds > timeout:
			subprocess.Popen("taskkill /F /T /PID %i"%process.pid , shell=True)
			print "Error: process " + process_name + " was killed (runtime over timeout)"
			return -9
	print "Success: process " + process_name + " has exited with code " + str(process.returncode)
	return process.returncode

if len(sys.argv) != 7:
	print "\nusage: vbammerTime.exe <t1> <t2> <path_to_bams> <path_to_input_image> <path_to_output_images> <path_to_final_image>" 
else:
	# check if <t1> argument is valid
	try:
		t1 = float(sys.argv[1]) #
		if isinstance(t1, float):
			pass
	except ValueError:
		sys.exit("\nT1 is not a float number.")
	
	# check if <t1> argument is valid
	try:
		t2 = int(sys.argv[2]) #
		if isinstance(t2, int):
			pass
	except ValueError:
		sys.exit("\nT2 is not an int number.")
	
	# check if <path_to_bams> argument is valid
	if os.path.isdir(sys.argv[3])  == True:
		path_to_bams = sys.argv[3] #"C:\\Users\\Student\\vbam\\bams" 
	else:
		sys.exit("\nThe path to the bams is not valid.")

	# check if <path_to_input_image> argument is valid
	try:
		with open(sys.argv[4]):
			path_to_input_image = sys.argv[4] #"C:\\Users\\Student\\vbam\\images\\test4.jpg"
	except IOError:
		sys.exit("\nThe input image does not exist.")
	
	# check if <path_to_output_images>> argument is valid
	if os.path.isdir(sys.argv[5])  == True:
		path_to_output_images = sys.argv[5] #
	else:
		sys.exit("\nThe path to the output images is not valid.")

	# check if <path_to_final_image>> argument is valid
	path_to_final_image = sys.argv[6] #

	# number of pixels in the input image
	img = Image.open(path_to_input_image).getdata()
	pixels = len(img)

	path_to_bammer = os.path.dirname(os.path.abspath(__file__)) + "\\bamerTime.exe"

	# timeout of a bam
	timeout = t1*pixels + t2

	# get files from directory
	f = []
	for (dirpath, dirnames, filenames) in walk(path_to_bams):
		f.extend(filenames)
		break

	bammerTime_args = path_to_input_image
	for filename in f:
		if filename.split(".")[1] == "exe" and filename != "bammerTime.exe" and  filename != "vbammerTime.exe":
			input_image_name = path_to_input_image.split("\\")[-1]
			path_to_output_image = path_to_output_images + "\\" + filename + "_" + input_image_name + ".TIFF"
			path_to_output_conf = path_to_output_images + "\\" + filename + "_" + input_image_name + "_conf.TIFF"
			result = timeout_command(path_to_bams + "\\" + filename + " " + path_to_input_image + " " + path_to_output_image + " " + path_to_output_conf, timeout)

			if result >= 0:
				print "Success: process " + filename.split(".")[0] + " computed images"
				bammerTime_args = bammerTime_args + " " + path_to_output_image
				bammerTime_args = bammerTime_args + " " + path_to_output_conf

	if bammerTime_args != path_to_input_image:
		print "\nSuccess: vbam has started "
		process = subprocess.Popen(path_to_bammer + " " + bammerTime_args , stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		out, err = process.communicate()
		print "Success: VBAM has exited with code " + str(process.returncode)
	else:
		print "Error: Nothing to do here! The bams did not provide any valid images."

