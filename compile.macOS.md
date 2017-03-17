# Compile Aseba on macOS

It is very easy to compile from source Aseba for OS X, using [homebrew](https://brew.sh).

## Preliminary, check your shell

Open a terminal and check which shell you have by typing:

    echo $SHELL

If the answer is `/bin/tcsh`, please run the commands in bash. To do so, type:

    bash

Otherwise skip this step and continue.

## Preliminary, install homebrew

You should not have ad hoc package system installed such as MacPorts.
To install homebrew, run the following command:

    ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    
## Add the Aseba repository

In the terminal, run the following command:

    brew tap stephanemagnenat/aseba

## Install Aseba

You can choose to install either the stable version or the latest development version.
In the terminal, run the following command:

### For the stable version

    brew install aseba

### For the development version

    brew reinstall --HEAD aseba
