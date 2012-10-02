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

import sys
import pickle

from wikidot.urltoname import urltoname
from wikidot.tree import WikiNode

# Global variables used to build the wiki's structure
__structure__ = dict()
__lang__ = ''

def add_language(lang):
    """Create a new entry for this language in the dictionary"""
    global __structure__
    __structure__[lang] = WikiNode('root_' + lang, '')

def set_current_language(lang):
    """Set the current language, for future insertions"""
    global __lang__
    __lang__ = lang

def get_structure(lang):
    """Return the tree corresponding to a specific language"""
    return __structure__[lang]

def insert(title, url, breadcrumbs = set()):
    """Insert a page into the current tree

    Set the current language using the 'set_current_language()' function."""

    global __structure__
    # convert url
    page_link = urltoname(url)
    # convert breadcrumbs
    page_breadcrumbs = list()
    for x in breadcrumbs:
        page_breadcrumbs.append(urltoname(x))
    if __lang__ != '':
        __structure__[__lang__].insert(title, page_link, page_breadcrumbs)
#        print >> sys.stderr, "**** Dump after insert:"
#        __structure__[__lang__].dump()
    else:
        print >> sys.stderr, "*** Error in wikidot.structure.insert: set the language beforehand"

def serialize(f):
    pickler = pickle.Pickler(f)
    pickler.dump(__lang__)
    pickler.dump(__structure__)

def unserialize(f):
    global __lang__
    global __structure__
    pickler = pickle.Unpickler(f)
    __lang__ = pickler.load()
    __structure__ = pickler.load()

