//
//  ViewController.swift
//  EasyCart
//
//  Created by Antonio Malara on 28/04/16.
//  Copyright © 2016 Antonio Malara. All rights reserved.
//

import Cocoa

let EasyFlashSize = 1024 * 1024 // EASYFLASH_SIZE

class Entry : NSObject {
    var idx    : Int
    var name   : String 
    var type   : String
    var bank   : Int
    var offset : Int
    var size   : Int
    
    init(idx: Int, name: String, type: String, bank: Int, offset: Int, size: Int) {
        self.idx    = idx
        self.name   = name
        self.type   = type
        self.bank   = bank
        self.offset = offset
        self.size   = size
    }
}

func buildEntriesList() -> [Entry] {
    var list : [Entry] = []
    
    for i in 0 ..< main_flash_efs_num {
        let entry = main_flash_efs_entry(i)
        
        list.append(Entry(
            idx:    Int(i + 1),
            name:   String.fromCString(main_flash_efs_entry_menuname(i)) ?? "",
            type:   String.fromCString(efs_entry_type_string(entry.type)) ?? "",
            bank:   Int(entry.bank),
            offset: Int(entry.offset),
            size:   Int(entry.size)
        ))
    }
    
    return list
}

class ViewController: NSViewController {
    @IBOutlet      var arrayController: NSArrayController!
    @IBOutlet weak var spaceIndicator:  NSLevelIndicator!
    
    override func awakeFromNib() {
        self.spaceIndicator.maxValue = Double(EasyFlashSize)
        
        main_flash_init()
        refreshUI()
    }
    
    override var representedObject: AnyObject? {
        didSet {
        // Update the view, if already loaded.
        }
    }
    
    func refreshUI() {
        self.arrayController.content = buildEntriesList()
        self.spaceIndicator.intValue = Int32(EasyFlashSize) - main_flash_space.total
    }
    
    @IBAction func add(sender: AnyObject) {
        let panel = NSOpenPanel()
        
        panel.allowedFileTypes = [
            "prg", "PRG",
            "crt", "CRT",
        ]
        
        panel.beginWithCompletionHandler { _ in
            guard let fileUrl = panel.URL else {
                return
            }

            guard let filePath = fileUrl.path else {
                return
            }
            
            let addResult = main_flash_add_file(filePath, nil, 0, 0, 1)
            if  addResult < 0 {
                let alert = NSAlert()
                alert.messageText = "Error"
                alert.informativeText = "Error (\(addResult)) adding file \(filePath) to the cartridge"
                alert.runModal()
            }
            else {
                self.refreshUI()
            }
        }
    }
}

