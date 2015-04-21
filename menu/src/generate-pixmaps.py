#!/usr/bin/env python
# -*- coding: utf-8 -*-

import glob
import os

# note: to be launched from where the svg files are
#       this script requires inkscape and imagemagick

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
	print 'Generating Windows ico'
	convert('../windows/', '256', '-256')
	convert('../windows/', '48', '-48')
	os.system('convert ../windows/{0}-48.png ../windows/{0}-256.png ../windows/{0}.ico'.format(base_source_file))
	os.system('rm ../windows/{0}-48.png ../windows/{0}-256.png'.format(base_source_file))

