//
//  StringExtension.swift
//  ITorrentFramework-iOS
//
//  Created by Daniil Vinogradov on 01.08.2020.
//  Copyright © 2020 NoNameDude. All rights reserved.
//

import Foundation

extension String {
    func cString() -> UnsafeMutablePointer<Int8> {
        let count = self.utf8.count + 1
        let result = UnsafeMutablePointer<Int8>.allocate(capacity: count)
        self.withCString { (baseAddress) in
            result.initialize(from: baseAddress, count: count)
        }
        return result
    }
}
