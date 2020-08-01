//
//  FolderModel.swift
//  iTorrent
//
//  Created by Daniil Vinogradov on 31.03.2020.
//  Copyright © 2020  XITRIX. All rights reserved.
//

import Foundation

public class FolderModel {
    public var name: String = ""
    public var path: URL!
    public var size: Int64 = 0
    public var downloadedSize: Int64 = 0
    public var isPreview: Bool = true
    public var files: [FileModel] = []
    
    public init() {}
}
