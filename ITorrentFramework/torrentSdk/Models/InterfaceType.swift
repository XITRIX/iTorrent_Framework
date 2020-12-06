//
//  InterfaceType.swift
//  ITorrentFramework
//
//  Created by Daniil Vinogradov on 23.09.2020.
//  Copyright © 2020 NoNameDude. All rights reserved.
//

import Foundation

public enum InterfaceType {
    case all
    case primary
    case vpnOnly
    case manual(name: String)

    fileprivate var raw: String {
        switch self {
        case .all: return "all"
        case .primary: return "primary"
        case .vpnOnly: return "vpnOnly"
        case .manual(name: let name): return name
        }
    }
}

extension InterfaceType: Codable {
    public init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        let raw = try container.decode(String.self)
        switch raw {
        case "all":
            self = .all
        case "primary":
            self = .primary
        case "vpnOnly":
            self = .vpnOnly
        default:
            self = .manual(name: raw)
        }
    }

    public func encode(to encoder: Encoder) throws {
        var container = encoder.singleValueContainer()
        try container.encode(self.raw)
    }
}
