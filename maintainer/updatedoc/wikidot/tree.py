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

class WikiNode:
    """Build a tree, mirroring the structure of a wikidot-based wiki"""

    def __init__(self, title, link):
        """x.__init__(title, link) creates a new node x.

        This node corresponds to a page with the title set to 'title'
        and whose link is 'link'."""

        self.title = title
        self.link = link
        self.children = list()

    def __repr__(self):
        """x.__repr__() <==> repr(x)"""
        return "{} ({}) - {} children".format(self.title, self.link, len(self.children))

    def __getitem__(self, y):
        """x.__getitem__(y) <==> x[y]

        Return the y-th child of the node"""

        return self.children[y]

    def insert(self, title, link, breadcrumbs):
        """Insert a new node into the tree.

        Inputs:
            title: The page's title
            link: The page's URL (can be just the name, or full URL, or a partial path)
            breadcrumbs: list listing the URL of the parents, starting from the root

        The URLs should be coherent between all inputs.

        Output:
            Newly inserted node. 'None' if no corresponding parents.
        """
        if breadcrumbs == []:
            # it is a leaf, insert it
            child = WikiNode(title, link)
            self.children.append(child)
            return child
        else:
            # search a corresponding child
            for x in self.children:
                if x.link == breadcrumbs[0]:
                    # match
                    return x.insert(title, link, breadcrumbs[1:])
            # failure
            return None

    def dump(self, level=0):
        """Recursively dump to stderr the whole tree"""
        print >> sys.stderr, level * '  ' + str(level) + " {} - {}".format(self.title, self.link)
        level += 1
        for x in self.children:
            x.dump(level)

