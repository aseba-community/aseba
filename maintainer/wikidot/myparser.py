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

from HTMLParser import HTMLParser

class MyParser(HTMLParser):
    """Custom HTML parser, derived from HTMLParser lib.

    Member functions are overidden to output the HTML document
    as it, without any changes. The document is retrieved with the
    get_doc() function."""

    def __init__(self):
        """Initialize the parser"""
        HTMLParser.__init__(self)
        self.reset()

    # Public interface
    def reset(self):
        """Reset the parser's state"""
        self.out_doc = ""
        HTMLParser.reset(self)

    def get_doc(self):
        """Return the parsed document"""
        return self.out_doc

    # Private functions
    def format_start_tag(self, tag, attrs):
        """Private - Format a <tag attributes>"""
        # Format back attributes
        attributes = ""
        for attr in attrs:
            if ~isinstance(attr[1], str):
                attr = (attr[0], str(attr[1]))
            attributes += (attr[0] + "=\"" + attr[1] + "\" ")
        return "<{} {}>".format(tag, attributes)

    def format_end_tag(self, tag):
        """Private - Format an </tag>"""
        return "</{}>".format(tag)

    # Inherited functions
    def handle_starttag(self, tag, attrs):
        """Overidden - Called when a start tag is parsed"""
        self.out_doc += self.format_start_tag(tag, attrs)

    def handle_endtag(self, tag):
        """Overidden - Called when an end tag is parsed"""
        self.out_doc += self.format_end_tag(tag)

    def handle_data(self, data):
        """Overidden - Called when some data is encountered"""
        """
        chars = self.badchars_regex.findall(data)
        if len(chars) > 0:
            print >> sys.stderr, "Found bad characters: ", chars
        """
        self.out_doc += data

    def handle_charref(self, name):
        """Overidden - Called when a charref (&#xyz) is parsed"""
        self.out_doc += ("&#" + name + ";")

    def handle_entityref(self, name):
        """Overidden - Called when an entityref (&xyz) is parsed"""
        self.out_doc += ("&" + name + ";")

    def handle_decl(self, decl):
        """Overidden - Called when a SGML declaration (<!) is parsed"""
        self.out_doc += ("<!" + decl + ">")

