//
// Created by fred on 05/10/16.
//

#ifndef FRLOG_H
#define FRLOG_H

#include <string>
#include <iostream>
#include <fstream>
#include <atomic>
#include <ctime>
#include <fstream>
#include <chrono>
#include <algorithm>

//Set frlog define shortcut
#define frlog Log::logger

//Define static members
#define frlog_define() Log Log::logger; std::string Log::log_levels[Log::count]{"Info", "Warn", "Crit"};

class Log
{
public:
    //Log importance levels
    enum Level
    {
        info = 0,
        warn = 1,
        crit = 2,
        count = 3,
    };
    enum End
    {
        end = 0,
    };

    /*!
     * Initialises the logging class
     *
     * @param logpath The filepath to use for logging
     * @param replicate_to_stdout Should the log be repliacted to the standard output too?
     * @return True on successful initialisation, false on failure
     */
    static bool init(const std::string &logpath, bool replicate_to_stdout = true)
    {
        Log::logger.logstream.open(logpath);
        Log::logger.stdout_replication = replicate_to_stdout;
        if(!Log::logger.logstream.is_open())
        {
            std::cout << "Failed to open " << logpath << " for logging!" << std::endl;
            return false;
        }
        else
        {
            frlog << " -- Logging initialised -- " << Log::end;
        }

        return true;
    }

    /*!
     * Returns the current timestamp in a string format
     *
     * @return The timestamp in format YYYY-MM-DD HH:MM:SS
     */
    static std::string get_current_timestamp()
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

    /*!
     * Generates a suggested log name, with characters
     * safe on both Linux and Windows.
     *
     * @return A suitable log name
     */
    static std::string suggest_log_filename()
    {
        std::string timestamp = get_current_timestamp();
        std::replace(timestamp.begin(), timestamp.end(), ':', '-');
        return timestamp;
    }

    //Required overloads for the '<<' operator
    template<typename T>
    inline Log &operator<<(const T &data)
    {
        commit_log(std::to_string(data));
        return Log::logger;
    }

    inline Log &operator<<(const std::basic_string<char> &data)
    {
        commit_log(data);
        return Log::logger;
    }

    Log &operator<<(const char *data)
    {
        commit_log(data);
        return Log::logger;
    }

    Log &operator<<(Level loglevel)
    {
        //Spinlock using atomic flag, the inline asm is used to pause slightly so we don't waste too many CPU cycles
        while(lock.test_and_set(std::memory_order_acquire))
                asm volatile("pause\n": : :"memory");

        commit_log("[" + get_current_timestamp() + " " + log_levels[loglevel] + "]: ");

        return Log::logger;
    }

    Log &operator<<(End end)
    {
        //Release the lock
        commit_log("\n");
        lock.clear(std::memory_order_release);
        return Log::logger;
    }

    /*!
     * Flush the log out stream
     */
    static void flush()
    {
        Log::logger.logstream.flush();
    }

    //The single instance of the class which will be used
    static Log logger;
private:
    //Constructor/destructor
    Log()
    : lock(ATOMIC_FLAG_INIT){}

    //Disable copying/moving and whatnot
    void operator=(const Log &l)=delete;
    Log(const Log &l)=delete;
    Log(const Log &&l)=delete;

    //Internal logging function, it's what all of the '<<' overloads feed into
    inline void commit_log(const std::string &data)
    {
        if(stdout_replication)
            std::cout << data;
        logstream << data;
    }

    static std::string log_levels[]; //String log levels
    std::atomic_flag lock; //Atomic flag used for thread safe logging using a spinlock
    std::ofstream logstream; //Out output stream
    bool stdout_replication; //Should logs also be sent to stdout?
};

#endif //FRLOG_H
