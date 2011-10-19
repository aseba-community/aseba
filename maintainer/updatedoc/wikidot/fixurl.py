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
import urlparse

# Custom lib
from wikidot.myparser import MyParser
from wikidot.urltoname import urltoname


class FixURL(MyParser):
    """Fix HTML links (as well as images), so they point to
    the local files. If a local file is not available, the full
    link to the remote file is built.

    The list of available local files is given at initialization."""

    def __init__(self, links, host):
        """Initialization.

        links: set of available local files
        host: full path to remote host"""
        MyParser.__init__(self)
        self.local_links = links
        self.remote_host = host
        self.reset()

    def reset(self):
        MyParser.reset(self)
        self.local_set = set()      # Set of local links
        self.remote_set = set()     # Set of remote links

    # Public functions
    def get_local_links(self):
        return self.local_set

    def get_remote_links(self):
        return self.remote_set

    # Private functions
    def __is_link_local__(self, link):
        """Private - Tell if a link match a local file.

        Output:
            True if a local file match the link
            False otherwise"""
        if urltoname(link) in self.local_links:
            return True
        else:
            return False

    def __is_link_toc__(self, link):
        """Private - Tell if the link is part of the Table of Content.

        Output:
            True if the link is of the form #tocXYZ
            False otherwise"""
        if link.find('#toc') == 0:
            return True
        else:
            return False

    def __fix_link__(self, link):
        """Private - Take a link and convert it,
        either as a local link, either as a link pointing
        to the remote host."""
        if self.__is_link_toc__(link) == True:
            # don't touch it!
            return link

        if self.__is_link_local__(link) == True:
            # Convert link
            new_link = urltoname(link)
            self.local_set.add(new_link)
        else:
            # Remote link
            new_link = urlparse.urljoin(self.remote_host, link)
            self.remote_set.add(new_link)
        return new_link


    # Inherited functions
    def handle_starttag(self, tag, attrs):
        """Overidden - Parse links and convert them.

        <a> and <img> tags are looked for links."""
        # Special case 1: links
        if tag == 'a':
            for index, attr in enumerate(attrs):
                if attr[0] == 'href':
                    attrs[index] = attr[0], self.__fix_link__(attr[1])
                    break
        # Special case 2: images
        elif tag == 'img':
            for index, attr in enumerate(attrs):
                if attr[0] == 'src':
                    attrs[index] = attr[0], self.__fix_link__(attr[1])
                    break

        MyParser.handle_starttag(self, tag, attrs)



def fixurls(directory, base_url):
    """Iterate over the files of a directory, and fix the links to point to
    local files."""
    # List all files, then HTML files to be fixed
    files = os.listdir(directory)
    html_files = [x for x in files if '.html' in x]
    # Create the 'fixer'
    fix = FixURL(files, base_url)
    print >> sys.stderr, "\nFixing URLs..."
    local_set = set()
    remote_set = set()
    for x in html_files:
        file_name = os.path.join(directory, x)
        print >> sys.stderr, "Processing ", file_name
        # Parse the file
        f = open(file_name, 'r')
        fix.feed(f.read())
        f.close()
        # Write result to the file
        f = open(file_name, 'w')
        f.write(fix.get_doc())
        f.close()
        # Reset parser
        local_set.update(fix.get_local_links())
        remote_set.update(fix.get_remote_links())
        fix.reset()

    print >> sys.stderr, "\nUpdated local URLs: "
    for x in sorted(local_set):
        print >> sys.stderr, "  ", x
    print >> sys.stderr, "\nRemote URLs: "
    for x in sorted(remote_set):
        print >> sys.stderr, "  ", x


