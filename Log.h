//
// Created by fred on 05/10/16.
//

#ifndef FRLOG_H
#define FRLOG_H


#include <fstream>
#include <atomic>
#include <iostream>
#include <vector>
#include <algorithm>
#include <filesystem>

template<typename Test, template<typename...> class Ref>
struct is_specialization : std::false_type {};

template<template<typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref>: std::true_type {};

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

    //Disable copying/moving and whatnot
    void operator=(const Log &l)=delete;
    Log(const Log &l)=delete;
    Log(const Log &&l)=delete;

    /*!
     * Initialises the logging class
     *
     * @param log_directory The directory to create logs in. If it doesn't exist then it will be created.
     * @param replicate_to_stdout Should the log be repliacted to the standard output too?
     * @param max_log_size Maximum log size in bytes before rotating. Default 1MiB. 0 = unlimited.
     * @param log_retention Maximum log retention in iterations
     * @return True on successful initialisation, false on failure
     */
    bool init(std::string log_directory_, uint64_t max_log_size_ = 0x100000, uint64_t log_retention_ = 7, bool replicate_to_stdout = true)
    {
        log_directory = std::move(log_directory_);
        stdout_replication = replicate_to_stdout;
        max_log_size = max_log_size_;
        log_retention = log_retention_;
        if(log_directory.empty() || (log_directory.back() != '/' && log_directory.back() != '\\'))
        {
            log_directory += '/';
        }

        //Create logs directory
        if(!std::filesystem::exists(log_directory))
        {
            if(!std::filesystem::create_directory(log_directory))
            {
                std::cout << "Failed to create '" << log_directory << "' directory. Exiting." << std::endl;
                return false;
            }
        }

        //Open a new log file
        logpath = log_directory + suggest_log_filename();
        logstream.open(logpath);
        if(!logstream.is_open())
        {
            std::cout << "Failed to open " << logpath << " for logging!" << std::endl;
            return false;
        }
        *this << " -- Logging initialised -- " << Log::end;

        //Delete old logs
        purge_old_logs();
        return true;
    }

    //Required overloads for the '<<' operator
    template<typename T>
    inline Log &operator<<(const T &data)
    {
        if constexpr (is_specialization<T, std::atomic>())
        {
            *this << data.load();
        }
        else if constexpr (std::is_same<T, Level>())
        {
            //Spinlock using atomic flag, the inline asm is used to pause slightly so we don't waste too many CPU cycles
            while(lock.test_and_set(std::memory_order_acquire))
                    asm volatile("pause\n": : :"memory");

            static std::string log_levels[] = {"Info", "Warn", "Crit"}; //String log levels
            commit_log("[" + get_current_timestamp() + " " + log_levels[data] + "]: "); //data = log level enum
        }
        else if constexpr (std::is_same<T, End>())
        {
            //End log and release lock
            commit_log("\n", true);
            lock.clear(std::memory_order_release);
        }
        else
        {
            commit_log(data);
        }

        return *this;
    }

    /*!
     * Flush the log out stream
     */
    void flush()
    {
        logstream.flush();
    }

    /*!
     * Gets the logger instance
     *
     * @return The logger instance
     */
    static Log &get_instance()
    {
        static Log logger;
        return logger;
    }
private:

    //Constructor/destructor
    Log()
            : lock(ATOMIC_FLAG_INIT),
              max_log_size(0),
              log_retention(0),
              stdout_replication(true)
    {

    }

    /*!
     * Returns the current timestamp in a string format
     *
     * @return The timestamp in format YYYY-MM-DD HH:MM:SS
     */
    std::string get_current_timestamp()
    {
        //Get the current timestamp
        std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm now = *std::localtime(&time);
        now.tm_mon++; //Months start at 0 instead of 1 for some reason. But not days or years.

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
    std::string suggest_log_filename()
    {
        std::string timestamp = get_current_timestamp();
        std::replace(timestamp.begin(), timestamp.end(), ':', '-');
        return timestamp;
    }

    /*!
     * Deletes logs which are out of retention
     */
    void purge_old_logs()
    {
        if(log_retention != 0)
        {
            //Get a list of logs, and if there's more than retention allows, delete some
            std::vector<std::filesystem::directory_entry> log_files;
            auto iter = std::filesystem::directory_iterator(log_directory);
            for(auto &file : iter)
            {
                log_files.emplace_back(file);
            }

            if(log_files.size() < log_retention)
                return;

            //Sort files by name, oldest should be first
            std::sort(log_files.begin(), log_files.end(), [&](const auto &f1, const auto &f2){
                return f1.path() < f2.path();
            });

            //Erase oldest ones
            for(size_t a = 0; a < log_files.size() - log_retention; a++)
            {
                std::filesystem::remove(log_files[a]);
            }

        }
    }

    /*!
     * Internal logging function, it's what all of the '<<' overloads feed into.
     * It will automatically cycle the log if it gets too big.
     *
     * @param data The data to log
     * @param may_cycle True of the logs should be cycled if over limit
     */
    template<typename T>
    inline void commit_log(const T &data, bool may_cycle = false)
    {
        //Print it
        if(stdout_replication)
            std::cout << data;
        logstream << data;

        //Cycle logs if needed
        if(may_cycle && max_log_size != 0 && (size_t)logstream.tellp() > max_log_size)
            do_log_cycle();
    }

    /*!
     * Closes the current log,
     * opens a new one.
     */
    void do_log_cycle()
    {
        //Close current log file and open new one
        logstream.close();
        logpath = log_directory + suggest_log_filename();
        logstream.open(logpath);
        if(!logstream.is_open())
            throw std::runtime_error("Failed to open new log file: " + logpath);

        //Delete old logs if needbe
        purge_old_logs();
    }

    std::atomic_flag lock; //Atomic flag used for thread safe logging using a spinlock
    std::ofstream logstream; //Out output stream
    std::string log_directory; //Directory to create logs in, ends in a /
    std::string logpath; //The log filepath
    uint64_t max_log_size; //Max allowed log size in bytes. 0 means infinite.
    uint64_t log_retention; //Number of log iterations to keep
    bool stdout_replication; //Should logs also be sent to stdout?
};

//Set frlog define shortcut
#define frlog Log::get_instance()

#endif //FRLOG_H
