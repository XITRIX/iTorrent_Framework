//
//  PeerModel.swift
//  iTorrent
//
//  Created by Daniil Vinogradov on 28.06.2020.
//  Copyright © 2020  XITRIX. All rights reserved.
//

import Foundation

public struct PeerModel: Hashable {
    public var port: Int
    public var client: String
    public var totalDownload: Int64
    public var totalUpload: Int64
    public var upSpeed: Int
    public var downSpeed: Int
    public var connectionType: Int
    public var progress: Int
    public var progressPpm: Int
    public var address: String

    public init(_ model: Peer) {
        port = Int(model.port)
        client = String(validatingUTF8: model.client) ?? "ERROR"
        totalDownload = model.total_download
        totalUpload = model.total_upload
        upSpeed = Int(model.up_speed)
        downSpeed = Int(model.down_speed)
        connectionType = Int(model.connection_type)
        progress = Int(model.progress)
        progressPpm = Int(model.progress_ppm)
        address = String(validatingUTF8: model.address) ?? "ERROR"
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(address)
    }

    public static func == (lhs: Self, rhs: Self) -> Bool {
        lhs.address == rhs.address
    }
}
