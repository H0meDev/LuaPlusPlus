#ifndef UNITTEST_ASSERTEXCEPTION_H
#define UNITTEST_ASSERTEXCEPTION_H

#include <stdexcept>


namespace UnitTest {

class AssertException : public std::runtime_error
{
public:
    AssertException(char const* description, char const* filename, int lineNumber);
    virtual ~AssertException() throw();

    virtual char const* what() const throw() override;

    char const* Filename() const;
    int LineNumber() const;

private:
    char m_description[512];
    char m_filename[256];
    int m_lineNumber;
};

}

#endif
