#!/usr/bin/env python
# -*- coding: utf-8 -*-

import glob
import os
import sys
import subprocess

# note: to be launched from where the svg files are
#       this script requires inkscape, imagemagick and makeicns

def cmd_exists(cmd):
	return subprocess.call("type " + cmd, shell=True, 
		stdout=subprocess.PIPE, stderr=subprocess.PIPE) == 0

if __name__ == '__main__':
	if not cmd_exists('inkscape'):
		print 'Install Inkscape!'
		sys.exit(1)
	if not cmd_exists('convert'):
		print 'convert not found, Windows icons generation disabled'
	if not cmd_exists('makeicns'):
		print 'makeicns not found, OS X icons generation disabled'
	
	for file in glob.glob('*.svg'):
		
		def convert(target_dir, target_size, add=''):
			source_file = os.path.join(os.getcwd(), file)
			target_file = os.path.join(os.getcwd(), target_dir, os.path.splitext(file)[0] + add + '.png')
			print source_file
			print target_file
			cmd = 'inkscape --export-png={0} --export-width={1} --export-height={1} {2}'
			os.system(cmd.format(target_file, target_size, source_file))
		
		base_source_file = os.path.splitext(file)[0]
		print 'Processing ' + base_source_file
		
		print 'Generating freedesktop files'
		convert('../freedesktop/48x48', '48')
		
		if cmd_exists('convert'):
			print 'Generating Windows ico'
			convert('../windows/', '256', '-256')
			convert('../windows/', '48', '-48')
			os.system('convert ../windows/{0}-48.png ../windows/{0}-256.png ../windows/{0}.ico'.format(base_source_file))
			os.system('rm ../windows/{0}-48.png ../windows/{0}-256.png'.format(base_source_file))
		
		if cmd_exists('makeicns'):
			print 'Generating OS X icns'
			convert('../osx/', '512', '-512')
			convert('../osx/', '256', '-256')
			convert('../osx/', '128', '-128')
			convert('../osx/', '32', '-32')
			convert('../osx/', '16', '-16')
			os.system('makeicns -512 ../osx/{0}-512.png -256 ../osx/{0}-256.png -128 ../osx/{0}-128.png -32 ../osx/{0}-32.png -16 ../osx/{0}-16.png -out ../osx/{0}.icns'.format(base_source_file))
			os.system('rm ../osx/{0}-*.png'.format(base_source_file))

