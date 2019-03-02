#!/bin/bash

export VERSION=1_0_0

export PATH_MAC=/Applications
export PATH_WIN=~/Shared/
export PATH_LINUX=~/Shared/


\rm -rf *.zip *.gz

echo "#####################################################"
echo "# MacOS"
echo "#####################################################"
cp -r ${PATH_MAC}/BLM.app .
zip -r BLM_${VERSION}.app.zip BLM.app
rm -rf BLM.app


echo "#####################################################"
echo "# Windows"
echo "#####################################################"
ls -la ${PATH_WIN}/BLM.exe
cp -r ${PATH_WIN}/BLM.exe BLM.exe
zip -r BLM_${VERSION}.zip BLM.exe
rm -rf BLM.exe

echo "#####################################################"
echo "# Linux"
echo "#####################################################"
ls -la ${PATH_LINUX}/BLM
cp -r ${PATH_LINUX}/BLM .
tar cfvz BLM_${VERSION}.tar.gz BLM
rm -rf BLM
