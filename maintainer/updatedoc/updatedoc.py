#! /usr/bin/env python

#   Aseba - an event-based framework for distributed robot control
#   Copyright (C) 2007--2011:
#           Stephane Magnenat <stephane at magnenat dot net>
#           (http://stephane.magnenat.net)
#           and other contributors, see authors.txt for details
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, version 3 of the License.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with this program. If not, see <http://www.gnu.org/licenses/>.

# System lib
import os
import os.path
import sys
from shutil import rmtree
from shutil import copytree
from shutil import copy

# Custom lib
from wikidot.fetch import fetchwikidot
import wikidot.structure
import qthelp

#language_map = [('fr','http://aseba.wikidot.com/fr:asebausermanual')]
language_map = [
                ('en','http://aseba.wikidot.com/en:asebausermanual'),
                ('fr','http://aseba.wikidot.com/fr:asebausermanual'),
                ('de','http://aseba.wikidot.com/de:asebausermanual'),
                ('es','http://aseba.wikidot.com/es:asebausermanual')
                ]
OUTPUT_DIR = 'doc'
QHP_FILE = './aseba-doc.qhp'
CSS_FILE = './css/aseba.css'
STUDIO_PATH = '../../studio/'

# Clean
rmtree(OUTPUT_DIR, True)
os.mkdir(OUTPUT_DIR)
try:
    os.remove(QHP_FILE)
except OSError:
    # File doesn't exist
    pass

# Fetch the wiki, for all languages
output_directories = list()
available_languages = list()
for x in language_map:
    lang = x[0]
    available_languages.append(lang)
    print >> sys.stderr, "\n*** Getting doc for language ", lang, " ***"
    # Get + clean the output directory for the current language
    output = OUTPUT_DIR + '_' + lang
    output_directories.append(output)
    rmtree(output, True)
    # Prepare the tree before fetching this language
    wikidot.structure.add_language(lang)
    wikidot.structure.set_current_language(lang)
    # Fetch!
    fetchwikidot(x[1], output)
    # Add the CSS to output directory
    copy(CSS_FILE, output)
    # Copy from output to OUTPUT_DIR
    # copytree not possible, as OUTPUT_DIR already exists
    listing = os.listdir(output)
    for y in listing:
        copy(os.path.join(output, y), os.path.join(OUTPUT_DIR, y))


# Generate the Qt Help files
qthelp.generate(output_directories, OUTPUT_DIR, available_languages, QHP_FILE)

# Clean Aseba Studio files
print >> sys.stderr, "\nCleaning Aseba Studio directory..."
studio_output_dir = os.path.join(STUDIO_PATH, OUTPUT_DIR)
studio_doc_qhp = os.path.join(STUDIO_PATH, QHP_FILE)
rmtree(studio_output_dir)
try:
    os.remove(studio_doc_qhp)
except OSError:
    # File doesn't exist
    pass

# Copy new files
print >> sys.stderr, "\nCopying new files to Aseba Studio..."
copytree(OUTPUT_DIR, studio_output_dir)
copy(QHP_FILE, STUDIO_PATH)

print >> sys.stderr, "Finished!!! :-)"
