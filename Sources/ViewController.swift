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

func buildEntriesList(_ cart : Cart) -> [Entry] {
    var list : [Entry] = []
    
    for i in 0 ..< cart.entryCount() {
        let entry = cart.entry(at: i)
        
        list.append(Entry(
            idx:    Int(i + 1),
            name:   String(cString: cart.menuName(at: i)),
            type:   String(cString: cart.type(at: i)),
            bank:   Int(entry.bank),
            offset: Int(entry.offset),
            size:   Int(entry.size)
        ))
    }
    
    return list
}

func getViceAppUrl() -> URL? {
    guard let pathsCFArray = LSCopyApplicationURLsForBundleIdentifier("org.viceteam.x64" as CFString, nil) else {
        return nil
    }
    
    let pathsArray = pathsCFArray.takeUnretainedValue() as [AnyObject]
    return pathsArray.first as? URL
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
        entriesTable.register(forDraggedTypes: [RowPasteboardType])
        refreshUI()
    }
    
    func refreshUI() {
        arrayController.content   = buildEntriesList(document.cart)
        spaceIndicator.intValue   = Int32(EasyFlashSize) - document.cart.main_flash_space.total
        nameTextField.stringValue = String(cString: document.cart.efname)
        
        if nameTextField.stringValue == "(no EF-Name)" { nameTextField.stringValue = "" }
    }
    
    @IBAction func add(_ sender: AnyObject) {
        let panel = NSOpenPanel()
        
        panel.allowedFileTypes = [
            "prg", "PRG",
            "crt", "CRT",
        ]
        
        panel.begin { _ in
            guard let fileUrl = panel.url else {
                return
            }

            let addResult = self.document.cart.addFile(fileUrl.path)
            if  addResult < 0 {
                let alert = NSAlert()
                alert.messageText = "Error"
                alert.informativeText = "Error (\(addResult)) adding file \(fileUrl.path) to the cartridge"
                alert.runModal()
            }
            else {
                self.refreshUI()
                self.document.updateChangeCount(.changeDone)
            }
        }
    }
    
    @IBAction func remove(_ sender: AnyObject) {
        document.cart.removeEntry(at: arrayController.selectionIndex)
        refreshUI()
        self.document.updateChangeCount(.changeDone)
    }
   
    @IBAction func test(_ sender: AnyObject) {
        guard let viceUrl = getViceAppUrl() else {
            showAlert("Cannot find VICE", "Make sure you have the VICE C64 emulator")
            return
        }
        
        document.autosave(withImplicitCancellability: false) { (error) in
            if let error = error {
                showAlert("Error saving a preview file", "\(error)")
                return
            }
            
            guard let autosavedUrl = self.document.autosavedContentsFileURL else {
                showAlert("Error saving a preview file", "Couldn't get the autosaved file location")
                return
            }
            
            let vicePath = (viceUrl.path as NSString).appendingPathComponent("Contents/MacOS/x64")
            
            let p = Process()
            p.launchPath = vicePath
            p.arguments = [
                "-cartcrt",
                autosavedUrl.path
            ]
            p.launch()
        }
    }

    @IBAction func flash(_ sender: AnyObject) {
        document.autosave(withImplicitCancellability: false) { (error) in
            if let error = error {
                showAlert("Error saving a preview file", "\(error)")
                return
            }
            
            guard let autosavedUrl = self.document.autosavedContentsFileURL else {
                showAlert("Error saving a preview file", "Couldn't get the autosaved file location")
                return
            }
            
            self.startUpload(.CRT, data: (try! Data(contentsOf: autosavedUrl)).toBytes())
        }
    }
    
    @IBAction func uploadProgram(_ sender: AnyObject) {
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
    
    @IBAction func closeTransferringSheet(_ sender: AnyObject) {
        mainWindow.endSheet(transferringSheet)
    }
    
    func startUpload(_ type: UploadType, data: [UInt8]) {
        mainWindow.beginSheet(transferringSheet) { _ in }
        
        uploadFileInBackground(type, data: data) { (event) in
            switch event {
            case .message(let msg):
                self.transferringLabel.stringValue = msg
            case .completion:
                self.transferringButton.isEnabled = true
            }
        }
    }
    
    // - //
    
    func tableView(_ aTableView: NSTableView,
                   writeRowsWith rowIndexes: IndexSet,
                   to pboard: NSPasteboard) -> Bool
    {
        pboard.setData(NSKeyedArchiver.archivedData(withRootObject: rowIndexes),
                       forType: RowPasteboardType)
        return true
    }
    
    func tableView(_ tableView: NSTableView,
                   acceptDrop info: NSDraggingInfo,
                   row: Int,
                   dropOperation: NSTableViewDropOperation) -> Bool
    {
        let data : Data = info.draggingPasteboard().data(forType: RowPasteboardType)!
        let rowIndexes : IndexSet = NSKeyedUnarchiver.unarchiveObject(with: data) as! IndexSet
        
        let from  = rowIndexes.first
        var to    = row
        
        if to > from! { to -= 1 }
        
        document.cart.swapEntries(at: from!, with: to)
        
        refreshUI()
        arrayController.setSelectionIndex(Int(to))
        self.document.updateChangeCount(.changeDone)
        
        return true
    }
    
    func tableView(_ aTableView: NSTableView,
                   validateDrop info: NSDraggingInfo,
                   proposedRow row: Int,
                   proposedDropOperation operation: NSTableViewDropOperation) -> NSDragOperation
    {
        aTableView.setDropRow(row, dropOperation:.above)
        return .move
    }
    
    func selectedProgramEntry() -> efs_entry_t? {
        let indexes = entriesTable.selectedRowIndexes
        
        return indexes.count == 1
            ? document.cart.entry(at: indexes.first!)
            : nil
    }
    
    func canRunSelection() -> Bool {
        return selectedProgramEntry()?.type == EF_ENTRY_PRG
    }
    
    func tableViewSelectionDidChange(_ notification: Notification) {
        uploadButton.isEnabled = canRunSelection()
    }
}

func showAlert(_ message : String, _ info : String) {
    let alert = NSAlert()
    alert.messageText = message
    alert.informativeText = info
    alert.runModal()
}

extension Data {
    func toBytes() -> [UInt8] {
        return Array(UnsafeBufferPointer(start: (self as NSData).bytes.bindMemory(to: UInt8.self, capacity: self.count), count: self.count))
    }
}

extension String {
    func toBytes() -> [UInt8] {
        return self.data(using: String.Encoding.utf8)!.toBytes()
    }
}
