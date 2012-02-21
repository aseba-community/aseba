#!/usr/bin/env python
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

import os
import os.path
import shutil
import sys
import re
from string import Template
import subprocess

tex_file = \
r"""
\documentclass{article}
\pagestyle{empty}
\begin{document}
${formula}
\end{document}
"""

tex_file_template = Template(tex_file)
tex_math_re = re.compile(r"<span class=\"math-inline\">(.*?)</span>")

def from_tex(tex_code, filename, density = 100):
    """Given a LaTeX string, generates a PNG file at a given density.
    Temporary files are generated in the current folder, prefixed by filename, 
    then erased.

    Inputs:
        tex_code:       String with a valid LaTeX code (ex: $a_i = b_i$)
        filename        Filename, without the extension. Can be located in another directory
        density         (Optional) Output density, by default at 100

    Output:
        No output."""

    basename = os.path.basename(filename)   # process in the current folder, result will be copied later
    f = open("{0}.tex".format(basename),'w')
    f.write(tex_file_template.substitute(formula=tex_code))
    f.close()
    devnull = open(os.devnull)
    retcode = subprocess.call(["texi2dvi","{0}.tex".format(basename)], stdout=devnull, stderr=devnull)
    retcode = subprocess.call(["dvips","-E","{0}.dvi".format(basename)], stdout=devnull, stderr=devnull)
    retcode = subprocess.call(["convert","-density","{0}x{0}".format(density),"{0}.ps".format(basename),"{0}.png".format(basename)])
    # copy the result to the final file (if directory differes)
    if basename != filename:
        local_png = "{0}.png".format(basename)
        new_png = "{0}.png".format(filename)
        shutil.copy(local_png, new_png)
        os.remove(local_png)
    os.remove("{0}.tex".format(basename))
    os.remove("{0}.aux".format(basename))
    os.remove("{0}.dvi".format(basename))
    os.remove("{0}.log".format(basename))
    os.remove("{0}.ps".format(basename))

def from_html(source_html_file, output_basename):
    """Given an HTML file, convert all LaTeX codes enclosed between <span class="math-inline"> tags.

    Inputs:
        source_html_file    Path to the HTML file
        output_basename     Basename for the output files, without any extension.
                            Corresponding .html and .png files will be generated.

    Output:
        No output."""

    try:
        f = open(source_html_file, 'r')
    except IOError:
        print >> sys.stderr, "No such file: {0}".format(source_html_file)
        return
    else:
        html = f.read()
        f.close()

    # counter
    i = 0

    # lookup dictionnary
    tex_lookup = dict()

    # loop search for the regular expression
    pos = 0
    while 1:
        match = tex_math_re.search(html, pos)
        if match:
            tex_expr = match.group(1)
            if tex_lookup.has_key(tex_expr):
                # already encountered this expression, retrive the file
                filename = tex_lookup[tex_expr]
            else:
                # we must convert to png
                filename = output_basename + "-eq" + str(i)
                i += 1
                print "   " + filename + ".png"
                from_tex(tex_expr, filename)
                # register into the lookup
                tex_lookup[tex_expr] = filename
            #html = tex_math_re.sub("<img class=inline-math src=\"" + os.path.basename(filename) + ".png\">", html, 1)
            html = tex_math_re.sub("<img src=\"" + os.path.basename(filename) + ".png\">", html, 1)
        else:
            break

    # Save the file
    output_html = output_basename + ".html"
    try:
        f = open(output_html, 'w')
    except IOError:
        print >> sys.stderr, "Unable to write the result to: {0}".format(output_html)
        return
    else:
        f.write(html)
        f.close()

# Test the module
if __name__ == "__main__":
#    from_tex(r"$\frac{a}{b} = c$", "test", 200)
#    from_html("test.html", "test-out")
    from_html("en_asebastdnative.html", "test-out")
