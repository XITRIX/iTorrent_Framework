#!/bin/sh

mkdir build

rm -r ./build/ITorrentFramework-iphonesimulator.xcarchive
rm -r ./build/ITorrentFramework-iphoneos.xcarchive
rm -r ./build/ITorrentFramework-xrsimulator.xcarchive
rm -r ./build/ITorrentFramework-xros.xcarchive
rm -r ./build/ITorrentFramework.xcframework

xcodebuild archive \
 -scheme ITorrentFramework \
 -archivePath ./build/ITorrentFramework-iphonesimulator.xcarchive \
 -sdk iphonesimulator \
 SKIP_INSTALL=NO

xcodebuild archive \
 -scheme ITorrentFramework \
 -archivePath ./build/ITorrentFramework-iphoneos.xcarchive \
 -sdk iphoneos \
 SKIP_INSTALL=NO

xcodebuild archive \
 -scheme ITorrentFramework \
 -archivePath ./build/ITorrentFramework-xrsimulator.xcarchive \
 -sdk xrsimulator \
 SKIP_INSTALL=NO

xcodebuild archive \
 -scheme ITorrentFramework \
 -archivePath ./build/ITorrentFramework-xros.xcarchive \
 -sdk xros \
 SKIP_INSTALL=NO

xcodebuild -create-xcframework \
 -framework ./build/ITorrentFramework-iphonesimulator.xcarchive/Products/Library/Frameworks/ITorrentFramework.framework \
 -framework ./build/ITorrentFramework-iphoneos.xcarchive/Products/Library/Frameworks/ITorrentFramework.framework \
 -framework ./build/ITorrentFramework-xrsimulator.xcarchive/Products/Library/Frameworks/ITorrentFramework.framework \
 -framework ./build/ITorrentFramework-xros.xcarchive/Products/Library/Frameworks/ITorrentFramework.framework \
 -output ./build/ITorrentFramework.xcframework


xcodebuild -create-xcframework \
 -framework ./build/ITorrentFramework-iphonesimulator.xcarchive/Products/Library/Frameworks/ITorrentFramework.framework \
 -framework ./build/ITorrentFramework-iphoneos.xcarchive/Products/Library/Frameworks/ITorrentFramework.framework \
 -framework ./build/ITorrentFramework-xrsimulator.xcarchive/Products/Library/Frameworks/ITorrentFramework.framework \
 -output ./build/ITorrentFramework.xcframework

# rm -r ./build/ITorrentFramework-iphonesimulator.xcarchive
# rm -r ./build/ITorrentFramework-iphoneos.xcarchive
# rm -r ./build/ITorrentFramework-xrsimulator.xcarchive
# rm -r ./build/ITorrentFramework-xros.xcarchive
