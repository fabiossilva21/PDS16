#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import re
import binascii

addressSymbol = []
numericSymbol = []

def addToSymbolsArray(name, symtype, value):
	global addressSymbol
	global numericSymbol

	if symtype == "N":
		symtype = "0"
		name_length = "%04x" % len(name)
		name_hex = binascii.hexlify(name)
		string = symtype+name_length+name_hex+value+"\n"
		numericSymbol.append(string)
		return
	if symtype == "E":
		symtype = "1"
		name_length = "%04x" % len(name)
		name_hex = binascii.hexlify(name)
		string = symtype+name_length+name_hex+value+"\n"
		addressSymbol.append(string)
		return

	print "Well! If we got here there is a problem with our regex sentence! Pls fix"


def parseLST(file):
	regex = ur"(^[a-zA-Z][a-zA-Z0-9_\-]*)\s*([E-N])\s*([0-9A-F]*)H"
	regex = re.compile(regex)

	while 1:
		line = file.readline()
		if not line:
			print "We reached the end of the file and didn't find the Symbols section! :( Please update!"
			exit()
		if "Tipo" in line:
			line = file.readline()
			line = file.readline()
			while "E - " not in line:
				match = regex.match(line)
				if match:
					addToSymbolsArray(match.group(1), match.group(2), match.group(3))
				line = file.readline()
			return


def main():
	if (len(sys.argv) < 2 or len(sys.argv) > 3):
		print "Bad calling: \n"
		print "Usage: LSTtoSymbols.py <lst_file> (optional)<output_file_name>"
		exit()
	if len(sys.argv) == 2:
		lst_file_name = sys.argv[1]
		output_file_name = lst_file_name[:-3] + 'syms'
	if len(sys.argv) == 3:
		lst_file_name = sys.argv[1]
		output_file_name = sys.argv[2]

	try:
		lst_file = open(lst_file_name, "r")
	except:
		print "File " + lst_file_name + " is not in the current directory."
		exit()
	
	parseLST(lst_file)

	try:
		output_file = open(output_file_name, "w+")
	except:
		print "Something went wrong! I can't create the output_file... Check if you have write permissions on this directory"

	for line in addressSymbol:
		output_file.write(line)
	for line in numericSymbol:
		output_file.write(line)


if __name__ == "__main__":
	main()