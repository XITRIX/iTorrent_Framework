//
//  SettingsPack.swift
//  iTorrent
//
//  Created by Daniil Vinogradov on 19.06.2020.
//  Copyright © 2020  XITRIX. All rights reserved.
//

import Foundation
import ITorrentFramework.Private

public enum EncryptionPolicy: Int8, CaseIterable, Codable {
    case enabled
    case forced
    case disabled
}

public enum ProxyType: Int, CaseIterable, Codable {
    case none
    case socks4
    case socks5
    case http
    case i2p_proxy

    public var title: String {
        switch self {
        case .none:
            return NSLocalizedString("None", comment: "")
        case .socks4:
            return NSLocalizedString("SOCKS4", comment: "")
        case .socks5:
            return NSLocalizedString("SOCKS5", comment: "")
        case .http:
            return NSLocalizedString("HTTP", comment: "")
        case .i2p_proxy:
            return NSLocalizedString("I2P", comment: "")
        }
    }
}

public struct SettingsPack {
    public var downloadLimit: Int
    public var uploadLimit: Int

    public var encryptionPolicy: EncryptionPolicy

    public var enableDht: Bool
    public var enableLsd: Bool
    public var enableUtp: Bool
    public var enableUpnp: Bool
    public var enableNatpmp: Bool

    public var enablePex: Bool

    public var maxActiveTorrents: Int
    public var maxDownloadingTorrents: Int
    public var maxUplodingTorrents: Int

    public var interfaceType: InterfaceType
//    public var outgoingInterfaces: String
//    public var listenInterfaces: String
    
    public var portRangeFirst: Int
    public var portRangeSecond: Int

    public var proxyType: ProxyType
    public var proxyRequiresAuth: Bool
    public var proxyHostname: String
    public var proxyPort: Int
    public var proxyUsername: String
    public var proxyPassword: String
    public var proxyPeerConnections: Bool

    public func asNative() -> settings_pack_struct {
        settings_pack_struct(download_limit: Int32(downloadLimit),
                             upload_limit: Int32(uploadLimit),
                             max_active_torrents_limit: Int32(maxActiveTorrents),
                             max_upload_torrents_limit: Int32(maxUplodingTorrents),
                             max_download_torrents_limit: Int32(maxDownloadingTorrents),
                             encryption_policy: encryption_policy_t(rawValue: UInt32(encryptionPolicy.rawValue)),
                             enable_dht: enableDht,
                             enable_lsd: enableLsd,
                             enable_utp: enableUtp,
                             enable_upnp: enableUpnp,
                             enable_natpmp: enableNatpmp,
                             enable_pex: enablePex,
                             outgoing_interfaces: outgoungInterfaces.cString(),
                             listen_interfaces: listenInterfaces.cString(),
                             port_range_first: Int32(portRangeFirst),
                             port_range_second: Int32(portRangeSecond),
                             proxy_type: proxy_type_t(rawValue: UInt32(proxyType.rawValue)),
                             proxy_requires_auth: proxyRequiresAuth,
                             proxy_hostname: proxyHostname.cString(),
                             proxy_port: Int32(proxyPort),
                             proxy_username: proxyUsername.cString(),
                             proxy_password: proxyPassword.cString(),
                             proxy_peer_connections: proxyPeerConnections)
    }
    
    private var outgoungInterfaces: String {
        let interfaces = Utils.interfaceNames()
        
        switch interfaceType {
        case .all: return ""
        case .primary: return interfaces.filter{ $0.hasPrefix("en0") || $0.hasPrefix("utun")}.joined(separator: ",")
        case .vpnOnly: return interfaces.filter{ $0.hasPrefix("utun")}.joined(separator: ",")
        case .manual(name: let interface): return interface
        }
    }
    
    private var listenInterfaces: String {
        let interfaces = Utils.interfaceNames()
        
        switch interfaceType {
        case .all: return "0.0.0.0:\(portRangeFirst),[::]:\(portRangeFirst)"
        case .primary: return interfaces.filter{ $0.hasPrefix("en0") || $0.hasPrefix("utun")}.map{ "\($0):\(portRangeFirst)" }.joined(separator: ",")
        case .vpnOnly: return interfaces.filter{ $0.hasPrefix("utun")}.map{ "\($0):\(portRangeFirst)" }.joined(separator: ",")
        case .manual(name: let interface): return "\(interface):\(portRangeFirst)"
        }
    }
}

public extension SettingsPack {
    init(_ native: settings_pack_struct) {
        downloadLimit = Int(native.download_limit)
        uploadLimit = Int(native.upload_limit)
        encryptionPolicy = .init(rawValue: Int8(native.encryption_policy.rawValue))!
        enableDht = native.enable_dht
        enableLsd = native.enable_lsd
        enableUtp = native.enable_utp
        enableUpnp = native.enable_upnp
        enableNatpmp = native.enable_natpmp
        enablePex = native.enable_pex
        maxActiveTorrents = Int(native.max_active_torrents_limit)
        maxUplodingTorrents = Int(native.max_upload_torrents_limit)
        maxDownloadingTorrents = Int(native.max_download_torrents_limit)
        interfaceType = .all
//        outgoingInterfaces = String(validatingUTF8: native.outgoing_interfaces) ?? ""
//        listenInterfaces = String(validatingUTF8: native.listen_interfaces) ?? ""
        portRangeFirst = Int(native.port_range_first)
        portRangeSecond = Int(native.port_range_second)
        proxyType = ProxyType(rawValue: Int(native.proxy_type.rawValue))!
        proxyRequiresAuth = native.proxy_requires_auth
        proxyHostname = String(validatingUTF8: native.proxy_hostname) ?? ""
        proxyPort = Int(native.proxy_port)
        proxyUsername = String(validatingUTF8: native.proxy_username) ?? ""
        proxyPassword = String(validatingUTF8: native.proxy_password) ?? ""
        proxyPeerConnections = native.proxy_peer_connections
    }

    init(downloadLimit: Int,
         uploadLimit: Int,
         enableDht: Bool,
         enableLsd: Bool,
         enableUtp: Bool,
         enableUpnp: Bool,
         enableNatpmp: Bool,
         enablePex: Bool,
         maxActiveTorrents: Int,
         maxUplodingTorrents: Int,
         maxDownloadingTorrents: Int,
         interfaceType: InterfaceType,
         encryptionPolicy: EncryptionPolicy,
//         outgoingInterfaces: String,
//         listenInterfaces: String,
         portRangeFirst: Int,
         portRangeSecond: Int,
         proxyType: ProxyType,
         proxyRequiresAuth: Bool,
         proxyHostname: String,
         proxyPort: Int,
         proxyUsername: String,
         proxyPassword: String,
         proxyPeerConnection: Bool) {
        self.downloadLimit = downloadLimit
        self.uploadLimit = uploadLimit
        self.enableDht = enableDht
        self.enableLsd = enableLsd
        self.enableUtp = enableUtp
        self.enableUpnp = enableUpnp
        self.enableNatpmp = enableNatpmp
        self.enablePex = enablePex
        self.maxActiveTorrents = maxActiveTorrents
        self.maxUplodingTorrents = maxUplodingTorrents
        self.maxDownloadingTorrents = maxDownloadingTorrents
        self.interfaceType = interfaceType
        self.encryptionPolicy = encryptionPolicy
//        self.outgoingInterfaces = outgoingInterfaces
//        self.listenInterfaces = listenInterfaces
        self.portRangeFirst = portRangeFirst
        self.portRangeSecond = portRangeSecond
        self.proxyType = proxyType
        self.proxyRequiresAuth = proxyRequiresAuth
        self.proxyHostname = proxyHostname
        self.proxyPort = proxyPort
        self.proxyUsername = proxyUsername
        self.proxyPassword = proxyPassword
        proxyPeerConnections = proxyPeerConnection
    }
}
