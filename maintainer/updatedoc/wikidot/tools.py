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
import urllib2
import mimetypes
import subprocess

# Custom lib
import wikidot.debug
import wikidot.structure
from wikidot.urltoname import urltoname
from wikidot.parser import WikidotParser
import wikidot.tex2png

def __fix_breadcrumbs__(breadcrumbs, toplevel):
    # check if toplevel is part of the breadcrumbs,
    # and rebase the basecrumb on this level
    if toplevel == '':
        return breadcrumbs
    output = list()
    output_enable = False
    for bc in breadcrumbs:
        if output_enable:
            output.append(bc)
        if toplevel in bc:
            output_enable = True
    return output

def fetchurl(page, offline_name, breadcrumbs = ''):
    """Given a wikidot URL, fetch it, convert it and store it locally.

    Inputs:
        page:           URL of the wikidot page
        offline_name    Local file name for storage purpose
        breadcrumbs     (Optional) Name of the root page. If given,
                        checks that the breadcrumbs reference this page
                        (child page). If not, page is discarded
    Output:
        Links to other pages / images."""
    try:
        # Get the page
        print >> sys.stderr, "Connecting to {}...".format(page)
        response = urllib2.urlopen(page)
    except urllib2.HTTPError, e:
        print >> sys.stderr, e.code
    except urllib2.URLError, e:
        print >> sys.stderr, e.reason
    else:
        retval = dict()
        retval['links'] = set()
        retval['breadcrumbs'] = list()
        # Check MIME type
        mime = mimetypes.guess_type(page)[0]
        if (mime == None) or ('html' in mime):
            # HTML or unknown type
            # Convert the wikidot page and check the breakcrumbs
            print >> sys.stderr, "Parsing..."
            parser = WikidotParser()
            parser.feed(response.read())
            # Test if the breadcrumbs links to the main page
            page_breadcrumbs = parser.get_breadcrumbs()
            page_breadcrumbs = __fix_breadcrumbs__(page_breadcrumbs, breadcrumbs)
            retval['breadcrumbs'].extend(page_breadcrumbs)
            if (breadcrumbs == '') or (len(page_breadcrumbs) > 0):
                # Ok, page valid
                wikidot.structure.insert(parser.get_title(), page, page_breadcrumbs if breadcrumbs != '' else set())
                data = parser.get_doc()
                retval['links'] = parser.get_links()
            else:
                # Page is not linked to the main page
                print >> sys.stderr, "*** Page is not part of the documentation. Page skipped."
                return retval
        elif ('image' in mime):
            # Image
            data = response.read()
        else:
            # Type is not supported
            if wikidot.debug.ENABLE_DEBUG == True:
                print >> sys.stderr, "*** This is not a supported type of file. File skipped."
            return retval
        # Save
        print >> sys.stderr, "Saving the result to {}...".format(offline_name)
        f = open(offline_name, 'w')
        f.write(data)
        f.close()
        if wikidot.debug.ENABLE_DEBUG == True:
            print >> sys.stderr, "***DEBUG: links: ", reval['links']
        return retval

def tidy(directory):
    html_files = [x for x in os.listdir(directory) if '.html' in x]
    print >> sys.stderr, "\nCleaning HTML files..."
    for x in html_files:
        filename = os.path.join(directory, x)
        print >> sys.stderr, "Processing ", filename
        retcode = subprocess.call(["tidy","-config", "wikidot/tidy.config", "-q", "-o", filename, filename])

def fix_latex(directory):
    """Given a directory, convert LaTeX code to PNG images for every HTML file.

    Inputs:
        directory:      Source directory

    Output:
        No output."""

    html_files = [x for x in os.listdir(directory) if '.html' in x]
    print >> sys.stderr, "\nConverting LaTeX equations embedded in HTML files..."
    for x in html_files:
        filename = os.path.join(directory, x)
        print >> sys.stderr, "Processing ", filename
        wikidot.tex2png.from_html(filename, os.path.splitext(filename)[0])

