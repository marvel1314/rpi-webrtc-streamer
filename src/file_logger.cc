/*
 *  Copyright (c) 2017, rpi-webrtc-streamer  Lyu,KeunChang
 *
 * utils.cc
 *
 * Modified version of webrtc/src/examples/peer/client/utils.cc in WebRTC source tree
 * The origianl copyright info below.
 */
/*
 *  Copyright 2011 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include <iostream>

#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/stringutils.h"
#include "rtc_base/stringencode.h"
#include "rtc_base/filerotatingstream.h"
#include "rtc_base/logsinks.h"

#include "file_logger.h"
#include "utils.h"
#include "compat/directory_iterator.h"

#define  LOGGING_FILENAME       "rws_log"
#define  MAX_LOG_FILE_SIZE      10*1024*1024    // 10M bytes;
#define  MAX_LOG_FILE_NUM       10

namespace utils {

//////////////////////////////////////////////////////////////////////////////////////////
//
// File Logger Class
//
//////////////////////////////////////////////////////////////////////////////////////////
FileLogger::FileLogger (const std::string path,
        const rtc::LoggingSeverity severity, bool disable_buffering) :
    inited_(false), dir_path_(path), severity_(severity),
    log_max_file_size_(MAX_LOG_FILE_SIZE),disable_buffering_(disable_buffering) {
}

FileLogger::~FileLogger () {
}

bool FileLogger::Init() {
    if( inited_ == true ) return true;  // no more initialization

    if( MoveLogFiletoNextShiftFolder() == false ) {
        RTC_LOG(LS_ERROR) << "Failed to rotate log files at path: " << dir_path_;
        return false;
    };

    logSink_.reset( new rtc::FileRotatingLogSink(dir_path_,
                LOGGING_FILENAME, log_max_file_size_, MAX_LOG_FILE_NUM ));

    if(!logSink_->Init()) {
        RTC_LOG(LS_ERROR) << "Failed to open log files at path: " << dir_path_;
        logSink_.reset();
        return false;
    }

    if( disable_buffering_ == true ) {
        // disabling_buffering in FileLogger
        logSink_->DisableBuffering();
    };

    rtc::LogMessage::LogThreads(true);
    rtc::LogMessage::LogTimestamps(true);
    rtc::LogMessage::AddLogToStream(logSink_.get(), severity_);
    inited_ = true;
    return true;
}

bool FileLogger::DeleteFolderFiles(const std::string &folder) {
    utils::DirectoryIterator it;

    // check src & dest path is directory
    if( !utils::IsFolder(folder)) {
        // folder does not exist, MoveFolderFiles will create the required folder
        return true;
    }

    // iterate source directory
    if (!it.Iterate(folder)) {
        return false;
    }
    do {
        std::string filename = it.Name();
        // ignore "." and ".." filename
        if( filename.compare(0, filename.size(), "." ) != 0 &&
                filename.compare(0, filename.size(), ".." ) != 0  ) {
            utils::DeleteFile(utils::GetFolder(folder) + filename);
            // don't care return value
        }
    } while ( it.Next() );

    return true;
}

bool FileLogger::MoveLogFiles(const std::string prefix,
            const std::string &src, const std::string &dest) {
    utils::DirectoryIterator it;
    std::string src_file, dest_file;

    // iterate source directory
    if (!it.Iterate(src)) {
        return false;
    }
    do {
        std::string filename = it.Name();
        if( filename.compare(0, prefix.size(), prefix ) == 0 ) {
            src_file =  utils::GetFolder(src) + filename;
            dest_file =  utils::GetFolder(dest) +  filename;
            if( utils::MoveFile(src_file, dest_file) == false )
                RTC_LOG(LS_ERROR) << "Failed to move file : "
                    << src_file << ", to: " << dest_file;
        };
    } while ( it.Next() );

    return true;
}

static int kDirectoryPrefixSize = 64;

bool FileLogger::MoveLogFiletoNextShiftFolder() {
    char src_dir_postfix[kDirectoryPrefixSize];
    char dest_dir_postfix[kDirectoryPrefixSize];

    std::string base_log_path;
    std::string src_log_dir, dest_log_dir;

    base_log_path = dir_path_;
    RTC_LOG(INFO) << __FUNCTION__ << "Base Log Path : " << base_log_path;
    // checking whether the base log directory is exist
    if( utils::IsFolder( base_log_path ) == false ) {
        RTC_LOG(LS_ERROR) << "Log directory does not exist : "
                    << base_log_path;
        return false;
    };

    // move log files to next shift directory
    for(int index = 9; index > 0 ; index -- ) {
        snprintf(dest_dir_postfix,sizeof(dest_dir_postfix),"0%d", index );
        snprintf(src_dir_postfix,sizeof(src_dir_postfix),"0%d", index - 1);
        dest_log_dir = base_log_path + dest_dir_postfix;
        src_log_dir = base_log_path + src_dir_postfix;

    RTC_LOG(INFO) << __FUNCTION__ << "Dest Log Path : " << dest_log_dir;
    RTC_LOG(INFO) << __FUNCTION__ << "Src Log Path : " << src_log_dir;

        // remove last rotate directory
        if( index == 9 ) {
            if( utils::IsFolder(dest_log_dir) == true ) {
                DeleteFolderFiles(dest_log_dir);
            }
        };
        if( MoveLogFiles(LOGGING_FILENAME, src_log_dir, dest_log_dir ) == false ){
            return false;
        }
    };

    // move log file to shit directory
    src_log_dir = dir_path_;
    dest_log_dir = dir_path_+"/00";
    if( MoveLogFiles(LOGGING_FILENAME, src_log_dir, dest_log_dir ) == false ){
        return false;
    }
    return true;
}

};


