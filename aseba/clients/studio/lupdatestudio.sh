#!/bin/bash
# update all translation ts files from the source code of Studio
lupdate *.cpp *.h plugins/*.h plugins/*.cpp plugins/*/*.h plugins/*/*.cpp -ts asebastudio*.ts
