#ifndef GENERALDOMO_LOGGING_HPP_SEEN
#define GENERALDOMO_LOGGING_HPP_SEEN

namespace generaldomo {

    struct logbase_t {
        virtual ~logbase_t() {}
        virtual void debug(const std::string& msg) {}
        virtual void info(const std::string& msg) {}
        virtual void error(const std::string& msg) {}
    };
    struct console_log : public logbase_t  {
        enum class log_level : int {
            debug=1, info=2, error=3;
        };
        log_level level;
        console_log(log_level level=2) : level(level) {}
        virtual ~console_log();
        virtual void debug(const std::string& msg);
        virtual void info(const std::string& msg);
        virtual void error(const std::string& msg);
        virtual void always(const std::string& msg);
    };

}

#endif
