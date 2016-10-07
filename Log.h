//
// Created by fred on 05/10/16.
//

#ifndef BACKUPSERVER_LOG_H
#define BACKUPSERVER_LOG_H

#include <string>
#include <iostream>
#include <fstream>
#include <atomic>
#include <ctime>
#include <fstream>
#include <chrono>

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
     * @return True on successfull initialisation, false on failure
     */
    static bool init(const std::string &logpath);

    static void setOutputStream(std::ostream &stream);

    //Required overloads for the '<<' operator
    template<typename T>
    Log &operator<<(const T &data);
    Log &operator<<(const std::basic_string<char> &data);
    Log &operator<<(const char *data);
    Log &operator<<(Level loglevel);
    Log &operator<<(End end);

    //The single instance of the class which will be used
    static Log logger;
private:
    //Constructor/destructor
    Log();
    ~Log();

    /*!
     * Returns the current timestamp in a string format
     *
     * @return The timestamp in format YYYY-MM-DD HH:MM:SS
     */
    std::string getCurrentTimestamp();

    //Disable copying/moving and whatnot
    void operator=(const Log &l)=delete;
    Log(const Log &l)=delete;
    Log(const Log &&l)=delete;

    //Internal logging function, it's what all of the '<<' overloads feed into
    void logCommit(const std::string &data);

    std::atomic_flag lock; //Atomic flag used for thread safe logging using a spinlock
    std::ofstream logstream; //Out output stream
};

#define log Log::logger

#endif //BACKUPSERVER_LOG_H
