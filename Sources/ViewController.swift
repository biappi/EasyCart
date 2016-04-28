//
//  ViewController.swift
//  EasyCart
//
//  Created by Antonio Malara on 28/04/16.
//  Copyright © 2016 Antonio Malara. All rights reserved.
//

import Cocoa

class Things : NSObject {
    var name : String
    var type : String
    
    init(name: String, type: String) {
        self.name = name
        self.type = type
    }
}

class ViewController: NSViewController {
    @IBOutlet var arrayController: NSArrayController!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        arrayController.content = [
            Things(name: "Test", type: "Test 1"),
            Things(name: "Test 2", type: "Test 3"),
        ]
    }

    override var representedObject: AnyObject? {
        didSet {
        // Update the view, if already loaded.
        }
    }
}

