#!/bin/sh

cmake -B build -DPLATFORM=OS64

make -C build -j8

xcodebuild archive \
 -scheme ITorrentFramework \
 -archivePath ./build/ITorrentFramework-iphoneos.xcarchive \
 -sdk iphoneos \
 SKIP_INSTALL=NO
