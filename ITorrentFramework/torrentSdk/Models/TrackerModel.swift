//
//  TrackerModel.swift
//  iTorrent
//
//  Created by Daniil Vinogradov on 30.03.2020.
//  Copyright © 2020  XITRIX. All rights reserved.
//

import Foundation

public struct TrackerModel: Hashable {
    public var url: String
    public var message: String
    public var seeders: Int32
    public var peers: Int32
    public var leechs: Int32
    public var working: Bool
    public var verified: Bool
    
    public init(_ tracker: Tracker) {
        url = String(cString: tracker.tracker_url)
        seeders = tracker.seeders
        peers = tracker.peers
        leechs = tracker.leechs
        working = tracker.working == 1
        verified = tracker.verified == 1
        message = working ? NSLocalizedString("Working", comment: "") : NSLocalizedString("Inactive", comment: "")
        if verified { message += ", \(NSLocalizedString("Verified", comment: ""))" }
    }
    
    public func hash(into hasher: inout Hasher) {
        hasher.combine(url)
    }
}
