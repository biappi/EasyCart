//
//  USBTransfer.swift
//  EasyCart
//
//  Created by Antonio Malara on 15/07/16.
//  Copyright © 2016 Antonio Malara. All rights reserved.
//

import Foundation

struct FTDIError : ErrorType {
    var code : Int32
    var description : String
}

extension ftdi_context {
    func getErrorString() -> String {
        return String.fromCString(self.error_str) ?? ""
    }
}

class FTDIContext {
    var context = ftdi_context()
    
    init (vendor: UInt16, product: UInt16) throws {
        ftdi_init(&context)
        try wrapError(ftdi_usb_open(&context, Int32(vendor), Int32(product)))
        try wrapError(ftdi_usb_reset(&context))
        try wrapError(ftdi_usb_purge_buffers(&context))
    }
    
    deinit {
        ftdi_usb_close(&context)
        ftdi_deinit(&context)
    }
    
    func write(data: [UInt8]) throws {
        var temp = data
        try temp.withUnsafeMutableBufferPointer {
            try wrapError(ftdi_write_data(&context, $0.baseAddress, Int32($0.count)))
        }
    }
    
    func read(count: Int, timeout: Int = 30) throws -> [UInt8] {
        var buffer = [UInt8].init(count: count, repeatedValue: 0)
        var received = 0
        var retryTimes = timeout * 100
        
        repeat {
            let read = try buffer[received..<buffer.count].withUnsafeMutableBufferPointer {
                return try wrapError(ftdi_read_data(&context, $0.baseAddress, Int32($0.count)))
            }
            
            received += Int(read)
            
            if read == 0 {
                usleep(10000)
                retryTimes -= 1
            }
            
            if retryTimes == 0 {
                throw FTDIError(code: -1, description: "read timeout")
            }
        } while received < count
        
        return buffer
    }
    
    func wrapError(ret: Int32) throws -> Int32 {
        if ret < 0 {
            throw FTDIError(code: ret, description: context.getErrorString())
        }
        else {
            return ret
        }
    }
}

enum UploadType : String {
    case PRG = "EFSTART:PRG\0"
    case CRT = "EFSTART:CRT\0"
}

enum UploadFileEvent {
    case Message(String)
    case Completion
}

typealias FileEventObserver = (UploadFileEvent) -> ()

func uploadFileInBackground(type: UploadType, data: [UInt8], observer: FileEventObserver) {
    Thread() {
        uploadFile(type, data: data) { event in
            dispatch_async(dispatch_get_main_queue(), { observer(event) })
        }
    }.start()
}

func uploadFile(type: UploadType, data: [UInt8], observer: FileEventObserver) {
    do {
        observer(.Message("Opening USB Interface"))
        
        let f = try FTDIContext(vendor: 0x0403, product: 0x8738)
        
        observer(.Message("Waiting for the cartridge to respond"))
        
        var waiting = false
        repeat {
            try f.write(type.rawValue.toBytes())
            
            let x = try f.read(5)
            let s = String(bytes: x, encoding: NSUTF8StringEncoding)
            
            waiting = s == "WAIT\0"
        } while waiting == true
        
        observer(.Message("Sending data"))
        
        var sent = 0
        repeat {
            let s = try f.read(2)
            let requestedSize = Int(s[0]) + Int(s[1]) << 8
            
            let lengthToSend = min(data.count, requestedSize)
            try f.write([UInt8(lengthToSend & 0xff), UInt8(lengthToSend >> 8)])
            
            try f.write(Array(data[sent..<sent+lengthToSend]))
            sent += lengthToSend
        } while sent < data.count
        
        observer(.Message("Transfer completed"))
    }
    catch let err as FTDIError {
        observer(.Message("Error during transfer: \(err)"))
    }
    catch {
        observer(.Message("Unknown error during transfer"))
    }
    
    observer(.Completion)
}

class Thread : NSThread {
    let callback: () -> ()
    
    init(aCallback: ()->()) {
        callback = aCallback
    }
    
    override func main() {
        callback()
    }
}
