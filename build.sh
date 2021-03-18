#!/bin/bash
set -e

mkdir -p tools && pushd tools
wget -N https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget -N https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
wget -N https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage
wget -N https://github.com/AppImage/AppImageUpdate/releases/download/continuous/appimageupdatetool-x86_64.AppImage
chmod +x *.AppImage
popd

rm -r build | true
mkdir -p build && cp ./resources/Garuda-Downloader.desktop ./build/ && cp ./resources/garuda.svg ./build/garuda-downloader.svg && cd build
cmake .. && make
../tools/linuxdeploy-x86_64.AppImage --plugin qt --executable=./garuda-downloader --appdir=./appdir/ --desktop-file ./Garuda-Downloader.desktop --icon-file ./garuda-downloader.svg
../tools/appimageupdatetool-x86_64.AppImage --appimage-extract
cp -r ./squashfs-root/ ./appdir/usr/bin/appimageupdatetool
rm -r ./appdir/usr/translations | true
rm -r ./appdir/usr/doc | true
rm -r ./appdir/usr/bin/appimageupdatetool/resources | true
rm -r ./appdir/usr/bin/appimageupdatetool/usr/share/icons | true
rm -r ./appdir/usr/bin/appimageupdatetool/usr/share/doc | true
rm -r ./squashfs-root | true
\cp --remove-destination ./appdir/usr/share/applications/Garuda-Downloader.desktop ./appdir
UPDATE_INFORMATION="zsync|https://builds.garudalinux.org/garuda-downloader/latest/Garuda_Downloader-x86_64.AppImage.zsync" ../tools/linuxdeploy-plugin-appimage-x86_64.AppImage --appdir ./appdir
