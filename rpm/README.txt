To build RPMs you need to have the rpmdevtools package installed and
an RPM building environment configured with rpmdev-setuptree. Then, for each 
release, do the following:

To create RPMs from an existing github release
---------------------------------------------

0. For what follows, use the following definitions. You can even set shell
variables accordingly so you can just copy paste commands.

PACKAGE_NAME=<the name of the package>
SOURCE_RELEASE_TAG=<the MAJOR.MINOR.PATCH git tag for the source release>

1. [NOTE: Ideally, this would be done before making the source release.]
If there have been any ABI-breaking changes to a shared library 
since the last RPM release, bump the LIB_VERSION_MAJOR number that is
set in the CMakeLists.txt file in the directory containing library
source or in a higher-level directory. The line looks like this:

set(LIB_VERSION_MAJOR 1) # Must be bumped for incompatible ABI changes

2. Determine the commit hash of the source release either by copying it
from the github releases web page or running:

    git show-ref -s --tags $SOURCE_RELEASE_TAG

3. Follow the directions in the spec file for updating the release information.

4. Run:

    rpmdev-bumpspec -u "Your Name <your@email>" \
        -c "changelog comment" ${PACKAGE_NAME}.spec

That will increment the RPM release number and add a changelog entry to the
spec file.

5. Download the appropriate sources into ~/rpmbuild/SOURCES with:

    spectool -g -R ${PACKAGE_NAME}.spec

6. Create a patch file containing all the changes since the specified commit 
by running:

    git diff $GITHUB_RELEASE_NUMBER \
	> ~/rpmbuild/SOURCES/${PACKAGE_NAME}-rpm.patch

7. Build the RPMs and SRPM with:

    rpmbuild -ba ${PACKAGE_NAME}.spec

If there are problems, fix them, update the most recent changelog entry
in the %changelog section at the bottom of the spec file and return to step 6.

8. Commit and push the updated spec file and your changes to the official
github repository.

9. Publish the RPMs and SRPM found in ~/rpmbuild/RPMS and ~/rpmbuild/SRPMS


To create pre-release or post-release RPMs 
------------------------------------------

If you want to create pre-release RPMs for an upcoming github 
source release or post-release RPMS based on a commit that
occurred after the most recent github source release, follow
the directions above with the following changes:

Instead of step 2, determine the commit hash from the official
github repository that you want the RPMs to be based on.

When editing the spec file in step 3, set the "Version:" tag to the
future github source release number (for pre-release RPMs) or past 
github source release number (for post-release RPMS) and follow the 
other directions in that file concerning pre-release and post-release 
RPMs.


