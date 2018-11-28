#!/usr/bin/python

import os
import shutil
import sys
from subprocess import call

timestamp_server = "http://timestamp.digicert.com"
url = "http://http://www.mobsya.org/"

binary = sys.argv[1] if len(sys.argv) > 1 else None
exists = binary is not None and os.path.isfile(binary)

scp = os.environ.get("SIGNTOOL_SCP")
key = os.environ.get("SIGNTOOL_KEY")
pfx = os.environ.get("SIGNTOOL_PFX")
pss = os.environ.get("SIGNTOOL_PASSPHRASE")

if binary and not exists:
    print("File {} does not exist".format(binary))
    exit(1)

elif not exists or not ((scp and key) or (pss and pfx)):
    print("""
Usage {} <binary>

Sign a binary with either osslsigncode or signcode(win32)

To sign with osslsigncode, the following environnement variables must be defined

 * SIGNCODE_SCP : Must be the path of a valid scp file (Software Certificate Publisher File)
 * SIGNCODE_KEY : Must be the path of a valid key file (PEM/DER)

To sign with signtool, the following environnement variables must be defined

 * SIGNTOOL_PFX : Must be the path of a valid pfx file
 * SIGNTOOL_PASSPHRASE : Passphrase of the PFX file
""")
    exit(1)

_, ext = os.path.splitext(binary)
if ext[1:] not in ['exe', 'dll']:
    print("Not a signable file, ignoring")
    exit(0)

# signtool ( native windows)
if pss and pfx:
    print("Signing with signtool")
    if not os.path.isfile(pfx):
        print("signtool: No PFX File Found ({} does not exist)".format(pfx))
        exit(1)
    ret = call([
        "signtool",
        "sign"
        "/f",
        pfx,
        "/p",
        pss,
        "/fd",
        "sha256",
        "/td",
        "sha256",
        "/du",
        url,
        "/tr",
        timestamp_server,
        "/as",
        binary,
    ])
    if ret != 0:
        print("signtool failed (ret {})".format(ret))
    exit(ret)

# signcode
if scp and key:
    print("Signing with osslsigncode")
    if not os.path.isfile(scp):
        print("signcode: No SCP File Found ({} does not exist)".format(scp))
        exit(1)
    if not os.path.isfile(key):
        print("signcode: No KEY File Found ({} does not exist)".format(key))
        exit(1)

    out = binary + ".signed"

    ret = call([
        "osslsigncode", "-spc", scp, "-key", key, "-h", "sha256", "-i", url, "-t", timestamp_server, "-in", binary,
        "-out", out
    ])
    if (os.path.isfile(out)):
        shutil.move(out, binary)
    if ret != 0:
        print("osslsigncode failed (ret {})".format(ret))
    exit(ret)
