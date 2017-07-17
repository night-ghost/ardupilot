/* 
   DataFlash logging - file oriented variant

   This uses SD library to create log files called logs/NN.bin in the
   given directory

   SD Card Rates 
    - deletion rate seems to be ~50 files/second.
    - stat seems to be ~150/second
    - readdir loop of 511 entry directory ~62,000 microseconds
 */


#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_REVOMINI && (defined(BOARD_SDCARD_NAME) || defined(BOARD_DATAFLASH_FATFS))

#include "DataFlash_File_sd.h"

#include <AP_Common/AP_Common.h>
#include <assert.h>
#include <AP_Math/AP_Math.h>
#include <stdio.h>
#include <time.h>
#include <time.h>
#include <GCS_MAVLink/GCS.h>
extern const AP_HAL::HAL& hal;

#define MAX_LOG_FILES 50U
#define DATAFLASH_PAGE_SIZE 1024UL

/*
  constructor
 */
DataFlash_File::DataFlash_File(DataFlash_Class &front,
                               DFMessageWriter_DFLogStart *writer,
                               const char *log_directory) :
    DataFlash_Backend(front, writer),
    _write_fd(File()),
    _read_fd(File()),
    _read_fd_log_num(0),
    _read_offset(0),
    _write_offset(0),
    _initialised(false),
    _open_error(false),
    _log_directory(log_directory),
    _cached_oldest_log(0),
    _writebuf(0),
    _writebuf_chunk(4096),
    _last_write_time(0)
{}


void DataFlash_File::Init()
{
    DataFlash_Backend::Init();

    semaphore = hal.util->new_semaphore();
    if (semaphore == nullptr) {
        AP_HAL::panic("Failed to create DataFlash_File semaphore");
        return;
    }

    if(HAL_REVOMINI::state.sd_busy) return; // SD mounted via USB

    // create the log directory if need be
    const char* custom_dir = hal.util->get_custom_log_directory();
    if (custom_dir != nullptr){
        _log_directory = custom_dir;
    }

    if (! SD.exists(_log_directory) ) {
        char buf[80];
        const char *cp, *rp;
        char *wp;
        for(cp=_log_directory + strlen(_log_directory);cp>_log_directory && *cp != '/';cp--){   }
        for(wp=buf, rp=_log_directory; rp<cp;){
            *wp++ = *rp++;
        }
        *wp++ = 0;
        SD.mkdir(buf);

        if (!SD.mkdir(_log_directory)) {
            
            hal.console->printf("Failed to create log directory %s\n", _log_directory);
            _log_directory="0:";
        }
    }

    // determine and limit file backend buffersize
    uint32_t bufsize = _front._params.file_bufsize;
    if (bufsize > 64) {
        bufsize = 64; // DMA limitations.
    }
    bufsize *= 1024;

    // If we can't allocate the full size, try to reduce it until we can allocate it
    while (!_writebuf.set_size(bufsize) && bufsize >= _writebuf_chunk) {
        hal.console->printf("DataFlash_File: Couldn't set buffer size to=%u\n", (unsigned)bufsize);
        bufsize >>= 1;
    }

    if (!_writebuf.get_size()) {
        hal.console->printf("Out of memory for logging\n");
        return;
    }

    hal.console->printf("DataFlash_File: buffer size=%u\n", (unsigned)bufsize);

    _initialised = true;
    hal.scheduler->register_io_process(FUNCTOR_BIND_MEMBER(&DataFlash_File::_io_timer, void));
}

bool DataFlash_File::file_exists(const char *filename) const
{
    return SD.exists(filename);
}

bool DataFlash_File::log_exists(const uint16_t lognum) const
{
    char *filename = _log_file_name(lognum);
    if (filename == nullptr) {
        // internal_error();
        return false; // ?!
    }
    bool ret = file_exists(filename);
    free(filename);
    return ret;
}

void DataFlash_File::periodic_1Hz(const uint32_t now)
{
    if (!io_thread_alive()) {
        gcs().send_text(MAV_SEVERITY_CRITICAL, "No IO Thread Heartbeat");
        // If you try to close the file here then it will almost
        // certainly block.  Since this is the main thread, this is
        // likely to cause a crash.
        _write_fd.close();
        _initialised = false;
    }
}

void DataFlash_File::periodic_fullrate(const uint32_t now)
{
    DataFlash_Backend::push_log_blocks();
}

uint32_t DataFlash_File::bufferspace_available()
{
    const uint32_t space = _writebuf.space();
    const uint32_t crit = critical_message_reserved_space();

    return (space > crit) ? space - crit : 0;
}

// return true for CardInserted() if we successfully initialized
bool DataFlash_File::CardInserted(void) const
{
    return _initialised && !_open_error && !HAL_REVOMINI::state.sd_busy;
}

// returns the available space in _log_directory as a percentage
// returns -1.0f on error
float DataFlash_File::avail_space_percent()
{
    
    uint32_t space;
    int32_t avail = SD.getfree(_log_directory, &space);
    if (avail == -1) {
        return -1.0f;
    }

    return (avail/(float)space) * 100;
}

// find_oldest_log - find oldest log in _log_directory
// returns 0 if no log was found
uint16_t DataFlash_File::find_oldest_log(uint16_t *log_count)
{
    if (_cached_oldest_log != 0) {
        return _cached_oldest_log;
    }

    uint16_t last_log_num = find_last_log();
    if (last_log_num == 0) {
        return 0;
    }

    uint16_t current_oldest_log = 0; // 0 is invalid

    // We could count up to find_last_log(), but if people start
    // relying on the min_avail_space_percent feature we could end up
    // doing a *lot* of asprintf()s and stat()s
    File dir = SD.open(_log_directory);
    if (!dir) {
        // internal_error();
        return 0;
    }

    // we only remove files which look like xxx.BIN
    while(1){
        File de=dir.openNextFile();
        if(!de) break;
        
        char *nm = de.name();
        de.close();
        
        uint8_t length = strlen(nm);
        if (length < 5) {
            // not long enough for \d+[.]BIN
            continue;
        }
        if (strncmp(&nm[length-4], ".BIN", 4)) {
            // doesn't end in .BIN
            continue;
        }

        if(log_count) (*log_count) += 1;

        uint16_t thisnum = strtoul(nm, nullptr, 10);
//        if (thisnum > MAX_LOG_FILES) {
//            // ignore files above our official maximum...
//            continue;
//        }
        if (current_oldest_log == 0) {
            current_oldest_log = thisnum;
        } else {
            if (current_oldest_log <= last_log_num) {
                if (thisnum > last_log_num) {
                    current_oldest_log = thisnum;
                } else if (thisnum < current_oldest_log) {
                    current_oldest_log = thisnum;
                }
            } else { // current_oldest_log > last_log_num
                if (thisnum > last_log_num) {
                    if (thisnum < current_oldest_log) {
                        current_oldest_log = thisnum;
                    }
                }
            }
        }
    }
    dir.close();
    _cached_oldest_log = current_oldest_log;

    return current_oldest_log;
}

void DataFlash_File::Prep_MinSpace()
{
    const uint16_t first_log_to_remove = find_oldest_log(NULL);
    if (first_log_to_remove == 0) {
        // no files to remove
        return;
    }

    _cached_oldest_log = 0;

    uint16_t log_to_remove = first_log_to_remove;

    uint16_t count = 0;
    do {
        float avail = avail_space_percent();
        if (avail < 0) {
            // internal_error()
            break;
        }
        if (avail >= min_avail_space_percent) {
            break;
        }
        if (count++ > MAX_LOG_FILES+10) {
            // *way* too many deletions going on here.  Possible internal error.
            // internal_error();
            break;
        }
        char *filename_to_remove = _log_file_name(log_to_remove);
        if (filename_to_remove == nullptr) {
            // internal_error();
            break;
        }
        if (SD.exists(filename_to_remove)) {
            hal.console->printf("Removing (%s) for minimum-space requirements (%.2f%% < %.0f%%)\n",
                                filename_to_remove, (double)avail, (double)min_avail_space_percent);
            if (!SD.remove(filename_to_remove)) {
                hal.console->printf("Failed to remove %s: %s\n", filename_to_remove, strerror(errno));
                free(filename_to_remove);
                if (errno == ENOENT) {
                    // corruption - should always have a continuous
                    // sequence of files...  however, there may be still
                    // files out there, so keep going.
                } else {
                    // internal_error();
                    break;
                }
            } else {
                free(filename_to_remove);
            }
        }
        log_to_remove++;
        if (log_to_remove > MAX_LOG_FILES) {
            log_to_remove = 1;
        }
    } while (log_to_remove != first_log_to_remove);
}


void DataFlash_File::Prep() {
    if (hal.util->get_soft_armed()) {
        // do not want to do any filesystem operations while we are e.g. flying
        return;
    }
    Prep_MinSpace();
}

bool DataFlash_File::NeedPrep()
{
    if (!CardInserted()) {
        // should not have been called?!
        return false;
    }

    if (avail_space_percent() < min_avail_space_percent) {
        return true;
    }

    return false;
}

/*
  construct a log file name given a log number. 
  Note: Caller must free.
 */
char *DataFlash_File::_log_file_name(const uint16_t log_num) const
{
    char *buf = (char *)malloc(256);
    if(!buf) return nullptr;
    
    sprintf(buf, "%s/%u.BIN", _log_directory, (unsigned)log_num);

    return buf;
}

/*
  return path name of the lastlog.txt marker file
  Note: Caller must free.
 */
char *DataFlash_File::_lastlog_file_name(void) const
{
    char *buf = (char *)malloc(256);
    if(!buf) return nullptr;

    sprintf(buf, "%s/LASTLOG.TXT", _log_directory);
    return buf;
}


// remove all log files
void DataFlash_File::EraseAll()
{
    uint16_t log_num;
    const bool was_logging = (_write_fd != -1);
    stop_logging();

    for (log_num=1; log_num<=MAX_LOG_FILES; log_num++) {
        char *fname = _log_file_name(log_num);
        if (fname == nullptr) {
            break;
        }
        SD.remove(fname);
        free(fname);
    }
    char *fname = _lastlog_file_name();
    if (fname != nullptr) {
        SD.remove(fname);
        free(fname);
    }

    _cached_oldest_log = 0;

    if (was_logging) {
        start_new_log();
    }
}

/* Write a block of data at current offset */
bool DataFlash_File::WritePrioritisedBlock(const void *pBuffer, uint16_t size, bool is_critical)
{
    if (_write_fd == -1 || !_initialised || _open_error) {
        return false;
    }

    if (! WriteBlockCheckStartupMessages()) {
        _dropped++;
        return false;
    }

    if (!semaphore->take(1)) {
        return false;
    }
        
    uint32_t space = _writebuf.space();

    if (_writing_startup_messages &&
        _startup_messagewriter->fmt_done()) {
        // the state machine has called us, and it has finished
        // writing format messages out.  It can always get back to us
        // with more messages later, so let's leave room for other
        // things:
        if (space < non_messagewriter_message_reserved_space()) {
            // this message isn't dropped, it will be sent again...
            semaphore->give();
            return false;
        }
    } else {
        // we reserve some amount of space for critical messages:
        if (!is_critical && space < critical_message_reserved_space()) {
            _dropped++;
            semaphore->give();
            return false;
        }
    }

    // if no room for entire message - drop it:
    if (space < size) {
//        hal.util->perf_count(_perf_overruns);
        hal.console->printf("dropping block!\n");

        _dropped++;
        semaphore->give();
        return false;
    }

    _writebuf.write((uint8_t*)pBuffer, size);
    semaphore->give();
    return true;
}

/*
  read a packet. The header bytes have already been read.
*/
bool DataFlash_File::ReadBlock(void *pkt, uint16_t size)
{
    if (!(_read_fd) || !_initialised || _open_error) {
        return false;
    }

    memset(pkt, 0, size);
    if (_read_fd.read(pkt, size) != size) {
        return false;
    }
    _read_offset += size;
    return true;
}


/*
  find the highest log number
 */
uint16_t DataFlash_File::find_last_log()
{
    unsigned ret = 0;
    char *fname = _lastlog_file_name();
    if (fname == nullptr) {
        return ret;
    }
    File fd = SD.open(fname, FILE_READ);
    free(fname);
    if (fd) {
        char buf[10];
        memset(buf, 0, sizeof(buf));
        if (fd.read(buf, sizeof(buf)-1) > 0) {
            sscanf(buf, "%u", &ret);            
        }
        fd.close();    
    }
    return ret;
}

uint32_t DataFlash_File::_get_log_size(const uint16_t log_num) const
{
    char *fname = _log_file_name(log_num);
    if (fname == nullptr) {
        return 0;
    }

    File fd = SD.open(fname, FILE_READ);
    free(fname);

    if(!fd) return 0;
    
    uint32_t sz= fd.size();
    fd.close();

    return sz;
}

uint32_t DataFlash_File::_get_log_time(const uint16_t log_num) const
{
    char *fname = _log_file_name(log_num);
    if (fname == nullptr) {
        return 0;
    }

    FILINFO fno;

    int8_t ret = SD.stat(fname, &fno);
    free(fname);

    if(ret<0) return 0;

    uint16_t date=fno.fdate,
             time=fno.ftime;
        
    
    struct tm t;
    
    t.tm_sec  = FAT_SECOND(time); //     seconds after the minute        0-61*
    t.tm_min  = FAT_MINUTE(time); //    minutes after the hour  0-59
    t.tm_hour = FAT_HOUR(time); //     hours since midnight    0-23
    t.tm_mday = FAT_DAY(date); //     day of the month        1-31
    t.tm_mon  = FAT_MONTH(date); //     months since January    0-11
    t.tm_year = FAT_YEAR(date); //     years since 1900        
//    t.tm_yday int     days since January 1    0-365
    t.tm_isdst =false;

    
    return to_timestamp(&t);
}

/*
  convert a list entry number back into a log number (which can then
  be converted into a filename).  A "list entry number" is a sequence
  where the oldest log has a number of 1, the second-from-oldest 2,
  and so on.  Thus the highest list entry number is equal to the
  number of logs.
*/
uint16_t DataFlash_File::_log_num_from_list_entry(const uint16_t list_entry)
{
    uint16_t oldest_log = find_oldest_log(NULL);
    if (oldest_log == 0) {
        // We don't have any logs...
        return 0;
    }

    uint32_t log_num = oldest_log + list_entry - 1;
    if (log_num > MAX_LOG_FILES) {
        log_num -= MAX_LOG_FILES;
    }
    return (uint16_t)log_num;
}

/*
  find the number of pages in a log
 */
void DataFlash_File::get_log_boundaries(const uint16_t list_entry, uint16_t & start_page, uint16_t & end_page)
{
    const uint16_t log_num = _log_num_from_list_entry(list_entry);
    if (log_num == 0) {
        // that failed - probably no logs
        start_page = 0;
        end_page = 0;
        return;
    }

    start_page = 0;
    end_page = _get_log_size(log_num) / DATAFLASH_PAGE_SIZE;
}

/*
  retrieve data from a log file
 */
int16_t DataFlash_File::get_log_data(const uint16_t list_entry, const uint16_t page, const uint32_t offset, const uint16_t len, uint8_t *data)
{
    if (!_initialised || _open_error) {
        return -1;
    }

    const uint16_t log_num = _log_num_from_list_entry(list_entry);
    if (log_num == 0) {
        // that failed - probably no logs
        return -1;
    }

    if (_read_fd && log_num != _read_fd_log_num) {
        _read_fd.close();
    }
    if (!(_read_fd)) {
        char *fname = _log_file_name(log_num);
        if (fname == nullptr) {
            return -1;
        }
        stop_logging();
        _read_fd = SD.open(fname, O_RDONLY);
        if (!(_read_fd)) {
            _open_error = true;
            int saved_errno = errno;
            ::printf("Log read open fail for %s - %s\n",
                     fname, strerror(saved_errno));
            hal.console->printf("Log read open fail for %s - %s\n",
                                fname, strerror(saved_errno));
            free(fname);
            return -1;            
        }
        free(fname);
        _read_offset = 0;
        _read_fd_log_num = log_num;
    }

    int16_t ret = (int16_t)_read_fd.read(data, len);
    if (ret > 0) {
        _read_offset += ret;
    }
    return ret;
}

/*
  find size and date of a log
 */
void DataFlash_File::get_log_info(const uint16_t list_entry, uint32_t &size, uint32_t &time_utc)
{
    uint16_t log_num = _log_num_from_list_entry(list_entry);
    if (log_num == 0) {
        // that failed - probably no logs
        size = 0;
        time_utc = 0;
        return;
    }

    size = _get_log_size(log_num);
    time_utc = _get_log_time(log_num);
}



/*
  get the number of logs - note that the log numbers must be consecutive
 */
uint16_t DataFlash_File::get_num_logs()
{
    uint16_t ret = 0;
    uint16_t high = find_last_log();
    uint16_t i;
    for (i=high; i>0; i--) {
        if (! log_exists(i)) {
            break;
        }
        ret++;
    }
    if (i == 0) {
        for (i=MAX_LOG_FILES; i>high; i--) {
            if (! log_exists(i)) {
                break;
            }
            ret++;
        }
    }
    return ret;
}

/*
  stop logging
 */
void DataFlash_File::stop_logging(void)
{
    if (_write_fd) {
        _write_fd.close();
        log_write_started = false;
    }
}


/*
  start writing to a new log file
 */
uint16_t DataFlash_File::start_new_log(void)
{
    stop_logging();

    start_new_log_reset_variables();

    if (_open_error) {
        // we have previously failed to open a file - don't try again
        // to prevent us trying to open files while in flight
        return 0xFFFF;
    }

    if (_read_fd) {
        _read_fd.close();
    }

    uint16_t log_num = find_last_log();
    // re-use empty logs if possible
    if (_get_log_size(log_num) > 0 || log_num == 0) {
        log_num++;
    }
    if (log_num > MAX_LOG_FILES) {
        log_num = 1;
    }
    char *fname = _log_file_name(log_num);
    if (fname == nullptr) {
        return 0xFFFF;
    }
    _write_fd = SD.open(fname, O_WRITE|O_CREAT|O_TRUNC);
    _cached_oldest_log = 0;

    if (!(_write_fd)) {
        _initialised = false;
        _open_error = true;
        int saved_errno = errno;
//        ::printf("Log open fail for %s - %s\n",     fname, strerror(saved_errno));
        hal.console->printf("Log open fail for %s - %s\n",
                            fname, strerror(saved_errno));
        free(fname);
        return 0xFFFF;
    }
    free(fname);
    _write_offset = 0;
    _writebuf.clear();
    log_write_started = true;

    // now update lastlog.txt with the new log number
    fname = _lastlog_file_name();

    File fd = SD.open(fname, O_WRITE|O_CREAT);
    free(fname);
    if (fd == -1) {
        return 0xFFFF;
    }

    char buf[30];
    snprintf(buf, sizeof(buf), "%u\r\n", (unsigned)log_num);
    const ssize_t to_write = strlen(buf);
    const ssize_t written = fd.write((uint8_t *)buf, to_write);
    fd.close();

    if (written < to_write) {
        return 0xFFFF;
    }

    return log_num;
}

/*
  Read the log and print it on port
*/
void DataFlash_File::LogReadProcess(const uint16_t list_entry,
                                    uint16_t start_page, uint16_t end_page, 
                                    print_mode_fn print_mode,
                                    AP_HAL::BetterStream *port)
{
    uint8_t log_step = 0;
    if (!_initialised || _open_error) {
        return;
    }

    const uint16_t log_num = _log_num_from_list_entry(list_entry);
    if (log_num == 0) {
        return;
    }

    if (_read_fd) {
        _read_fd.close();
    }
    char *fname = _log_file_name(log_num);
    if (fname == nullptr) {
        return;
    }
    _read_fd = SD.open(fname, O_RDONLY);
    free(fname);
    if (!(_read_fd)) {
        return;
    }
    _read_fd_log_num = log_num;
    _read_offset = 0;
    if (start_page != 0) {
        if (!_read_fd.seek(start_page * DATAFLASH_PAGE_SIZE)) {
            _read_fd.close();
            return;
        }
        _read_offset = start_page * DATAFLASH_PAGE_SIZE;
    }

    uint8_t log_counter = 0;

    while (true) {
        uint8_t data;
        if (_read_fd.read(&data, 1) != 1) {
            // reached end of file
            break;
        }
        _read_offset++;

        // This is a state machine to read the packets
        switch(log_step) {
            case 0:
                if (data == HEAD_BYTE1) {
                    log_step++;
                }
                break;

            case 1:
                if (data == HEAD_BYTE2) {
                    log_step++;
                } else {
                    log_step = 0;
                }
                break;

            case 2:
                log_step = 0;
                _print_log_entry(data, print_mode, port);
                log_counter++;
                break;
        }
        if (_read_offset >= (end_page+1) * DATAFLASH_PAGE_SIZE) {
            break;
        }
    }

    _read_fd.close();
}

/*
  this is a lot less verbose than the block interface. Dumping 2Gbyte
  of logs a page at a time isn't so useful. Just pull the SD card out
  and look at it on your PC
 */
void DataFlash_File::DumpPageInfo(AP_HAL::BetterStream *port)
{
    port->printf("DataFlash: num_logs=%u\n", 
                   (unsigned)get_num_logs());    
}

void DataFlash_File::ShowDeviceInfo(AP_HAL::BetterStream *port)
{
    port->printf("DataFlash logs stored in %s\n", 
                   _log_directory);
}


/*
  list available log numbers
 */
void DataFlash_File::ListAvailableLogs(AP_HAL::BetterStream *port)
{
    uint16_t num_logs = get_num_logs();

    if (num_logs == 0) {
        port->printf("\nNo logs\n\n");
        return;
    }
    port->printf("\n%u logs\n", (unsigned)num_logs);

    for (uint16_t i=1; i<=num_logs; i++) {
        uint16_t log_num = _log_num_from_list_entry(i);
        char *filename = _log_file_name(log_num);


        if (filename != nullptr) {
            if(file_exists(filename)) {
                    uint32_t time=_get_log_time(log_num);
                    struct tm *tm = gmtime((const time_t*)&time);
                    port->printf("Log %u in %s of size %u %u/%u/%u %u:%u\n",
                                   (unsigned)i,
                                   filename,
                                   (unsigned)_get_log_size(log_num),
                                   (unsigned)tm->tm_year+1900,
                                   (unsigned)tm->tm_mon+1,
                                   (unsigned)tm->tm_mday,
                                   (unsigned)tm->tm_hour,
                                   (unsigned)tm->tm_min);
                
            }
            free(filename);
        }
    }
    port->println();    
}


void DataFlash_File::_io_timer(void)
{
    uint32_t tnow = AP_HAL::millis();
    _io_timer_heartbeat = tnow;
    if (!(_write_fd) || !_initialised || _open_error) {
        return;
    }

    uint32_t nbytes = _writebuf.available();
    if (nbytes == 0) {
        return;
    }
    if (nbytes < _writebuf_chunk && 
        tnow - _last_write_time < 2000UL) {
        // write in _writebuf_chunk-sized chunks, but always write at
        // least once per 2 seconds if data is available
        return;
    }

    _last_write_time = tnow;
    if (nbytes > _writebuf_chunk) {
        // be kind to the FAT filesystem
        nbytes = _writebuf_chunk;
    }

    uint32_t size;
    const uint8_t *head = _writebuf.readptr(size);
    nbytes = MIN(nbytes, size);

    // try to align writes on a 512 byte boundary to avoid filesystem reads
    if ((nbytes + _write_offset) % 512 != 0) {
        uint32_t ofs = (nbytes + _write_offset) % 512;
        if (ofs < nbytes) {
            nbytes -= ofs;
        }
    }

    ssize_t nwritten = _write_fd.write(head, nbytes);
    if (nwritten <= 0) {
        _write_fd.close();
        _initialised = false;
    } else {
        _write_offset += nwritten;
        _writebuf.advance(nwritten);
        /*
          the best strategy for minimizing corruption on microSD cards
          seems to be to write in 4k chunks and fsync the file on each
          chunk, ensuring the directory entry is updated after each
          write.
         */
        _write_fd.sync();
    }
}

// this sensor is enabled if we should be logging at the moment
bool DataFlash_File::logging_enabled() const
{
    if (hal.util->get_soft_armed() ||
        _front.log_while_disarmed()) {
        return true;
    }
    return false;
}

bool DataFlash_File::io_thread_alive() const
{
    uint32_t tnow = AP_HAL::millis();
    // if the io thread hasn't had a heartbeat in a full second then it is dead
    return _io_timer_heartbeat + 1000 > tnow;
}

bool DataFlash_File::logging_failed() const
{
    bool op=false;
    
    if(_write_fd) op=true;

    if (!op &&
        (hal.util->get_soft_armed() ||
         _front.log_while_disarmed())) {
        return true;
    }
    if (_open_error) {
        return true;
    }
    if (!io_thread_alive()) {
        // No heartbeat in a second.  IO thread is dead?! Very Not
        // Good.
        return true;
    }

    return false;
}


void DataFlash_File::vehicle_was_disarmed()
{
    if (_front._params.file_disarm_rot) {
        // rotate our log.  Closing the current one and letting the
        // logging restart naturally based on log_disarmed should do
        // the trick:
        stop_logging();
    }
}


 
/////////////////////////////////////////////////////////////////////
//  функция конвертации между UNIX-временем и обычным представлением в виде даты и времени суток
#define _TBIAS_DAYS             ((70 * (uint32_t)365) + 17)
#define _TBIAS_SECS             (_TBIAS_DAYS * (xtime_t)86400)
#define _TBIAS_YEAR             1900
#define MONTAB(year)            ((((year) & 03) || ((year) == 0)) ? mos : lmos)
 
const uint16_t     lmos[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
const uint16_t     mos[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
 
#define Daysto32(year, mon)     (((year - 1) / 4) + MONTAB(year)[mon])
 
uint32_t DataFlash_File::to_timestamp(const struct tm *t)
{       /* convert time structure to scalar time */
    int32_t           days;
    uint32_t          secs;
    int32_t           mon, year;
 
    /* Calculate number of days. */
    mon = t->tm_mon - 1;
    year = t->tm_year - _TBIAS_YEAR;
    days  = Daysto32(year, mon) - 1;
    days += 365 * year;
    days += t->tm_mday;
    days -= _TBIAS_DAYS;
 
    /* Calculate number of seconds. */
    secs  = 3600 * t->tm_hour;
    secs += 60 * t->tm_min;
    secs += t->tm_sec;
 
    secs += (days * (uint32_t)86400);
 
    return (secs);
}
#endif
