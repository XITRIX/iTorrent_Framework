//
//  Utils.swift
//  ITorrentFramework-iOS
//
//  Created by Daniil Vinogradov on 01.08.2020.
//  Copyright © 2020 NoNameDude. All rights reserved.
//

import Foundation

class Utils {
    class func withArrayOfCStrings<R>(
        _ args: [String],
        _ body: ([UnsafeMutablePointer<CChar>?]) -> R
    ) -> R {
        var cStrings = args.map {
            strdup($0)
        }
        cStrings.append(nil)
        defer {
            cStrings.forEach {
                free($0)
            }
        }
        return body(cStrings)
    }
}
