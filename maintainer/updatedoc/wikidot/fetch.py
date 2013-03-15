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
import re

# Custom lib
from wikidot.tools import fetchurl
from wikidot.fixurl import fixurls
from wikidot.tools import tidy
from wikidot.tools import fix_latex
from wikidot.urltoname import urltoname
from wikidot.orderedset import OrderedSet

ALTERNATE_SERVERS = {
        'wikidot': set(['wikidot', 'wdfiles']),
        }

# TODO: somewhat ugly hack...
def _get_alternate_server(url):
    for base_server, servers in ALTERNATE_SERVERS.iteritems():
        for server in servers:
            if server in url:
                #print "Replacing ", server, " with ", base_server, "..."
                return re.sub(server, base_server, url)
    # no match
    return url


def fetchwikidot(starturl, outputdir):
    # Create the output directory, if needed
    try:
        os.stat(outputdir)
    except OSError, e:
        print >> sys.stderr, "Creating output directory; ", outputdir
        os.mkdir(outputdir)

    # Fetch root page
    output = os.path.join(outputdir, urltoname(starturl))
    retval = fetchurl(starturl, output)
    newlinks = retval['links']
    breadcrumbs = retval['breadcrumbs']
    # get the last element of the list
    if len(breadcrumbs) > 0:
        breadcrumbs = breadcrumbs[len(breadcrumbs)-1]
    else:
        breadcrumbs = ''

    # Create a set with fetched links (avoid loops...)
    links = OrderedSet(starturl)

    # Iterate on the links, and recursively download / convert
    fetchlinks = newlinks
    while len(fetchlinks) > 0:
        newlinks = OrderedSet()
        for url in fetchlinks:
            url = urlparse.urljoin(starturl, url)
            output = os.path.join(outputdir, urltoname(url))
            print >> sys.stderr, "\nProcessing ", url
            # Link on the same server? If no match, search the list of alternative servers
            start_server = urlparse.urlparse(starturl).netloc
            link_server = urlparse.urlparse(url).netloc
            if (link_server == start_server or _get_alternate_server(link_server) == _get_alternate_server(start_server)):
                retval = fetchurl(url, output, breadcrumbs)
                newlinks.update(retval['links'])
            else:
                print >> sys.stderr, "*** {} is not on the same server. Link skipped.".format(url)
        # Update sets of links
        links.update(fetchlinks)
        fetchlinks = newlinks - links

    # Fix local urls for the files of the output directory
    fixurls(outputdir, starturl)

    # Clean HTML code
    tidy(outputdir)

    # Convert equations to PNG
    fix_latex(outputdir)

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
