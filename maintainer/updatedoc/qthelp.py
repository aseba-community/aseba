import os
import sys
from string import Template

import wikidot.structure
from wikidot.tree import WikiNode

qhp_template = \
"""<?xml version="1.0" encoding="UTF-8"?>
<QtHelpProject version="1.0">
        <namespace>aseba.main</namespace>
        <virtualFolder>doc</virtualFolder>

${filter_def}

${filter_section}

</QtHelpProject>
"""

filter_def_template = \
"""        <customFilter name="${lang}">
                <filterAttribute>lang-${lang}</filterAttribute>
        </customFilter>
"""

filter_section_template = \
"""
        <filterSection>
                <filterAttribute>lang-${lang}</filterAttribute>
                <toc>
${sections}
                </toc>

                <files>
${files}
                </files>
        </filterSection>
"""


section_root_template = \
"""                        <section title="${title}" ref = "${ref}">
${subsection}
                        </section>
"""
section_root = Template(section_root_template)


section_leaf_template = \
"""                                <section title="${title}" ref="${ref}"></section>
"""
section_leaf = Template(section_leaf_template)


file_element_template = \
"""                        <file>${filename}</file>
"""


item_template = """	<file>${filename}</file>\n"""

def __generate_sections__(node, output_directory):
    result = ""
    filename = os.path.join(output_directory, node.link)
    if len(node.children) == 0:
        # we are a leaf
        result += section_leaf.substitute(title=node.title, ref=filename)
        return result
    # pass through the children
    subsections = ""
    for x in node.children:
        subsections += __generate_sections__(x, output_directory)
    # assemble
    if node.link == '':
        # this node is "virtual" (maybe the root?)
        return subsections
    else:
        result += section_root.substitute(title=node.title, ref=filename, subsection=subsections)
        return result

def generate(source_directories, output_directory, languages, qhp_file):
    """
    directory / language should be lists
    """

    print >> sys.stderr, "\nCreating {}, based on the content of {}...".format(qhp_file, source_directories)

    qhp = Template(qhp_template)
    filterdefs = ""
    filterdef = Template(filter_def_template)
    filtersections = ""
    filtersection = Template(filter_section_template)
    for index, directory in enumerate(source_directories):
        lang = languages[index]
        # Build the filter definitions
        filterdefs += filterdef.substitute(lang=lang)
        # Build the sections listing
        tree = wikidot.structure.get_structure(lang)
        sections = __generate_sections__(tree, output_directory)
        # List files
        files = [x for x in os.listdir(directory)]
        files.sort()
        filelist = ""
        fileitem = Template(file_element_template)
        for x in files:
            filename = os.path.join(output_directory, x)
            #print >> sys.stderr, "Processing ", filename
            filelist += fileitem.substitute(filename=filename)
        # Generate the chunk for the current language
        filtersections += filtersection.substitute(lang=lang, files=filelist, sections=sections)

    f = open(qhp_file, 'w')
    f.write(qhp.substitute(filter_def=filterdefs, filter_section=filtersections))
    f.close
    print >> sys.stderr, "Done."

