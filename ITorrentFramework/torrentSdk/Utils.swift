//
//  Utils.swift
//  ITorrentFramework-iOS
//
//  Created by Daniil Vinogradov on 01.08.2020.
//  Copyright © 2020 NoNameDude. All rights reserved.
//

import Foundation

class Utils {
    class func interfaceNames() -> [String] {
        let MAX_INTERFACES = 128

        var interfaceNames = [String]()
        let interfaceNamePtr = UnsafeMutablePointer<Int8>.allocate(capacity: Int(IF_NAMESIZE))
        for interfaceIndex in 1 ... MAX_INTERFACES {
            if if_indextoname(UInt32(interfaceIndex), interfaceNamePtr) != nil {
                let interfaceName = String(cString: interfaceNamePtr)
                interfaceNames.append(interfaceName)
            } else {
                break
            }
        }

        interfaceNamePtr.deallocate()
        return interfaceNames
    }
    
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
