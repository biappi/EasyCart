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
    
    init () {
        ftdi_init(&context)
    }
    
    deinit {
        ftdi_usb_close(&context)
        ftdi_deinit(&context)
    }
    
    func open(vendor vendor: UInt16, product: UInt16) throws {
        try wrapError(ftdi_usb_open(&context, Int32(vendor), Int32(product)))
        try wrapError(ftdi_usb_reset(&context))
        try wrapError(ftdi_usb_purge_buffers(&context))
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
            } else  { print("read \(read)") }
            
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