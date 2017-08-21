//
//  USBTransfer.swift
//  EasyCart
//
//  Created by Antonio Malara on 15/07/16.
//  Copyright © 2016 Antonio Malara. All rights reserved.
//

import Foundation

struct FTDIError : Error {
    let code : Int32
    let description : String
}

extension ftdi_context {
    func getErrorString() -> String {
        return String(cString: self.error_str)
    }
}

class FTDIContext {
    var context = ftdi_context()
    
    init (vendor: UInt16, product: UInt16) throws {
        ftdi_init(&context)
        try _ = wrapError(ftdi_usb_open(&context, Int32(vendor), Int32(product)))
        try _ = wrapError(ftdi_usb_reset(&context))
        try _ = wrapError(ftdi_usb_purge_buffers(&context))
    }
    
    deinit {
        ftdi_usb_close(&context)
        ftdi_deinit(&context)
    }
    
    func write(_ data: [UInt8]) throws {
        var temp = data
        try temp.withUnsafeMutableBufferPointer {
            _ = try wrapError(ftdi_write_data(&context, $0.baseAddress, Int32($0.count)))
        }
    }
    
    func read(_ count: Int, timeout: Int = 30) throws -> [UInt8] {
        var buffer = [UInt8].init(repeating: 0, count: count)
        var received = 0
        var retryTimes = timeout * 100
        
        repeat {
            var data = buffer[received..<buffer.count]
            let read = try data.withUnsafeMutableBufferPointer {
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
    
    func wrapError(_ ret: Int32) throws -> Int32 {
        guard ret >= 0 else {
            throw FTDIError(code: ret, description: context.getErrorString())
        }
        return ret
    }
}

enum UploadType : String {
    case PRG = "EFSTART:PRG\0"
    case CRT = "EFSTART:CRT\0"
}

enum UploadFileEvent {
    case message(String)
    case completion
}

typealias FileEventObserver = (UploadFileEvent) -> Void

func uploadFileInBackground(_ type: UploadType, data: [UInt8], observer: @escaping FileEventObserver) {
    Thread() {
        uploadFile(type, data: data) { event in
            DispatchQueue.main.async(execute: { observer(event) })
        }
    }.start()
}

func uploadFile(_ type: UploadType, data: [UInt8], observer: FileEventObserver) {
    do {
        observer(.message("Opening USB Interface"))
        
        let f = try FTDIContext(vendor: 0x0403, product: 0x8738)
        
        observer(.message("Waiting for the cartridge to respond"))
        
        var waiting = false
        repeat {
            try f.write(type.rawValue.toBytes())
            
            let x = try f.read(5)
            let s = String(bytes: x, encoding: String.Encoding.utf8)
            
            waiting = s == "WAIT\0"
        } while waiting == true
        
        observer(.message("Sending data"))
        
        var sent = 0
        repeat {
            let s = try f.read(2)
            let requestedSize = Int(s[0]) + Int(s[1]) << 8
            
            let lengthToSend = min(data.count, requestedSize)
            try f.write([UInt8(lengthToSend & 0xff), UInt8(lengthToSend >> 8)])
            
            try f.write(Array(data[sent..<sent+lengthToSend]))
            sent += lengthToSend
        } while sent < data.count
        
        observer(.message("Transfer completed"))
    }
    catch {
        observer(.message("Error during transfer: \(error)"))
    }
    
    observer(.completion)
}

class Thread : Foundation.Thread {
    let callback: () -> Void
    
    init(aCallback: @escaping () -> Void) {
        callback = aCallback
    }
    
    override func main() {
        callback()
    }
}
