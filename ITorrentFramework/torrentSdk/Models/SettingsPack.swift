//
//  SettingsPack.swift
//  iTorrent
//
//  Created by Daniil Vinogradov on 19.06.2020.
//  Copyright © 2020  XITRIX. All rights reserved.
//

import Foundation
import ITorrentFramework.Private

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

    public var enableDht: Bool
    public var enableLsd: Bool
    public var enableUtp: Bool
    public var enableUpnp: Bool
    public var enableNatpmp: Bool

    public var outgoingInterfaces: String
    
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
                             enable_dht: enableDht,
                             enable_lsd: enableLsd,
                             enable_utp: enableUtp,
                             enable_upnp: enableUpnp,
                             enable_natpmp: enableNatpmp,
                             outgoing_interfaces: outgoingInterfaces.cString(),
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
}

public extension SettingsPack {
    init(_ native: settings_pack_struct) {
        downloadLimit = Int(native.download_limit)
        uploadLimit = Int(native.upload_limit)
        enableDht = native.enable_dht
        enableLsd = native.enable_lsd
        enableUtp = native.enable_utp
        enableUpnp = native.enable_upnp
        enableNatpmp = native.enable_natpmp
        outgoingInterfaces = String(validatingUTF8: native.outgoing_interfaces) ?? ""
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
         outgoingInterfaces: String,
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
        self.outgoingInterfaces = outgoingInterfaces
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
