# How to use:

To initialise Frlog, add 'Log::init(...)' to initialise logging in your main function like so:
```c++
#include <Log.h>

int main()
{
    //Initialise logging
    if(!frlog.init("logs/"))
        return EXIT_FAILURE;
        
    //...
    return 0;
}
```

This will create a directory called 'logs', and store log files within the directory. To log some data, use:
```c++
frlog << Log::info << "This is some data to be written " << 1 << 1.2 << std::string(" Hello") << Log::end;
```
Which would result in something like this being written to the log:
```c++
[2016-09-07 21:16:05 Info]: This is some data to be written 11.200000 Hello
```

'log' is a global instance of the logging class, which can be used just by including 'Log.h'. 'Log::info' is the type of information being outputted, you *must* include this information. The 'Log::end' lets the class know that you're ending the current line, and adds a newline. You *must* include a 'Log::end' at the end of each output, or you'll face issues.

These are the following log types available:
- info
- crit
- warn

# Requirements:
- A C++17 compliant compiler (constexpr if is used)
- The '-lstdc++fs' link flag added, as std::filesystem is used.

# Why must I include 'Log::type' at the beginning of my log, and 'Log::end' at the end?

Frlog is thread safe. It's guaranteed that Frlog will not suffer from race conditions, and it will also not mix concurrent attempts to log like you might see with std::cout. This is because a spinlock is started when a 'Log::type' is met, and then released when a 'Log::end' is met. If no 'Log::end' is met then the spinlock will not unlock and so the next attempt to start a new log will hang. Passing a 'Log::type' also lets the class know to output a timestamp at the beginning of the log.

I've chosen to use a spinlock over a mutex as there's no real performance impact if you are not logging concurrently (and so you don't have to mess around with compile flags to disable thread safety like some other logging classes). Spinlocks are also much faster than traditional mutexes as the kernal is not involved, making it ideal for waiting short durations (such as whilst a log finishes writing).

This does however mean that you might waste some CPU cycles if you're logging the output of functions directly which take a while to return, and so in these cases it might be ideal to work out the output first and then log it.
