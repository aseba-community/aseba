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
import sys
import os.path
import urlparse

# Custom lib
from wikidot.tools import fetchurl
from wikidot.fixurl import fixurls
from wikidot.tools import tidy
from wikidot.tools import urltoname

def fetchwikidot(starturl, outputdir):
    # Create the output directory, if needed
    try:
        os.stat(outputdir)
    except OSError, e:
        print >> sys.stderr, "Creating output directory; ", outputdir
        os.mkdir(outputdir)

    # Fetch root page
    output = os.path.join(outputdir, urltoname(starturl))
    newlinks = fetchurl(starturl, output)

    # Create a set with fetched links (avoid loops...)
    links = set(starturl)

    # Iterate on the links, and recursively download / convert
    fetchlinks = newlinks
    breadcrumbs = os.path.basename(urlparse.urlparse(starturl).path)
    while len(fetchlinks) > 0:
        newlinks = set()
        for url in fetchlinks:
            url = urlparse.urljoin(starturl, url)
            output = os.path.join(outputdir, urltoname(url))
            print >> sys.stderr, "\nProcessing ", url
            # Link on the same server?
            if (urlparse.urlparse(url).netloc == urlparse.urlparse(starturl).netloc):
                newlinks.update(fetchurl(url, output, breadcrumbs))
            else:
                print >> sys.stderr, "*** {} is not on the same server. Link skipped.".format(url)
        # Update sets of links
        links.update(fetchlinks)
        fetchlinks = newlinks - links

    # Fix local urls for the files of the output directory
    fixurls(outputdir, starturl)

    # Clean HTML code
    tidy(outputdir)

    # We are done
    print >> sys.stderr, "\nDone!"


# When executed from the command line
if __name__ == "__main__":
    # Input arguments
    if len(sys.argv) == 3:
        starturl = sys.argv[1]
        outputdir = sys.argv[2]
    else:
        print >> sys.stderr, "Wrong number of arguments.\n"
        print >> sys.stderr, "Usage:"
        print >> sys.stderr, "  {} root_page output_dir".format(sys.argv[0])
        exit(1)

    fetchwikidot(starturl, outputdir)
