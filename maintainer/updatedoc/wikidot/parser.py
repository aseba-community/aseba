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

# Python lib
import sys
from myparser import MyParser
from string import Template

# Local module
import wikidot.debug
from wikidot.orderedset import OrderedSet

header = \
"""
<!DOCTYPE html
     PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
     "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
<link rel='stylesheet' type='text/css' href='aseba.css' />
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
<title>${title}</title>
</head >

<body>
<h1 class="title">${title}</h1>
${toc}
"""

footer = \
"""
</body>
</html>
"""

class WikidotParser(MyParser):
    """WikidotParser is used to clean a page from www.wikidot.com,
    keeping only the interesting content."""
    def __init__(self):
        """Intialize internal variables"""
        MyParser.__init__(self)
        self.div_level = 0
        self.div_bookmark = [-1]    # List managed as a stack
        self.state = ["none"]       # List managed as a stack
        self.current_state = "none" # Point to the top of the stack
        # map for div tag attribute -> state
        # (attribute name, attribute property, state)
        self.div_state_map = \
            [
            ('id', 'page-title', 'title'),
            ('id', 'breadcrumbs', 'breadcrumbs'),
            ('id', 'page-content', 'body'),
            ('id', 'toc-action-bar', 'useless'),
            ('id', 'toc', 'toc'),
            ('style','position:absolute', 'useless')]
        self.page_title = ""
        self.toc = ""
        self.links = OrderedSet()
        self.breadcrumbs = list()

    # Public interface
    def get_doc(self):
        """Retrieve the parsed and cleaned document"""
        # format the TOC
        if self.toc != "":
            self.toc = """<table id="toc-table" summary="TOC"><tr><td>""" + self.toc
            self.toc += "</td></tr></table>"
        # Add header
        header_template = Template(header)
        self.out_doc = header_template.substitute(title=self.page_title, toc=self.toc) + self.out_doc
        # Add footer
        self.out_doc += footer
        return self.out_doc

    def get_links(self):
        """Retrieve the links embedded in the page (including images)"""
        return self.links

    def get_title(self):
        return self.page_title

    def get_breadcrumbs(self):
        return self.breadcrumbs

    # Inherited functions
    def handle_starttag(self, tag, attrs):
        """Overridden - Called when a start tag is parsed

        The heart of this function is the state machine.
        When a <div> tag is detected, the attributes are compared with
        a map of the form (name,value) -> state. If a match occurs,
        the state is pushed on top of the stack.

        Depending on the current state, the start tag is queued for output,
        or not."""
        # Debug
        if wikidot.debug.ENABLE_DEBUG == True:
            print >> sys.stderr, "<{}> {}".format(tag, attrs)

        # Update the state machine
        state_changed = self.__update_state_machine_start__(tag, attrs)

        if (state_changed == True) and (self.current_state == "body"):
            # We have just entered the body, don't output this <div> tag
            return
        if self.current_state == "body":
            # Handle special tags
            self.__handle_body_tag__(tag, attrs)
            # Add the tag to output
            MyParser.handle_starttag(self, tag, attrs)
        elif self.current_state == "toc":
            # Handle the content of the TOC
            self.toc += MyParser.format_start_tag(self, tag, attrs)
        elif (self.current_state == "breadcrumbs") and (tag == 'a'):
            # Register the breadcrumbs
            for attr in attrs:
                if (attr[0] == 'href'):
                    self.breadcrumbs.append(attr[1])
                    break

    def handle_endtag(self, tag):
        """Overridden - Called when an end tag is parsed

        The state machine is updated when a </div> tag is encountered.
        Depending on the current state, the end tag is queued for output,
        or not."""
        if self.current_state == "toc":
            # Add the tag to the TOC
            self.toc += MyParser.format_end_tag(self, tag)

        # Update the state machine
        state_changed = self.__update_state_machine_end__(tag)
        if state_changed == True:
            return

        if self.current_state == "body":
            # Add the tag to output
            MyParser.handle_endtag(self, tag)

    def handle_data(self, data):
        """Overridden - Called when some data is parsed

        Depending on the current state, the data is queued for output,
        or not."""
        if self.current_state == "title":
            # Register the title
            self.page_title += data.strip()
        elif self.current_state == "body":
            # Add data to the output
            MyParser.handle_data(self, data)
        elif self.current_state == "toc":
            # Add data to the TOC
            self.toc += data

    def handle_charref(self, name):
        """Overridden - Called when a charref (&#xyz) is parsed

        Depending on the current state, the charref is queued for output,
        or not."""
        if self.current_state == "title":
            # Add charref to the title
            self.page_title += ("&#" + name + ";")
        elif self.current_state == "body":
            # Add charref to the output
            MyParser.handle_charref(self, name)
        elif self.current_state == "toc":
            # Add charref to the TOC
            self.toc += ("&#" + name + ";")

    def handle_entityref(self, name):
        """Overridden - Called when an entityref (&xyz) tag is parsed

        Depending on the current state, the entityref is queued for output,
        or not."""
        if self.current_state == "title":
            # Add the entityref to the title
            self.page_title += ("&" + name + ";")
        elif self.current_state == "body":
            # Add the entityref to the output
            MyParser.handle_entityref(self, name)
        elif self.current_state == "toc":
            # Add the entityref to the TOC
            self.toc += ("&" + name + ";")

    def handle_decl(self, decl):
        """Overridden - Called when a SGML declaration (<!) is parsed

        Depending on the current state, the declaration is queued for output,
        or not."""
        if self.current_state == "body":
            # Add the SGML declaration to the output
            MyParser.handle_decl(self, decl)

    # Private functions
    def __update_state_machine_start__(self, tag, attrs):
        """Update the state machine."""
        state_changed = False

        if tag == 'div':
            if wikidot.debug.ENABLE_DEBUG == True:
                print >> sys.stderr, self.state, self.div_bookmark
            # Look for the id = xyz attribute
            for attr in attrs:
                for div_attr in self.div_state_map:
                    if (div_attr[0] == attr[0]) and (div_attr[1] in attr[1]):
                        # Match !
                        self.state.append(div_attr[2])
                        self.div_bookmark.append(self.div_level)
                        state_changed = True
                        break
            # Increment div level
            self.div_level += 1

        # Update the current state
        self.current_state = self.__get_current_state__()
        return state_changed

    def __update_state_machine_end__(self, tag):
        state_changed = False

        if tag == 'div':
            if wikidot.debug.ENABLE_DEBUG == True:
                print >> sys.stderr, self.state, self.div_bookmark
            self.div_level -= 1
            if self.div_level == self.div_bookmark[-1]:
                # Matching closing </div> tag -> pop the state
                self.state.pop()
                self.div_bookmark.pop()
                state_changed = True

        # Update the current state
        self.current_state = self.__get_current_state__()
        return state_changed

    def __get_current_state__(self):
        return self.state[-1]

    def __handle_body_tag__(self, tag, attrs):
        # Special case 1: links
        if tag == 'a':
            for index, attr in enumerate(attrs):
                if attr[0] == 'href':
                    # Register the link
                    self.links.add(attr[1])
                    break
        # Special case 2: images
        elif tag == 'img':
            for index, attr in enumerate(attrs):
                if attr[0] == 'src':
                    # Register the link
                    self.links.add(attr[1])
                elif attr[0] == 'width':
                    # Fix the width=xx attribute
                    # Wikidot gives width="600px", instead of width=600
                    pos = attr[1].find('px')
                    if pos >= 0:
                        attrs[index] = (attr[0], attr[1][0:pos])

