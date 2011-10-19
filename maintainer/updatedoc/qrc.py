import os
import sys
from string import Template

qrc_template = \
"""
<!DOCTYPE RCC><RCC version="1.0">
<qresource>
${filelist}
</qresource>
</RCC>
"""

item_template = """	<file>${filename}</file>\n"""

def generate(directory, qrc_file):
    files = [x for x in os.listdir(directory)]
    files.sort()
    print >> sys.stderr, "\nCreating {}, based on the content of {}...".format(qrc_file, directory)
    items = ""
    item = Template(item_template)
    qrc = Template(qrc_template)
    for x in files:
        filename = os.path.join(directory, x)
        print >> sys.stderr, "Processing ", filename
        items += item.substitute(filename=filename)

    f = open(qrc_file, 'w')
    f.write(qrc.substitute(filelist=items))
    f.close
    print >> sys.stderr, "Done."

