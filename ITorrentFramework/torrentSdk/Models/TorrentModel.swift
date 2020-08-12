//
//  TorrentModel.swift
//  iTorrent
//
//  Created by Daniil Vinogradov on 30.03.2020.
//  Copyright © 2020  XITRIX. All rights reserved.
//

import Foundation

public class TorrentModel: Hashable {
    public var title: String = ""
    public var state: TorrentState = .null
    public var displayState: TorrentState = .null
    public var hash: String = ""
    public var creator: String = ""
    public var comment: String = ""
    public var progress: Float = 0
    public var totalWanted: Int64 = 0
    public var totalWantedDone: Int64 = 0
    public var downloadRate: Int = 0
    public var uploadRate: Int = 0
    public var totalDownloadSession: Int64 = 0
    public var totalDownload: Int64 = 0
    public var totalUploadSession: Int64 = 0
    public var totalUpload: Int64 = 0
    public var numSeeds: Int = 0
    public var numPeers: Int = 0
    public var numLeechers: Int = 0
    public var numTotalSeeds: Int = 0
    public var numTotalPeers: Int = 0
    public var numTotalLeechers: Int = 0
    public var totalSize: Int64 = 0
    public var totalDone: Int64 = 0
    public var creationDate: Date?
    public var addedDate: Date?
    public var isPaused: Bool = false
    public var isFinished: Bool = false
    public var isSeed: Bool = false
    public var seedMode: Bool = false
    public var seedLimit: Int64 = 0
    public var hasMetadata: Bool = false
    public var numPieces: Int = 0
    public var pieces: [Int] = []
    public var sequentialDownload: Bool = false

    public init(_ torrentInfo: TorrentInfo) {
        state = TorrentState(rawValue: String(validatingUTF8: torrentInfo.state) ?? "") ?? .null
        title = String(validatingUTF8: torrentInfo.name) ?? (state == .metadata ? NSLocalizedString("Obtaining Metadata", comment: "") : "ERROR")
        hash = String(validatingUTF8: torrentInfo.hash) ?? "ERROR"
        creator = String(validatingUTF8: torrentInfo.creator) ?? "ERROR"
        comment = String(validatingUTF8: torrentInfo.comment) ?? "ERROR"
        progress = torrentInfo.progress
        totalWanted = torrentInfo.total_wanted
        totalWantedDone = torrentInfo.total_wanted_done
        downloadRate = Int(torrentInfo.download_rate)
        uploadRate = Int(torrentInfo.upload_rate)
        totalDownloadSession = torrentInfo.total_download
        totalUploadSession = torrentInfo.total_upload
        numSeeds = Int(torrentInfo.num_seeds)
        numPeers = Int(torrentInfo.num_peers)
        numLeechers = Int(torrentInfo.num_leechers)
        numTotalSeeds = Int(torrentInfo.num_total_seeds)
        numTotalPeers = Int(torrentInfo.num_total_peers)
        numTotalLeechers = Int(torrentInfo.num_total_leechers)
        totalSize = torrentInfo.total_size
        totalDone = torrentInfo.total_done
        creationDate = Date(timeIntervalSince1970: TimeInterval(torrentInfo.creation_date))
        isPaused = torrentInfo.is_paused == 1
        isFinished = torrentInfo.is_finished == 1
        isSeed = torrentInfo.is_seed == 1
        hasMetadata = torrentInfo.has_metadata == 1

        sequentialDownload = torrentInfo.sequential_download == 1
        numPieces = Int(torrentInfo.num_pieces)
        pieces = Array(UnsafeBufferPointer(start: torrentInfo.pieces, count: numPieces)).map({Int($0)})

        displayState = getDisplayState()
    }
    
    public func update(with model: TorrentModel) {
        let oldState = self.displayState
        
        state = model.state
        title = model.title
        creator = model.creator
        comment = model.comment
        progress = model.progress
        totalWanted = model.totalWanted
        totalWantedDone = model.totalWantedDone
        downloadRate = model.downloadRate
        uploadRate = model.uploadRate
        totalDownloadSession = model.totalDownloadSession
        totalUploadSession = model.totalUploadSession
        numSeeds = model.numSeeds
        numPeers = model.numPeers
        numLeechers = model.numLeechers
        numTotalSeeds = model.numTotalSeeds
        numTotalPeers = model.numTotalPeers
        numTotalLeechers = model.numTotalLeechers
        totalSize = model.totalSize
        totalDone = model.totalDone
        creationDate = model.creationDate
        isPaused = model.isPaused
        isFinished = model.isFinished
        isSeed = model.isSeed
        hasMetadata = model.hasMetadata

        sequentialDownload = model.sequentialDownload
        numPieces = model.numPieces
        pieces = model.pieces

        displayState = getDisplayState()
        
        if oldState != displayState {
            DispatchQueue.main.async {
                NotificationCenter.default.post(name: .torrentsStateChanged,
                                                object: nil,
                                                userInfo: ["data": (manager: self, oldState: oldState, newState: self.displayState)])
            }
        }
    }

    private func getDisplayState() -> TorrentState {
        if state == .finished || state == .downloading,
            isFinished, !isPaused, seedMode {
            return .seeding
        }
        if state == .seeding, isPaused || !seedMode {
            return .finished
        }
        if state == .downloading, isFinished {
            return .finished
        }
        if state == .downloading, !isFinished, isPaused {
            return .paused
        }
        return state
    }

    public func stateCorrector() {
        if displayState == .finished,
            !isPaused {
            TorrentSdk.stopTorrent(hash: hash)
        } else if displayState == .seeding,
            totalUpload >= seedLimit,
            seedLimit != 0 {
            seedMode = false
            TorrentSdk.stopTorrent(hash: hash)
        } else if state == .hashing, isPaused {
            TorrentSdk.startTorrent(hash: hash)
        }
    }
    
    public func hash(into hasher: inout Hasher) {
        hasher.combine(hash)
    }
    
    public static func == (lhs: TorrentModel, rhs: TorrentModel) -> Bool {
        lhs.state == rhs.state &&
        lhs.hash == rhs.hash &&
        lhs.progress == rhs.progress &&
        lhs.totalWanted == rhs.totalWanted &&
        lhs.totalWantedDone == rhs.totalWantedDone &&
        lhs.downloadRate == rhs.downloadRate &&
        lhs.uploadRate == rhs.uploadRate &&
        lhs.totalDownloadSession == rhs.totalDownloadSession &&
        lhs.totalUploadSession == rhs.totalUploadSession &&
        lhs.numSeeds == rhs.numSeeds &&
        lhs.numPeers == rhs.numPeers &&
        lhs.totalDone == rhs.totalDone &&
        lhs.isPaused == rhs.isPaused &&
        lhs.isFinished == rhs.isFinished &&
        lhs.isSeed == rhs.isSeed &&
        lhs.hasMetadata == rhs.hasMetadata &&
        lhs.sequentialDownload == rhs.sequentialDownload &&
        lhs.pieces == rhs.pieces
    }
}
