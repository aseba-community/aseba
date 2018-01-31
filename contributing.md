# Contributing guidelines

## Reporting bugs and suggesting improvements

Bugs and improvements are tracked using [github's issues](https://guides.github.com/features/issues/).
If you find a bug or wish to suggest an improvement, please first look through the [existing issues](https://github.com/aseba-community/aseba/issues) to see whether it has already been reported.
If not, please [open a new issue](https://github.com/aseba-community/aseba/issues/new).

To contact the Aseba team, please open an issue with your question. Referring to a person through an [@mention](https://guides.github.com/features/mastering-markdown/#GitHub-flavored-markdown) in an issue is better than sending a direct e-mail, because it allows another team member to answer the question, and it allows another user with the same question to discover it in the [closed issues](https://github.com/aseba-community/aseba/issues?q=is%3Aissue+is%3Aclosed).

If you are a developer and have found a bug, please consider contributing a test. The most helpful way to do this is to submit a pull request with a new test in the [`tests`](https://github.com/aseba-community/aseba/tree/master/tests) directory.

If you want to help us translating Aseba or its documentation, please read the [localization guide](localization.md).

## Software development work flow

Aseba uses [git](https://git-scm.com/) and the repository is stored on [github](https://github.com/aseba-community/aseba).
Please read the [description of the source tree](readme.sourcetree.md) before modifying the source code.

To integrate contributions, we work through [github's pull request mechanism](https://help.github.com/articles/about-pull-requests/).
To contribute to Aseba, [fork the repository](https://github.com/aseba-community/aseba#fork-destination-box) and submit pull requests.
The maintainer will comment on your pull request, a discussion will follow, and if an agreement is reached and requested changes are implemented, your contribution will be merged.

## Roadmap

The maintainer, in discussion with the stakeholders of the project on the mailing list, defines the project roadmap as a set of [milestones](https://github.com/aseba-community/aseba/milestones).
Each milestone collects issues that need to be solved or implemented to reach the milestone.

If an issue is assigned to a contributor, she or he is working on it.
Feel free to comment on the issue if you have elements to add.
If you want to work on an issue that is not assigned, please go ahead, assign yourself.
The maintainer might override assignment in the case of critical issues, although that should not happen often.

### Commit Policy

We warmly welcome patches and contributions.
To maintain a proper and easy-to-debug code base, commits must be clean.
So please check your commits twice, in order to ensure a smooth debugging process should a bug arise. Especially:

* Your commit must always compile.
* Your commit must work by itself.
* Your commits should be organized in a logical set of small atomic changes. Commit as often as possible.

Do not forget to include all your files (`git add`), check twice before committing (`git status`).
Do rework your commit history to group or combine related commits (`git rebase -i`).
Ask others for help if you are not sure about using git.

### Pull request policy

Pull requests are always welcome, but we ask you to understand that they may impact the work of many people and may affect long-term maintenance of Aseba. We actively avoid quick fixes that will create a need for maintenance in the future.

We will comment on your pull request but will not modify it ourselves. You might be asked to rework your commit history to produce a cleaner one before your pull request is merged. Please be understanding with these requests: by doing so you also help other contributors.

As a general guideline, in order for a pull request to be accepted, it:

1. **Must** be mergeable in the *master* branch
2. **Must** compile and pass all unit tests
3. **Must not** introduce regressions (unit tests or user testing)
4. **Should** contain a single, understandable improvement
5. **Should not** require more that 30 minutes of review by the maintainers
6. **Should** succeed in Aseba's continuous integration
7. **Should** have a clean commit history, from which individual commits may be cherry-picked

These guidelines help us focus our efforts. If we close a pull request summarily for one of these reasons, you can always ask to reopen it if changes have been made. You may find it necessary to break up a large pull request into more manageable pieces.

A special case are "work in progress" pull requests, which are created for discussion and are not intended to be merged. The title for such pull requests must start with "**WIP:**". This prefix will be removed when the pull request finally meets the guidelines and is ready for consideration.

## Release process

Releasing software smoothly is complicated.
This section contains the best practice for the Aseba release process, it should be updated according to the evolution of the process.

### Branch/tag naming

We follow these naming conventions:

* `master` : stable development branch, no funky experimental features here.
* `release-M.N.y` : branch for applying bugfix to last major release for Aseba version M.N, for instance release-1.1.x.
* `X.Y.Z` : semantic versioning tags corresponding to releases, for instance 1.0.1.

### Reminder

* Do not forget to bump version numbers in `common/consts.h`, in the library version numbers in the `.so`, in the `Doxyfile`, and for the packages (deb,rpm) rules before doing a release.
* When you make changes, including bug fixes to release branches, do not forget to update the [changelog](changelog.md) following the guidelines on [keepachangelog.com](http://keepachangelog.com).

## C++ coding style

The intended purpose is to have coherent-looking code through the project.
Sorry if this different to your usual conventions.
Aseba is currently programming in C++14.

### Readability and maintainability are paramount

All new code written in Aseba should be elegant and its behavior obvious.
Do not use clever tricks, avoid over-design, do not be obsessed about pure object-oriented style or getters/setters.
Everything should be kept **as simple as possible, but no simpler.**
This is a fundamental principle, and all new code should be kept with KISS—keep it simple, stupid—in mind.

### Pointers, references and ownership

Please follow as much as possible the [resource acquisition is initialization](https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization) programming idiom.
Whenever possible, use simple to understand and exception-safe ownership, such as `std::unique_ptr<T>`.
When passing objects to non-owning classes and methods, use a reference if the object cannot be `nullptr`, or a normal pointer if it can.
If a reference cannot be used because the target objects might be copyable, use `std::reference_wrapper<T>`.
Use STL containers as much as possible, and avoid duplicating the information whenever possible.

### Class hierarchy and inheritance

Use simple and clean class hierarchy whenever possible, use virtual inheritance only in the last resort.
If a member can simply be accessed as public, let it so, do not put it private and add accessors.
Always put a virtual destructor in base classes having virtual members, you can generate the default one using `virtual ~C() = default;`.
Always use `override` keyword for inherited virtual methods, do no re-specify `virtual`, as an overriden method must be virtual.

### File formats

Source files must use Unix End-Of-Line characters (\n) and be encoded in UTF-8.
Header files have included guards of the form `__FILE_NAME_H` for a file named `FileName.h`.

### Indentation

Code should be indented by tabs.
A tab should be equivalent to 4 spaces.
Empty lines should not have any whitespace.

### Class and struct naming

Aseba uses [CamelCase](https://en.wikipedia.org/wiki/Camel_case), that is, classes should begin with capitals, new words should be denoted by a capital, and words should not be separated by underscores or dashes.
For example, use:
```C++
class TotoIsMyFriend { };     // correct
```
instead of:
```C++
class toto_is_my_friend{};    // wrong
```
Class members, variables, and C function follow the same convention but without the initial capital: 
```C++
void totoIsMyFriend(void);    // correct
```
instead of:
```C++
void Toto-is-my-friend(void); // wrong
```

### Bracket placement

Brackets `{}` should be placed alone on a line, except if there is no or only one function call within them.
For example, use:
```C++
class Toto
{
    int i;
    void totoIsMyFriend(void) { return i; };
}; // correct
```
instead of:
```C++
class Toto {
    int i;
    void Toto-is-my-friend(void) {
        return i;
    };
}; // wrong
```

## Naming conventions

The following conventions apply in the Aseba context:

* Always use similar names as in the documentation for [Aseba concepts](https://www.thymio.org/en:asebaconcepts), [the language](https://www.thymio.org/en:asebalanguage), [the native functions](https://www.thymio.org/en:asebastdnative), the [integrated development environment](https://www.thymio.org/en:asebastudio) and the [Thymio visual programming environment](https://www.thymio.org/en:thymiovpl).
* In Aseba, one writes _programs_, whether they are text or graphical. The old use of _script_ for Aseba text programs is deprecated.
