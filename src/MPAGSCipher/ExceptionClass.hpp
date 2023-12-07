#ifndef MPAGSCIPHER_EXCEPTIONCLASS_HPP
#define MPAGSCIPHER_EXCEPTIONCLASS_HPP

#include <stdexcept>

class MissingArgument : public std::invalid_argument {
    public:
        MissingArgument( const std::string& msg ) : std::invalid_argument{msg}{

        }
}; 

class UnknownArgument : public std::invalid_argument {
    public:
        UnknownArgument(const std::string& msg) : std::invalid_argument{msg}{

        }
};

class InvalidInput : public std::invalid_argument {
    public:
        InvalidInput(const std::string& msg) : std::invalid_argument{msg}{

        }
};


#endif  //MPAGSCIPHER_EXCEPTIONCLASS_HPP