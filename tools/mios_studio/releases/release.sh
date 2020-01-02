#!/bin/bash

export VERSION=2_4_9

export PATH_MAC=/Applications
export PATH_WIN=~/Shared/
export PATH_LINUX=~/Shared/


\rm -rf *.zip *.gz

echo "#####################################################"
echo "# MacOS"
echo "#####################################################"
cp -r ${PATH_MAC}/MIOS_Studio.app .
zip -r MIOS_Studio_${VERSION}.app.zip MIOS_Studio.app
rm -rf MIOS_Studio.app


echo "#####################################################"
echo "# Windows"
echo "#####################################################"
ls -la ${PATH_WIN}/MIOS_Studio.exe
cp -r ${PATH_WIN}/MIOS_Studio.exe MIOS_Studio.exe
zip -r MIOS_Studio_${VERSION}.zip MIOS_Studio.exe
rm -rf MIOS_Studio.exe

echo "#####################################################"
echo "# Linux"
echo "#####################################################"
ls -la ${PATH_LINUX}/MIOS_Studio
cp -r ${PATH_LINUX}/MIOS_Studio .
tar cfvz MIOS_Studio_${VERSION}.tar.gz MIOS_Studio
rm -rf MIOS_Studio
