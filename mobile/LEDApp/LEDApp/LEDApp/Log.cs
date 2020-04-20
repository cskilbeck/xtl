using System;

namespace LEDApp
{
    public static class Log
    {
        public enum Level : int
        {
            Verbose,
            Debug,
            Info,
            Warn,
            Error
        }

        public static string[] Levels = new string[]
        {
            "VERBOSE",
            "DEBUG  ",
            "INFO   ",
            "WARN   ",
            "ERROR  "
        };

        public static Level LogLevel = Level.Debug;

        public static void Write(Level level, string s)
        {
            if (level >= LogLevel)
            {
                Console.WriteLine($"{DateTime.Now.ToUniversalTime()}:{Levels[(int)level]}:{s}");
            }
        }

        public static void Verbose(string s)
        {
            Write(Level.Verbose, s);
        }

        public static void Debug(string s)
        {
            Write(Level.Debug, s);
        }

        public static void Info(string s)
        {
            Write(Level.Info, s);
        }

        public static void Warn(string s)
        {
            Write(Level.Warn, s);
        }

        public static void Error(string s)
        {
            Write(Level.Error, s);
        }
    }

}
