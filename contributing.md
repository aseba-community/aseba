# Contributing guidelines

## Reporting bugs and suggesting improvements

Bugs and improvements are tracked using [github's issues](https://guides.github.com/features/issues/).
If you found a bug or wish to suggest an improvement, please first look at the [existing issues](https://github.com/aseba-community/aseba/issues) to see if it has already been reported.
If not, please [open a new issue](https://github.com/aseba-community/aseba/issues/new).

If you want to help us translating Aseba or its documentation, please read the [localization guide](localization.md).

To reach us, join the [development mailing list](https://mail.gna.org/listinfo/aseba-dev).

## Software development work flow

Aseba is using [git](https://git-scm.com/) and the repository is stored on [github](https://github.com/aseba-community/aseba).
Please read the [description of the source tree](readme.sourcetree.md) before modifying the source code.

To integrate contributions, we work through [github's pull request mechanism](https://help.github.com/articles/about-pull-requests/).
To contribute to Aseba, [fork the repository](https://github.com/aseba-community/aseba#fork-destination-box) and submit pull requests.
The maintainer will comment on your pull request, a discussion will follow, and if an agreement is reached and requested changes are implemented, your contribution will be merged.

## Roadmap

The maintainer, in discussion with the stakeholders of the project on the mailing list, defines the project roadmap as a set of [milestones](https://github.com/aseba-community/aseba/milestones).
Each milestone collects issues that need to be solved or implemented to reach the milestone.

If an issue is assigned to a contributor, she or he is working on it.
Feel free to comment on the issue if you have elements to add.
If you want to work on an issue that is not assigned, please go on, assign yourself.
The maintainer might overrun assignation in case of critical issues, although that should not happen often.

### Commit Policy

We warmly welcome patches and contributions.
To maintain a proper and easy-to-debug code base, commits must be clean.
So please check twice your commits, in order to ensure a smooth debugging process should a bug arise. Especially:
* Your commit must always compile.
* Your commit must work by itself.
* Your commits should be organized in a logical set of atomically small changes. Commit as often as possible.

Do not forget to include all your files (`git add`), check twice before committing (`git status`).
Do rework your commit history if not satisfactory enough (`git rebase -i`).
Ask others if you are not sure about using git.

### Pull request policy

Your pull request must pass all unit tests.
When changes are pushed into the github repository, we will not be able to fix them flawlessly, but we may revert them if they are too harmful.
You might be asked to rework your commit history to produce a cleaner one before your pull request is merged.
Thank you for your understanding, by doing so you also help other contributors.

## Release process

Releasing a software smoothly is not completely trivial.
This section contains the best practice for the Aseba release process, it should be updated according to the evolution of the process.

### Branch/tag naming

We follow these naming conventions:
* `master` : stable development branch, no funky experimental features here.
* `release-M.N.y` : branch for applying bugfix to last major release for Aseba version M.N, for instance release-1.1.x.
* `X.Y.Z` : tags corresponding to releases, for instance 1.0.1.

### Reminder

* Do not forget to bump version numbers in `common/consts.h`, in the library version numbers in the `.so`, in the `Doxyfile`, and for the packages (deb,rpm) rules before doing a release.
* When you make changes, including bug fixes to release branches, do not forget to update the release notes.

## C++ coding style

To be written...

## Naming conventions

The following conventions apply in the Aseba context:
* Always use similar names as in the documentation for [Aseba concepts](https://www.thymio.org/en:asebaconcepts), [the language](https://www.thymio.org/en:asebalanguage), [the native functions](https://www.thymio.org/en:asebastdnative), the [integrated development environment](https://www.thymio.org/en:asebastudio) and the [Thymio visual programming environment](https://www.thymio.org/en:thymiovpl).
* In Aseba, one writes _programs_, being them text or graphical. The old use _script_ for Aseba text programs is deprecated.
