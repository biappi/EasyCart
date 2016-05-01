//
//  ViewController.swift
//  EasyCart
//
//  Created by Antonio Malara on 28/04/16.
//  Copyright © 2016 Antonio Malara. All rights reserved.
//

import Cocoa

let EasyFlashSize = 1024 * 1024 // EASYFLASH_SIZE

let RowPasteboardType = "name.malara.antonio.CartRowPboardType"

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

func buildEntriesList(cart : Cart) -> [Entry] {
    var list : [Entry] = []
    
    for i in 0 ..< cart.entryCount() {
        let entry = cart.entryAt(i)
        
        list.append(Entry(
            idx:    Int(i + 1),
            name:   String.fromCString(cart.menuNameAt(i)) ?? "",
            type:   String.fromCString(cart.typeAt(i)) ?? "",
            bank:   Int(entry.bank),
            offset: Int(entry.offset),
            size:   Int(entry.size)
        ))
    }
    
    return list
}

class ViewController : NSObject, NSTableViewDataSource {
    @IBOutlet      var arrayController: NSArrayController!
    @IBOutlet weak var spaceIndicator:  NSLevelIndicator!
    @IBOutlet weak var entriesTable:    NSTableView!
    @IBOutlet weak var document:        Document!
    @IBOutlet weak var nameTextField:   NSTextField!
    
    override func awakeFromNib() {
        spaceIndicator.maxValue = Double(EasyFlashSize)
        entriesTable.registerForDraggedTypes([RowPasteboardType])
        refreshUI()
    }
    
    func refreshUI() {
        arrayController.content   = buildEntriesList(document.cart)
        spaceIndicator.intValue   = Int32(EasyFlashSize) - document.cart.main_flash_space.total
        nameTextField.stringValue = String.fromCString(document.cart.efname) ?? ""
        
        if nameTextField.stringValue == "(no EF-Name)" { nameTextField.stringValue = "" }
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
            
            let addResult = self.document.cart.addFile(filePath)
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
    
    @IBAction func remove(sender: AnyObject) {
        document.cart.removeEntryAt(arrayController.selectionIndex)
        refreshUI()
    }
    
    // - //
    
    func tableView(aTableView: NSTableView,
                   writeRowsWithIndexes rowIndexes: NSIndexSet,
                   toPasteboard pboard: NSPasteboard) -> Bool
    {
        pboard.setData(NSKeyedArchiver.archivedDataWithRootObject(rowIndexes),
                       forType: RowPasteboardType)
        return true
    }
    
    func tableView(tableView: NSTableView,
                   acceptDrop info: NSDraggingInfo,
                   row: Int,
                   dropOperation: NSTableViewDropOperation) -> Bool
    {
        let data : NSData = info.draggingPasteboard().dataForType(RowPasteboardType)!
        let rowIndexes : NSIndexSet = NSKeyedUnarchiver.unarchiveObjectWithData(data) as! NSIndexSet
        
        let from  = rowIndexes.firstIndex
        var to    = row
        
        if to > from { to -= 1 }
        
        document.cart.swapEntriesAt(from, with: to)
        
        refreshUI()
        arrayController.setSelectionIndex(Int(to))
        
        return true
    }
    
    func tableView(aTableView: NSTableView,
                   validateDrop info: NSDraggingInfo,
                   proposedRow row: Int,
                   proposedDropOperation operation: NSTableViewDropOperation) -> NSDragOperation
    {
        aTableView.setDropRow(row, dropOperation:.Above)
        return .Move
    }
}
