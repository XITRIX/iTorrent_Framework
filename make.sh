#!/bin/sh

#  make.sh
#  ITorrentFramework
#
#  Created by Даниил Виноградов on 20.11.2022.
#  Copyright © 2022 NoNameDude. All rights reserved.

xcodebuild archive \
 -scheme ITorrentFramework-iOS \
 -archivePath ./ITorrentFramework-iphonesimulator.xcarchive \
 -sdk iphonesimulator \
 SKIP_INSTALL=NO

xcodebuild archive \
 -scheme ITorrentFramework-iOS \
 -archivePath ./ITorrentFramework-iphoneos.xcarchive \
 -sdk iphoneos \
 SKIP_INSTALL=NO

xcodebuild -create-xcframework \
 -framework ./ITorrentFramework-iphonesimulator.xcarchive/Products/Library/Frameworks/ITorrentFramework.framework \
 -framework ./ITorrentFramework-iphoneos.xcarchive/Products/Library/Frameworks/ITorrentFramework.framework \
 -output ./ITorrentFramework.xcframework
