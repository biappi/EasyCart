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

func getViceAppUrl() -> NSURL? {
    guard let pathsCFArray = LSCopyApplicationURLsForBundleIdentifier("org.viceteam.x64", nil) else {
        return nil
    }
    
    let pathsArray = pathsCFArray.takeUnretainedValue() as [AnyObject]
    return pathsArray.first as? NSURL
}

class ViewController : NSObject, NSTableViewDataSource, NSTableViewDelegate {
    @IBOutlet      var arrayController:    NSArrayController!
    @IBOutlet weak var spaceIndicator:     NSLevelIndicator!
    @IBOutlet weak var entriesTable:       NSTableView!
    @IBOutlet weak var document:           Document!
    @IBOutlet weak var nameTextField:      NSTextField!
    @IBOutlet weak var uploadButton:       NSButton!
    @IBOutlet weak var mainWindow:         NSWindow!
    @IBOutlet      var transferringSheet:  NSWindow!
    @IBOutlet weak var transferringLabel:  NSTextField!
    @IBOutlet weak var transferringButton: NSButton!
    
    
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
                self.document.updateChangeCount(.ChangeDone)
            }
        }
    }
    
    @IBAction func remove(sender: AnyObject) {
        document.cart.removeEntryAt(arrayController.selectionIndex)
        refreshUI()
        self.document.updateChangeCount(.ChangeDone)
    }
   
    @IBAction func test(sender: AnyObject) {
        guard let viceUrl = getViceAppUrl() else {
            showAlert("Cannot find VICE", "Make sure you have the VICE C64 emulator")
            return
        }
        
        document.autosaveWithImplicitCancellability(false) { (error) in
            if let error = error {
                showAlert("Error saving a preview file", error.description)
                return
            }
            
            guard let autosavedUrl = self.document.autosavedContentsFileURL else {
                showAlert("Error saving a preview file", "Couldn't get the autosaved file location")
                return
            }
            
            let vicePath = (viceUrl.path! as NSString).stringByAppendingPathComponent("Contents/MacOS/x64")
            let command = "\(vicePath) -cartcrt \"\(autosavedUrl.path!)\""
            
            command.withCString { system($0) }
        }
    }

    @IBAction func flash(sender: AnyObject) {
        document.autosaveWithImplicitCancellability(false) { (error) in
            if let error = error {
                showAlert("Error saving a preview file", error.description)
                return
            }
            
            guard let autosavedUrl = self.document.autosavedContentsFileURL else {
                showAlert("Error saving a preview file", "Couldn't get the autosaved file location")
                return
            }
            
            self.startUpload(.CRT, data: NSData(contentsOfURL: autosavedUrl)!.toBytes())
        }
    }
    
    @IBAction func uploadProgram(sender: AnyObject) {
        guard let entry = self.selectedProgramEntry() else {
            return
        }
        
        if entry.type != EF_ENTRY_PRG {
            return
        }
        
        let data = Array<UInt8>(UnsafeBufferPointer(start: UnsafePointer(entry.data),
                                                    count: Int(entry.size)))
        startUpload(.PRG, data: data)
    }
    
    @IBAction func closeTransferringSheet(sender: AnyObject) {
        mainWindow.endSheet(transferringSheet)
    }
    
    func startUpload(type: UploadType, data: [UInt8]) {
        mainWindow.beginSheet(transferringSheet) { _ in }
        
        uploadFileInBackground(type, data: data) { (event) in
            switch event {
            case .Message(let msg):
                self.transferringLabel.stringValue = msg
            case .Completion:
                self.transferringButton.enabled = true
            }
        }
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
        self.document.updateChangeCount(.ChangeDone)
        
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
    
    func selectedProgramEntry() -> efs_entry_t? {
        let indexes = entriesTable.selectedRowIndexes
        
        if indexes.count != 1 {
            return nil
        }
        else {
            return document.cart.entryAt(indexes.firstIndex)
        }
    }
    
    func canRunSelection() -> Bool {
        if let entry = selectedProgramEntry() {
            return entry.type == EF_ENTRY_PRG
        }
        else {
            return false;
        }
    }
    
    func tableViewSelectionDidChange(notification: NSNotification) {
        uploadButton.enabled = canRunSelection()
    }
}

func showAlert(message : String, _ info : String) {
    let alert = NSAlert()
    alert.messageText = message
    alert.informativeText = info
    alert.runModal()
}

extension NSData {
    func toBytes() -> [UInt8] {
        return Array(UnsafeBufferPointer(start: UnsafePointer<UInt8>(self.bytes), count: self.length))
    }
}

extension String {
    func toBytes() -> [UInt8] {
        return self.dataUsingEncoding(NSUTF8StringEncoding)!.toBytes()
    }
}
