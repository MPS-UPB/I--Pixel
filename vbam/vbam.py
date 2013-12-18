from os import walk, system
import subprocess
import sys
from PIL import Image
import os
def timeout_command(command, timeout):
	"""
	"""
	import subprocess, datetime, os, time, signal

	start = datetime.datetime.now()
	process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	while process.poll() is None:
		time.sleep(0.1)
		now = datetime.datetime.now()
		t = now - start

		milliseconds = (t.days * 24 * 60 * 60 + t.seconds) * 1000 + t.microseconds / 1000.0
		print str(milliseconds)  + " - "+ str(timeout)

		if milliseconds > timeout:
			subprocess.Popen("taskkill /F /T /PID %i"%process.pid , shell=True)
			print "process " + str(command) + " was killed"
			return -9
	print "Process " + str(command) + " has exited with code " + str(process.returncode)
	return process.returncode

if len(sys.argv) != 7:
	print "usage: vbammerTime.exe <t1> <t2> <path_to_bams> <path_to_input_image> <path_to_output_images> <path_to_final_image>" 
else:
	# check if <t1> argument is valid
	try:
		t1 = float(sys.argv[1]) #
		if isinstance(t1, float):
			pass
	except ValueError:
		sys.exit("T1 is not a float number.")
	
	# check if <t1> argument is valid
	try:
		t2 = int(sys.argv[2]) #
		if isinstance(t2, int):
			pass
	except ValueError:
		sys.exit("T2 is not an int number.")
	
	# check if <path_to_bams> argument is valid
	if os.path.isdir(sys.argv[3])  == True:
		path_to_bams = sys.argv[3] #"C:\\Users\\Student\\vbam\\bams" 
	else:
		sys.exit("The path to the bams is not valid.")

	# check if <path_to_input_image> argument is valid
	try:
		with open(sys.argv[4]):
			path_to_input_image = sys.argv[4] #"C:\\Users\\Student\\vbam\\images\\test4.jpg"
	except IOError:
		sys.exit("The input image does not exist.")
	
	# check if <path_to_output_images>> argument is valid
	if os.path.isdir(sys.argv[5])  == True:
		path_to_output_images = sys.argv[5] #
	else:
		sys.exit("The path to the output images is not valid.")

	# check if <path_to_final_image>> argument is valid
	# TODO: parse path to get dir 
	path_to_final_image = sys.argv[6] #

	# number of pixels in the input image
	img = Image.open(path_to_input_image).getdata()
	pixels = len(img)
	print "number of pixels = "  + str(pixels)

	path_to_bammer = os.path.dirname(os.path.abspath(__file__)) + "\\bammerTime.exe"
	print path_to_bammer

	# timeout of a bam
	timeout = t1*pixels + t2

	print "timeout " + str(timeout)

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
				bammerTime_args = bammerTime_args + " " + path_to_output_image
				bammerTime_args = bammerTime_args + " " + path_to_output_conf
	print bammerTime_args

	if bammerTime_args != path_to_input_image:
		print "yes"
		process = subprocess.Popen(path_to_bammer + " " + bammerTime_args , stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		out, err = process.communicate()
		print "\n" + out + str(process.returncode) +"\n"
	else:
		print "the bams did not provide any valid images"

