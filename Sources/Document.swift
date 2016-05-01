//
//  Document.swift
//  EasyCart
//
//  Created by Antonio Malara on 28/04/16.
//  Copyright © 2016 Antonio Malara. All rights reserved.
//

import Cocoa

class Document: NSDocument {
    var cart : Cart = Cart()
    
    override init() {
        super.init()
    }
    
    override class func autosavesInPlace() -> Bool {
        return true
    }
    
    override var windowNibName: String? {
        // Returns the nib file name of the document
        // If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this property and override -makeWindowControllers instead.
        return "Document"
    }
    
    override func writeToURL(url: NSURL, ofType typeName: String) throws {
        if !url.fileURL {
            throw NSError(domain: "SaveError", code: 1, userInfo: nil)
        }
        
        guard let path = url.path else {
            throw NSError(domain: "SaveError", code: 2, userInfo: nil)
        }
        
        try path.withCString {
            let result = cart.save($0)
            if result < 0 {
                throw NSError(domain: "NdefpackSaveError", code: Int(result), userInfo: nil)
            }
        }
    }
    
    override func readFromURL(url: NSURL, ofType typeName: String) throws {
        if !url.fileURL {
            throw NSError(domain: "LoadError", code: 1, userInfo: nil)
        }
        
        guard let path = url.path else {
            throw NSError(domain: "LoadError", code: 2, userInfo: nil)
        }
        
        try path.withCString {
            let result = cart.load($0)
            if result < 0 {
                throw NSError(domain: "NdefpackLoadError", code: Int(result), userInfo: nil)
            }
        }
    }
}
