//
// Created by fred on 05/10/16.
//

#include "Log.h"

//Instantiate logger
Log Log::logger;

//Force compiler to generate code needed for << overload of different types
template Log &Log::operator<< <int8_t>(const int8_t &data);
template Log &Log::operator<< <int16_t>(const int16_t &data);
template Log &Log::operator<< <int32_t>(const int32_t &data);
template Log &Log::operator<< <int64_t>(const int64_t &data);
template Log &Log::operator<< <uint8_t>(const uint8_t &data);
template Log &Log::operator<< <uint16_t>(const uint16_t &data);
template Log &Log::operator<< <uint32_t>(const uint32_t &data);
template Log &Log::operator<< <uint64_t>(const uint64_t &data);
template Log &Log::operator<< <float>(const float &data);
template Log &Log::operator<< <double>(const double &data);

//String names for each logging level
std::string log_levels[Log::count] = {"Info", "Warn", "Crit"};

Log::Log()
: lock(ATOMIC_FLAG_INIT)
{

}

Log::~Log()
{

}


bool Log::init(const std::string &logpath)
{
    Log::logger.logstream.open(logpath);
    return Log::logger.logstream.is_open();
}

template<typename T>
Log &Log::operator<<(const T &data)
{
    logCommit(std::to_string(data));
    return Log::logger;
}

Log &Log::operator<<(const char *data)
{
    logCommit(data);
    return Log::logger;
}

Log &Log::operator<<(Log::Level loglevel)
{
    //Spinlock using atomic flag, the inline asm is used to pause slightly so we don't waste too many CPU cycles
    while(lock.test_and_set(std::memory_order_acquire))
            asm volatile("pause\n": : :"memory");

    logCommit("[" + getCurrentTimestamp() + " " + log_levels[loglevel] + "]: ");

    return Log::logger;
}

Log &Log::operator<<(Log::End end)
{
    //Release the lock
    logCommit("\n");
    lock.clear(std::memory_order_release);
    return Log::logger;
}

Log &Log::operator<<(const std::basic_string<char> &data)
{
    logCommit(data);
    return Log::logger;
}

void Log::logCommit(const std::string &data)
{
    logstream << data;
}

std::string Log::getCurrentTimestamp()
{
    //Get the current timestamp
    std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm now = *std::localtime(&time);

    //Convert the required bits of information into strings, padding with 0's where needed
    std::string day = now.tm_mday > 9 ?  std::to_string(now.tm_mday) : "0" + std::to_string(now.tm_mday);
    std::string month = now.tm_mon > 9 ?  std::to_string(now.tm_mon) : "0" + std::to_string(now.tm_mon);
    std::string year = now.tm_year > 9 ?  std::to_string(1900 + now.tm_year) : "0" + std::to_string(1900 + now.tm_year);

    std::string hour = now.tm_hour > 9 ?  std::to_string(now.tm_hour) : "0" + std::to_string(now.tm_hour);
    std::string min = now.tm_min > 9 ?  std::to_string(now.tm_min) : "0" + std::to_string(now.tm_min);
    std::string sec = now.tm_sec > 9 ?  std::to_string(now.tm_sec) : "0" + std::to_string(now.tm_sec);

    //Format data and return
    return year + "-" + month + "-" + day + " " + hour + ":" + min + ":" + sec;
}
