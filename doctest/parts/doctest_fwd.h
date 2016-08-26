//
// doctest.h - the lightest feature-rich C++ single-header testing framework for unit tests and TDD
//
// Copyright (c) 2016 Viktor Kirilov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
// The documentation can be found at the library's page:
// https://github.com/onqtam/doctest/blob/master/doc/markdown/readme.md
//
// =================================================================================================
// =================================================================================================
// =================================================================================================
//
// The library is heavily influenced by Catch - https://github.com/philsquared/Catch
// which uses the Boost Software License - Version 1.0
// see here - https://github.com/philsquared/Catch/blob/master/LICENSE_1_0.txt
//
// The concept of subcases (sections in Catch) and expression decomposition are from there.
// Some parts of the code are taken directly:
// - stringification - the detection of "ostream& operator<<(ostream&, const T&)" and StringMaker<>
// - the Approx() helper class for floating point comparison
// - colors in the console
// - breaking into a debugger
//
// The expression decomposing templates are taken from lest - https://github.com/martinmoene/lest
// which uses the Boost Software License - Version 1.0
// see here - https://github.com/martinmoene/lest/blob/master/LICENSE_1_0.txt
//
// =================================================================================================
// =================================================================================================
// =================================================================================================

// Suppress this globally - there is no way to silence it in the expression decomposition macros
// _Pragma() in macros doesn't work for the c++ front-end of g++
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55578
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69543
// Also the warning is completely worthless nowadays - http://stackoverflow.com/questions/14016993
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Waggregate-return"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif // __clang__

#if defined(__GNUC__) && !defined(__clang__)
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 6)
#pragma GCC diagnostic push
#endif // > gcc 4.6
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Winline"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wunsafe-loop-optimizations"
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 6)
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif // > gcc 4.6
#endif // __GNUC__

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // The compiler encountered a deprecated declaration
#pragma warning(disable : 4267) // 'var' : conversion from 'size_t' to 'type', possible loss of data
#pragma warning(disable : 4706) // assignment within conditional expression
#pragma warning(disable : 4512) // 'class' : assignment operator could not be generated
#pragma warning(disable : 4127) // conditional expression is constant
#endif                          // _MSC_VER

#ifndef DOCTEST_LIBRARY_INCLUDED
#define DOCTEST_LIBRARY_INCLUDED

#define DOCTEST_VERSION_MAJOR 1
#define DOCTEST_VERSION_MINOR 0
#define DOCTEST_VERSION_PATCH 0
#define DOCTEST_VERSION_STR "1.0.0"

#define DOCTEST_VERSION                                                                            \
    (DOCTEST_VERSION_MAJOR * 10000 + DOCTEST_VERSION_MINOR * 100 + DOCTEST_VERSION_PATCH)

// internal macros for string concatenation and anonymous variable name generation
#define DOCTEST_CONCAT_IMPL(s1, s2) s1##s2
#define DOCTEST_CONCAT(s1, s2) DOCTEST_CONCAT_IMPL(s1, s2)
#ifdef __COUNTER__ // not standard and may be missing for some compilers
#define DOCTEST_ANONYMOUS(x) DOCTEST_CONCAT(x, __COUNTER__)
#else // __COUNTER__
#define DOCTEST_ANONYMOUS(x) DOCTEST_CONCAT(x, __LINE__)
#endif // __COUNTER__

// not using __APPLE__ because... this is how Catch does it
#if defined(__MAC_OS_X_VERSION_MIN_REQUIRED)
#define DOCTEST_PLATFORM_MAC
#elif defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
#define DOCTEST_PLATFORM_IPHONE
#elif defined(_WIN32) || defined(_MSC_VER)
#define DOCTEST_PLATFORM_WINDOWS
#else
#define DOCTEST_PLATFORM_LINUX
#endif

#define DOCTEST_GCS() (*doctest::detail::getTestsContextState())

// should probably take a look at https://github.com/scottt/debugbreak
#ifdef DOCTEST_PLATFORM_MAC
// The following code snippet based on:
// http://cocoawithlove.com/2008/03/break-into-debugger.html
#if defined(__ppc64__) || defined(__ppc__)
#define DOCTEST_BREAK_INTO_DEBUGGER()                                                              \
    __asm__("li r0, 20\nsc\nnop\nli r0, 37\nli r4, 2\nsc\nnop\n" : : : "memory", "r0", "r3", "r4")
#else // __ppc64__ || __ppc__
#define DOCTEST_BREAK_INTO_DEBUGGER() __asm__("int $3\n" : :)
#endif // __ppc64__ || __ppc__
#elif defined(_MSC_VER)
#define DOCTEST_BREAK_INTO_DEBUGGER() __debugbreak()
#elif defined(__MINGW32__)
extern "C" __declspec(dllimport) void __stdcall DebugBreak();
#define DOCTEST_BREAK_INTO_DEBUGGER() ::DebugBreak()
#else // linux
#define DOCTEST_BREAK_INTO_DEBUGGER() ((void)0)
#endif // linux

#define DOCTEST_BREAK_INTO_DEBUGGER_CHECKED()                                                      \
    if(doctest::detail::isDebuggerActive() && !DOCTEST_GCS().no_breaks)                            \
        DOCTEST_BREAK_INTO_DEBUGGER();

#ifdef __clang__
// to detect if libc++ is being used with clang (the _LIBCPP_VERSION identifier)
#include <ciso646>
#endif // __clang__

#ifdef _LIBCPP_VERSION
// for libc++ I decided not to forward declare ostream myself because I had some problems
// so the <iosfwd> header is used - also it is very light and doesn't drag a ton of stuff
#include <iosfwd>
#else // _LIBCPP_VERSION
#ifndef DOCTEST_CONFIG_USE_IOSFWD
namespace std
{
template <class charT>
struct char_traits;
template <>
struct char_traits<char>;
template <class charT, class traits>
class basic_ostream;
typedef basic_ostream<char, char_traits<char> > ostream;
}
#else // DOCTEST_CONFIG_USE_IOSFWD
#include <iosfwd>
#endif // DOCTEST_CONFIG_USE_IOSFWD
#endif // _LIBCPP_VERSION

#ifndef DOCTEST_CONFIG_WITH_LONG_LONG
#if __cplusplus >= 201103L || (defined(_MSC_VER) && (_MSC_VER >= 1400)) // 1400 is MSVC 2005
#define DOCTEST_CONFIG_WITH_LONG_LONG
#endif // __cplusplus / _MSC_VER
#endif // DOCTEST_CONFIG_WITH_LONG_LONG

#if __cplusplus >= 201103L
#define DOCTEST_CONFIG_WITH_NULLPTR
#endif // __cplusplus >= 201103L

#ifdef __clang__
#if __has_feature(cxx_nullptr)
#define DOCTEST_CONFIG_WITH_NULLPTR
#endif // __has_feature(cxx_nullptr)
#endif // __clang__

#if defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ >= 6 && defined(__GXX_EXPERIMENTAL_CXX0X__)
#define DOCTEST_CONFIG_WITH_NULLPTR
#endif // __GNUC__

#if defined(_MSC_VER) && (_MSC_VER >= 1600) // MSVC 2010
#define DOCTEST_CONFIG_WITH_NULLPTR
#endif // _MSC_VER

#if defined(DOCTEST_CONFIG_NO_NULLPTR) && defined(DOCTEST_CONFIG_WITH_NULLPTR)
#undef DOCTEST_CONFIG_WITH_NULLPTR
#endif // DOCTEST_CONFIG_NO_NULLPTR

#ifdef DOCTEST_CONFIG_WITH_NULLPTR
#ifdef __clang__
#include <cstddef>
#else  // __clang__
namespace std
{ typedef decltype(nullptr) nullptr_t; }
#endif // __clang__
#endif // DOCTEST_CONFIG_WITH_NULLPTR

namespace doctest
{
class String
{
    char* m_str;

    void copy(const String& other);

public:
    String(const char* in = "");
    String(const String& other);
    ~String();

    String& operator=(const String& other);

    String operator+(const String& other) const;
    String& operator+=(const String& other);

    char& operator[](unsigned pos) { return m_str[pos]; }
    const char& operator[](unsigned pos) const { return m_str[pos]; }

    char*       c_str() { return m_str; }
    const char* c_str() const { return m_str; }

    unsigned size() const;
    unsigned length() const;

    int compare(const char* other, bool no_case = false) const;
    int compare(const String& other, bool no_case = false) const;
};

// clang-format off
inline bool operator==(const String& lhs, const String& rhs) { return lhs.compare(rhs) == 0; }
inline bool operator!=(const String& lhs, const String& rhs) { return lhs.compare(rhs) != 0; }
inline bool operator< (const String& lhs, const String& rhs) { return lhs.compare(rhs) < 0; }
inline bool operator> (const String& lhs, const String& rhs) { return lhs.compare(rhs) > 0; }
inline bool operator<=(const String& lhs, const String& rhs) { return (lhs != rhs) ? lhs.compare(rhs) < 0 : true; }
inline bool operator>=(const String& lhs, const String& rhs) { return (lhs != rhs) ? lhs.compare(rhs) > 0 : true; }
// clang-format on

std::ostream& operator<<(std::ostream& stream, const String& in);

namespace detail
{
    template <bool>
    struct STATIC_ASSERT_No_output_stream_operator_found_for_type;

    template <>
    struct STATIC_ASSERT_No_output_stream_operator_found_for_type<true>
    {};

    namespace has_insertion_operator_impl
    {
        typedef char no;
        typedef char yes[2];

        template <bool in>
        void           f() {
            STATIC_ASSERT_No_output_stream_operator_found_for_type<in>();
        }

        struct any_t
        {
            template <typename T>
            any_t(const T&) {
                f<false>();
            }
        };

        yes& testStreamable(std::ostream&);
        no   testStreamable(no);

        no operator<<(const std::ostream&, const any_t&);

        template <typename T>
        struct has_insertion_operator
        {
            static std::ostream& s;
            static const T&      t;
            static const bool    value = sizeof(testStreamable(s << t)) == sizeof(yes);
        };
    } // namespace has_insertion_operator_impl

    template <typename T>
    struct has_insertion_operator : has_insertion_operator_impl::has_insertion_operator<T>
    {};

    std::ostream* createStream();
    String        getStreamResult(std::ostream*);
    void          freeStream(std::ostream*);

    template <bool C>
    struct StringMakerBase
    {
        template <typename T>
        static String convert(const T&) {
            return "{?}";
        }
    };

    template <>
    struct StringMakerBase<true>
    {
        template <typename T>
        static String convert(const T& in) {
            std::ostream* stream = createStream();
            *stream << in;
            String result = getStreamResult(stream);
            freeStream(stream);
            return result;
        }
    };

    String rawMemoryToString(const void* object, unsigned size);

    template <typename T>
    String rawMemoryToString(const T& object) {
        return rawMemoryToString(&object, sizeof(object));
    }
} // namespace detail

template <typename T>
struct StringMaker : detail::StringMakerBase<detail::has_insertion_operator<T>::value>
{};

template <typename T>
struct StringMaker<T*>
{
    template <typename U>
    static String convert(U* p) {
        if(!p)
            return "NULL";
        else
            return detail::rawMemoryToString(p);
    }
};

template <typename R, typename C>
struct StringMaker<R C::*>
{
    static String convert(R C::*p) {
        if(!p)
            return "NULL";
        else
            return detail::rawMemoryToString(p);
    }
};

template <typename T>
String toString(const T& value) {
    return StringMaker<T>::convert(value);
}

String toString(const char* in);
String toString(bool in);
String toString(float in);
String toString(double in);
String toString(double long in);

String toString(char in);
String toString(char unsigned in);
String toString(int short in);
String toString(int short unsigned in);
String toString(int in);
String toString(int unsigned in);
String toString(int long in);
String toString(int long unsigned in);

#ifdef DOCTEST_CONFIG_WITH_LONG_LONG
String toString(int long long in);
String toString(int long long unsigned in);
#endif // DOCTEST_CONFIG_WITH_LONG_LONG

#ifdef DOCTEST_CONFIG_WITH_NULLPTR
String toString(std::nullptr_t in);
#endif // DOCTEST_CONFIG_WITH_NULLPTR

class Approx
{
public:
    explicit Approx(double value);

    Approx(Approx const& other)
            : m_epsilon(other.m_epsilon)
            , m_scale(other.m_scale)
            , m_value(other.m_value) {}

    Approx operator()(double value) {
        Approx approx(value);
        approx.epsilon(m_epsilon);
        approx.scale(m_scale);
        return approx;
    }

    friend bool operator==(double lhs, Approx const& rhs);
    friend bool operator==(Approx const& lhs, double rhs) { return operator==(rhs, lhs); }
    friend bool operator!=(double lhs, Approx const& rhs) { return !operator==(lhs, rhs); }
    friend bool operator!=(Approx const& lhs, double rhs) { return !operator==(rhs, lhs); }

    Approx& epsilon(double newEpsilon) {
        m_epsilon = newEpsilon;
        return *this;
    }

    Approx& scale(double newScale) {
        m_scale = newScale;
        return *this;
    }

    String toString() const;

private:
    double m_epsilon;
    double m_scale;
    double m_value;
};

template <>
inline String toString<Approx>(Approx const& value) {
    return value.toString();
}

#if !defined(DOCTEST_CONFIG_DISABLE)

namespace detail
{
    // the function type this library works with
    typedef void (*funcType)(void);

    // clang-format off
    template<class T>               struct decay_array       { typedef T type; };
    template<class T, unsigned N>   struct decay_array<T[N]> { typedef T* type; };
    template<class T>               struct decay_array<T[]>  { typedef T* type; };

    template<class T>   struct not_char_pointer              { enum { value = true }; };
    template<>          struct not_char_pointer<char*>       { enum { value = false }; };
    template<>          struct not_char_pointer<const char*> { enum { value = false }; };

    template<class T> struct can_use_op : not_char_pointer<typename decay_array<T>::type> {};

    template<bool, class = void> struct enable_if {};
    template<class T> struct enable_if<true, T> { typedef T type; };
    // clang-format on

    struct TestFailureException
    {};

    bool checkIfShouldThrow(const char* assert_name);
    void throwException();
    bool always_false();

    // a struct defining a registered test callback
    struct TestData
    {
        // not used for determining uniqueness
        const char* m_suite; // the test suite in which the test was added
        const char* m_name;  // name of the test function
        funcType    m_f;     // a function pointer to the test function

        // fields by which uniqueness of test cases shall be determined
        const char* m_file; // the file in which the test was registered
        unsigned    m_line; // the line where the test was registered

        TestData(const char* suite, const char* name, funcType f, const char* file, int line)
                : m_suite(suite)
                , m_name(name)
                , m_f(f)
                , m_file(file)
                , m_line(line) {}

        bool operator<(const TestData& other) const;
    };

    struct SubcaseSignature
    {
        const char* m_name;
        const char* m_file;
        int         m_line;

        SubcaseSignature(const char* name, const char* file, int line)
                : m_name(name)
                , m_file(file)
                , m_line(line) {}

        bool operator<(const SubcaseSignature& other) const;
    };

    struct Subcase
    {
        SubcaseSignature m_signature;
        bool             m_entered;

        Subcase(const char* name, const char* file, int line);
        ~Subcase();

        operator bool() const { return m_entered; }
    };

    template <typename L, typename R>
    String stringifyBinaryExpr(const L& lhs, const char* op, const R& rhs) {
        return toString(lhs) + " " + op + " " + toString(rhs);
    }

    // TODO: think about this
    //struct STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison;

    struct Result
    {
        bool   m_passed;
        String m_decomposition;

// to fix gcc 4.7 "-Winline" warnings
#if defined(__GNUC__) && !defined(__clang__)
        __attribute__((noinline))
#endif
        ~Result() {
        }

        Result(bool passed = false, const String& decomposition = String())
                : m_passed(passed)
                , m_decomposition(decomposition) {}

// to fix gcc 4.7 "-Winline" warnings
#if defined(__GNUC__) && !defined(__clang__)
        __attribute__((noinline))
#endif
        Result&
        operator=(const Result& other) {
            m_passed        = other.m_passed;
            m_decomposition = other.m_decomposition;

            return *this;
        }

        operator bool() { return !m_passed; }

        void invert() { m_passed = !m_passed; }

        // clang-format off
        //template <typename R> STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison& operator+(const R&);
        //template <typename R> STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison& operator-(const R&);
        //template <typename R> STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison& operator/(const R&);
        //template <typename R> STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison& operator*(const R&);
        //template <typename R> STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison& operator&&(const R&);
        //template <typename R> STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison& operator||(const R&);
        //
        //template <typename R> STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison& operator==(const R&);
        //template <typename R> STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison& operator!=(const R&);
        //template <typename R> STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison& operator<(const R&);
        //template <typename R> STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison& operator<=(const R&);
        //template <typename R> STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison& operator>(const R&);
        //template <typename R> STATIC_ASSERT_Expression_Too_Complex_Please_Rewrite_As_Binary_Comparison& operator>=(const R&);
        // clang-format on
    };
    // clang-format off
    template <typename L, typename R>
    typename enable_if<can_use_op<L>::value || can_use_op<R>::value, bool>::type eq(const L& lhs, const R& rhs) { return lhs == rhs; }
    template <typename L, typename R>
    typename enable_if<can_use_op<L>::value || can_use_op<R>::value, bool>::type ne(const L& lhs, const R& rhs) { return lhs != rhs; }
    template <typename L, typename R>
    typename enable_if<can_use_op<L>::value || can_use_op<R>::value, bool>::type lt(const L& lhs, const R& rhs) { return lhs < rhs; }
    template <typename L, typename R>
    typename enable_if<can_use_op<L>::value || can_use_op<R>::value, bool>::type gt(const L& lhs, const R& rhs) { return lhs > rhs; }
    template <typename L, typename R>
    typename enable_if<can_use_op<L>::value || can_use_op<R>::value, bool>::type le(const L& lhs, const R& rhs) { return ne(lhs, rhs) ? lhs < rhs : true; }
    template <typename L, typename R>
    typename enable_if<can_use_op<L>::value || can_use_op<R>::value, bool>::type ge(const L& lhs, const R& rhs) { return ne(lhs, rhs) ? lhs > rhs : true; }

    inline bool eq(const char* lhs, const char* rhs) { return String(lhs) == String(rhs); }
    inline bool ne(const char* lhs, const char* rhs) { return String(lhs) != String(rhs); }
    inline bool lt(const char* lhs, const char* rhs) { return String(lhs) <  String(rhs); }
    inline bool gt(const char* lhs, const char* rhs) { return String(lhs) >  String(rhs); }
    inline bool le(const char* lhs, const char* rhs) { return String(lhs) <= String(rhs); }
    inline bool ge(const char* lhs, const char* rhs) { return String(lhs) >= String(rhs); }
    // clang-format on

    template <typename L>
    struct Expression_lhs
    {
        L lhs;

        Expression_lhs(L in)
                : lhs(in) {}

        Expression_lhs(const Expression_lhs& other)
                : lhs(other.lhs) {}

        operator Result() { return Result(!!lhs, toString(lhs)); }

        // clang-format off
        template <typename R> Result operator==(const R& rhs) { return Result(eq(lhs, rhs), stringifyBinaryExpr(lhs, "==", rhs)); }
        template <typename R> Result operator!=(const R& rhs) { return Result(ne(lhs, rhs), stringifyBinaryExpr(lhs, "!=", rhs)); }
        template <typename R> Result operator< (const R& rhs) { return Result(lt(lhs, rhs), stringifyBinaryExpr(lhs, "<" , rhs)); }
        template <typename R> Result operator<=(const R& rhs) { return Result(le(lhs, rhs), stringifyBinaryExpr(lhs, "<=", rhs)); }
        template <typename R> Result operator> (const R& rhs) { return Result(gt(lhs, rhs), stringifyBinaryExpr(lhs, ">" , rhs)); }
        template <typename R> Result operator>=(const R& rhs) { return Result(ge(lhs, rhs), stringifyBinaryExpr(lhs, ">=", rhs)); }
        // clang-format on
    };

    struct ExpressionDecomposer
    {
        template <typename L>
        Expression_lhs<const L&> operator<<(const L& operand) {
            return Expression_lhs<const L&>(operand);
        }
    };

    // forward declarations of functions used by the macros
    int regTest(void (*f)(void), unsigned line, const char* file, const char* name);
    int setTestSuiteName(const char* name);

    void addFailedAssert(const char* assert_name);

    void logTestStart(const char* name, const char* file, unsigned line);
    void logTestEnd();

    void logTestCrashed();

    void logAssert(bool passed, const char* decomposition, bool threw, const char* expr,
                   const char* assert_name, const char* file, int line);

    void logAssertThrows(bool threw, const char* expr, const char* assert_name, const char* file,
                         int line);

    void logAssertThrowsAs(bool threw, bool threw_as, const char* as, const char* expr,
                           const char* assert_name, const char* file, int line);

    void logAssertNothrow(bool threw, const char* expr, const char* assert_name, const char* file,
                          int line);

    bool isDebuggerActive();
    void writeToDebugConsole(const String&);

    struct TestAccessibleContextState
    {
        bool            success;   // include successful assertions in output
        bool            no_throw;  // to skip exceptions-related assertion macros
        bool            no_breaks; // to not break into the debugger
        const TestData* currentTest;
        bool            hasLoggedCurrentTestStart;
        int             numAssertionsForCurrentTestcase;
    };

    struct ContextState;

    TestAccessibleContextState* getTestsContextState();

    namespace assertType
    {
        enum assertTypeEnum
        {
            normal,
            negated,
            throws,
            throws_as,
            nothrow
        };
    } // namespace assertType

    struct ResultBuilder
    {
        const char*                m_assert_name;
        assertType::assertTypeEnum m_assert_type;
        const char*                m_file;
        int                        m_line;
        const char*                m_expr;
        const char*                m_exception_type;

        Result m_res;
        bool   m_threw;
        bool   m_threw_as;
        bool   m_failed;

        ResultBuilder(const char* assert_name, assertType::assertTypeEnum assert_type,
                      const char* file, int line, const char* expr,
                      const char* exception_type = "");

// to fix gcc 4.7 "-Winline" warnings
#if defined(__GNUC__) && !defined(__clang__)
        __attribute__((noinline))
#endif
        ~ResultBuilder() {
        }

        void setResult(const Result& res) { m_res = res; }

        bool log();
        void react() const;
    };

    namespace fastAssertAction
    {
        enum Enum
        {
            nothing     = 0,
            dbgbreak    = 1,
            shouldthrow = 2
        };
    } // namespace fastAssertAction

    namespace fastAssertComparison
    {
        enum Enum
        {
            eq = 0,
            ne,
            gt,
            lt,
            ge,
            le
        };
    } // namespace fastAssertComparison

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif // __clang__
    inline const char* getCmpString(fastAssertComparison::Enum val) {
        switch(val) {
            case fastAssertComparison::eq: return "==";
            case fastAssertComparison::ne: return "!=";
            case fastAssertComparison::gt: return ">";
            case fastAssertComparison::lt: return "<";
            case fastAssertComparison::ge: return ">=";
            case fastAssertComparison::le: return "<=";
        }
        return "";
    }
#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__

    // clang-format off
    template <int, class L, class R> struct FastComparator     { bool operator()(const L&,     const R&    ) const { return true;       } };
    template <class L, class R> struct FastComparator<0, L, R> { bool operator()(const L& lhs, const R& rhs) const { return lhs == rhs; } };
    template <class L, class R> struct FastComparator<1, L, R> { bool operator()(const L& lhs, const R& rhs) const { return lhs != rhs; } };
    template <class L, class R> struct FastComparator<2, L, R> { bool operator()(const L& lhs, const R& rhs) const { return lhs >  rhs; } };
    template <class L, class R> struct FastComparator<3, L, R> { bool operator()(const L& lhs, const R& rhs) const { return lhs <  rhs; } };
    template <class L, class R> struct FastComparator<4, L, R> { bool operator()(const L& lhs, const R& rhs) const { return lhs >= rhs; } };
    template <class L, class R> struct FastComparator<5, L, R> { bool operator()(const L& lhs, const R& rhs) const { return lhs <= rhs; } };
    // clang-format on

    template <int comparison, typename L, typename R>
    int fast_assert(const char* assert_name, const char* file, int line, const char* lhs_str,
                    const char* rhs_str, const L& lhs, const R& rhs) {
        const char*   comp_str = getCmpString(static_cast<fastAssertComparison::Enum>(comparison));
        String        expr     = String(lhs_str) + " " + comp_str + " " + rhs_str;
        const char*   expr_str = expr.c_str();
        ResultBuilder rb(assert_name, doctest::detail::assertType::normal, file, line, expr_str);
        try {
            rb.m_res.m_passed        = FastComparator<comparison, L, R>()(lhs, rhs);
            rb.m_res.m_decomposition = stringifyBinaryExpr(lhs, comp_str, rhs);
        } catch(...) { rb.m_threw = true; }

        int res = 0;

        if(rb.log())
            res |= fastAssertAction::dbgbreak;

        if(rb.m_failed && checkIfShouldThrow(assert_name))
            res |= fastAssertAction::shouldthrow;

        return res;
    }

    template <typename T>
    int fast_assert_unary(const char* assert_name, const char* file, int line, const char* expr,
                          const T& val, bool is_false) {
        ResultBuilder rb(assert_name, doctest::detail::assertType::normal, file, line, expr);
        try {
            rb.m_res.m_passed        = !!val;
            rb.m_res.m_decomposition = toString(val);
            if(is_false)
                rb.m_res.m_passed = !rb.m_res.m_passed;
        } catch(...) { rb.m_threw = true; }

        int res = 0;

        if(rb.log())
            res |= fastAssertAction::dbgbreak;

        if(rb.m_failed && checkIfShouldThrow(assert_name))
            res |= fastAssertAction::shouldthrow;

        return res;
    }
} // namespace detail

#endif // DOCTEST_CONFIG_DISABLE

class Context
{
#if !defined(DOCTEST_CONFIG_DISABLE)
    detail::ContextState* p;

    void parseArgs(int argc, const char* const* argv, bool withDefaults = false);

#endif // DOCTEST_CONFIG_DISABLE

public:
    Context(int argc = 0, const char* const* argv = 0);

// to fix gcc 4.7 "-Winline" warnings
#if defined(__GNUC__) && !defined(__clang__)
    __attribute__((noinline))
#endif
    ~Context();

    void applyCommandLine(int argc, const char* const* argv);

    void addFilter(const char* filter, const char* value);
    void setOption(const char* option, int value);
    void setOption(const char* option, const char* value);

    bool shouldExit();

    int run();
};

} // namespace doctest

// if registering is not disabled
#if !defined(DOCTEST_CONFIG_DISABLE)

// registers the test by initializing a dummy var with a function
#if defined(__GNUC__) && !defined(__clang__)
#define DOCTEST_REGISTER_FUNCTION(f, name)                                                         \
    static int DOCTEST_ANONYMOUS(DOCTEST_ANON_VAR_) __attribute__((unused)) =                      \
            doctest::detail::regTest(f, __LINE__, __FILE__, name);
#elif defined(__clang__)
#define DOCTEST_REGISTER_FUNCTION(f, name)                                                         \
    _Pragma("clang diagnostic push")                                                               \
            _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"") static int               \
                    DOCTEST_ANONYMOUS(DOCTEST_ANON_VAR_) =                                         \
                            doctest::detail::regTest(f, __LINE__, __FILE__, name);                 \
    _Pragma("clang diagnostic pop")
#else // MSVC
#define DOCTEST_REGISTER_FUNCTION(f, name)                                                         \
    static int DOCTEST_ANONYMOUS(DOCTEST_ANON_VAR_) =                                              \
            doctest::detail::regTest(f, __LINE__, __FILE__, name);
#endif // MSVC

#define DOCTEST_IMPLEMENT_FIXTURE(der, base, func, name)                                           \
    namespace                                                                                      \
    {                                                                                              \
        struct der : base                                                                          \
        { void f(); };                                                                             \
        static void func() {                                                                       \
            der v;                                                                                 \
            v.f();                                                                                 \
        }                                                                                          \
        DOCTEST_REGISTER_FUNCTION(func, name)                                                      \
    }                                                                                              \
    inline void der::f()

#define DOCTEST_CREATE_AND_REGISTER_FUNCTION(f, name)                                              \
    static void f();                                                                               \
    DOCTEST_REGISTER_FUNCTION(f, name)                                                             \
    inline void f()

// for registering tests
#define DOCTEST_TEST_CASE(name)                                                                    \
    DOCTEST_CREATE_AND_REGISTER_FUNCTION(DOCTEST_ANONYMOUS(DOCTEST_ANON_FUNC_), name)

// for registering tests with a fixture
#define DOCTEST_TEST_CASE_FIXTURE(c, name)                                                         \
    DOCTEST_IMPLEMENT_FIXTURE(DOCTEST_ANONYMOUS(DOCTEST_ANON_CLASS_), c,                           \
                              DOCTEST_ANONYMOUS(DOCTEST_ANON_FUNC_), name)

// for subcases
#if defined(__GNUC__)
#define DOCTEST_SUBCASE(name)                                                                      \
    if(const doctest::detail::Subcase & DOCTEST_ANONYMOUS(DOCTEST_ANON_SUBCASE_)                   \
                                                __attribute__((unused)) =                          \
               doctest::detail::Subcase(name, __FILE__, __LINE__))
#else // __GNUC__
#define DOCTEST_SUBCASE(name)                                                                      \
    if(const doctest::detail::Subcase & DOCTEST_ANONYMOUS(DOCTEST_ANON_SUBCASE_) =                 \
               doctest::detail::Subcase(name, __FILE__, __LINE__))
#endif // __GNUC__

// for starting a testsuite block
#if defined(__GNUC__) && !defined(__clang__)
#define DOCTEST_TEST_SUITE(name)                                                                   \
    static int DOCTEST_ANONYMOUS(DOCTEST_ANON_VAR_) __attribute__((unused)) =                      \
            doctest::detail::setTestSuiteName(name);                                               \
    void DOCTEST_ANONYMOUS(DOCTEST_ANON_FOR_SEMICOLON_)()
#elif defined(__clang__)
#define DOCTEST_TEST_SUITE(name)                                                                   \
    _Pragma("clang diagnostic push")                                                               \
            _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"") static int               \
                    DOCTEST_ANONYMOUS(DOCTEST_ANON_VAR_) =                                         \
                            doctest::detail::setTestSuiteName(name);                               \
    _Pragma("clang diagnostic pop") void DOCTEST_ANONYMOUS(DOCTEST_ANON_FOR_SEMICOLON_)()
#else // MSVC
#define DOCTEST_TEST_SUITE(name)                                                                   \
    static int DOCTEST_ANONYMOUS(DOCTEST_ANON_VAR_) = doctest::detail::setTestSuiteName(name);     \
    void       DOCTEST_ANONYMOUS(DOCTEST_ANON_FOR_SEMICOLON_)()
#endif // MSVC

// for ending a testsuite block
#if defined(__GNUC__) && !defined(__clang__)
#define DOCTEST_TEST_SUITE_END                                                                     \
    static int DOCTEST_ANONYMOUS(DOCTEST_ANON_VAR_) __attribute__((unused)) =                      \
            doctest::detail::setTestSuiteName("");                                                 \
    void DOCTEST_ANONYMOUS(DOCTEST_ANON_TESTSUITE_END_)
#elif defined(__clang__)
#define DOCTEST_TEST_SUITE_END                                                                                         \
    _Pragma("clang diagnostic push")                                                                                   \
            _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"") static int                                   \
                                         DOCTEST_ANONYMOUS(DOCTEST_ANON_VAR_) = doctest::detail::setTestSuiteName(""); \
    _Pragma("clang diagnostic pop") void DOCTEST_ANONYMOUS(DOCTEST_ANON_TESTSUITE_END_)
#else // MSVC
#define DOCTEST_TEST_SUITE_END                                                                     \
    static int DOCTEST_ANONYMOUS(DOCTEST_ANON_VAR_) = doctest::detail::setTestSuiteName("");       \
    void       DOCTEST_ANONYMOUS(DOCTEST_ANON_TESTSUITE_END_)
#endif // MSVC

#define DOCTEST_ASSERT_LOG_AND_REACT(rb)                                                           \
    if(rb.log())                                                                                   \
        DOCTEST_BREAK_INTO_DEBUGGER();                                                             \
    rb.react()

#define DOCTEST_ASSERT_IMPLEMENT(expr, assert_name, assert_type)                                   \
    doctest::detail::ResultBuilder _DOCTEST_RB(                                                    \
            assert_name, doctest::detail::assertType::assert_type, __FILE__, __LINE__, #expr);     \
    try {                                                                                          \
        _DOCTEST_RB.setResult(doctest::detail::ExpressionDecomposer() << expr);                    \
    } catch(...) { _DOCTEST_RB.m_threw = true; }                                                   \
    DOCTEST_ASSERT_LOG_AND_REACT(_DOCTEST_RB);

#if defined(__clang__)
#define DOCTEST_ASSERT_PROXY(expr, assert_name, assert_type)                                       \
    do {                                                                                           \
        _Pragma("clang diagnostic push")                                                           \
                _Pragma("clang diagnostic ignored \"-Woverloaded-shift-op-parentheses\"")          \
                        DOCTEST_ASSERT_IMPLEMENT(expr, assert_name, assert_type)                   \
                                _Pragma("clang diagnostic pop")                                    \
    } while(doctest::detail::always_false())
#else // __clang__
#define DOCTEST_ASSERT_PROXY(expr, assert_name, assert_type)                                       \
    do {                                                                                           \
        DOCTEST_ASSERT_IMPLEMENT(expr, assert_name, assert_type)                                   \
    } while(doctest::detail::always_false())
#endif // __clang__

#define DOCTEST_WARN(expr) DOCTEST_ASSERT_PROXY(expr, "WARN", normal)
#define DOCTEST_CHECK(expr) DOCTEST_ASSERT_PROXY(expr, "CHECK", normal)
#define DOCTEST_REQUIRE(expr) DOCTEST_ASSERT_PROXY(expr, "REQUIRE", normal)

#define DOCTEST_WARN_FALSE(expr) DOCTEST_ASSERT_PROXY(expr, "WARN_FALSE", negated)
#define DOCTEST_CHECK_FALSE(expr) DOCTEST_ASSERT_PROXY(expr, "CHECK_FALSE", negated)
#define DOCTEST_REQUIRE_FALSE(expr) DOCTEST_ASSERT_PROXY(expr, "REQUIRE_FALSE", negated)

#define DOCTEST_ASSERT_THROWS(expr, assert_name)                                                   \
    do {                                                                                           \
        if(!DOCTEST_GCS().no_throw) {                                                              \
            doctest::detail::ResultBuilder _DOCTEST_RB(                                            \
                    assert_name, doctest::detail::assertType::throws, __FILE__, __LINE__, #expr);  \
            try {                                                                                  \
                expr;                                                                              \
            } catch(...) { _DOCTEST_RB.m_threw = true; }                                           \
            DOCTEST_ASSERT_LOG_AND_REACT(_DOCTEST_RB);                                             \
        }                                                                                          \
    } while(doctest::detail::always_false())

#define DOCTEST_ASSERT_THROWS_AS(expr, as, assert_name)                                            \
    do {                                                                                           \
        if(!DOCTEST_GCS().no_throw) {                                                              \
            doctest::detail::ResultBuilder _DOCTEST_RB(assert_name,                                \
                                                       doctest::detail::assertType::throws_as,     \
                                                       __FILE__, __LINE__, #expr, #as);            \
            try {                                                                                  \
                expr;                                                                              \
            } catch(as) {                                                                          \
                _DOCTEST_RB.m_threw    = true;                                                     \
                _DOCTEST_RB.m_threw_as = true;                                                     \
            } catch(...) { _DOCTEST_RB.m_threw = true; }                                           \
            DOCTEST_ASSERT_LOG_AND_REACT(_DOCTEST_RB);                                             \
        }                                                                                          \
    } while(doctest::detail::always_false())

#define DOCTEST_ASSERT_NOTHROW(expr, assert_name)                                                  \
    do {                                                                                           \
        if(!DOCTEST_GCS().no_throw) {                                                              \
            doctest::detail::ResultBuilder _DOCTEST_RB(                                            \
                    assert_name, doctest::detail::assertType::nothrow, __FILE__, __LINE__, #expr); \
            try {                                                                                  \
                expr;                                                                              \
            } catch(...) { _DOCTEST_RB.m_threw = true; }                                           \
            DOCTEST_ASSERT_LOG_AND_REACT(_DOCTEST_RB);                                             \
        }                                                                                          \
    } while(doctest::detail::always_false())

#define DOCTEST_WARN_THROWS(expr) DOCTEST_ASSERT_THROWS(expr, "WARN_THROWS")
#define DOCTEST_CHECK_THROWS(expr) DOCTEST_ASSERT_THROWS(expr, "CHECK_THROWS")
#define DOCTEST_REQUIRE_THROWS(expr) DOCTEST_ASSERT_THROWS(expr, "REQUIRE_THROWS")

#define DOCTEST_WARN_THROWS_AS(expr, ex) DOCTEST_ASSERT_THROWS_AS(expr, ex, "WARN_THROWS_AS")
#define DOCTEST_CHECK_THROWS_AS(expr, ex) DOCTEST_ASSERT_THROWS_AS(expr, ex, "CHECK_THROWS_AS")
#define DOCTEST_REQUIRE_THROWS_AS(expr, ex) DOCTEST_ASSERT_THROWS_AS(expr, ex, "REQUIRE_THROWS_AS")

#define DOCTEST_WARN_NOTHROW(expr) DOCTEST_ASSERT_NOTHROW(expr, "WARN_NOTHROW")
#define DOCTEST_CHECK_NOTHROW(expr) DOCTEST_ASSERT_NOTHROW(expr, "CHECK_NOTHROW")
#define DOCTEST_REQUIRE_NOTHROW(expr) DOCTEST_ASSERT_NOTHROW(expr, "REQUIRE_NOTHROW")

#define DOCTEST_FAST_ASSERTION(assert_name, lhs, rhs, comparison)                                  \
    do {                                                                                           \
        int res = doctest::detail::fast_assert<doctest::detail::fastAssertComparison::comparison>( \
                assert_name, __FILE__, __LINE__, #lhs, #rhs, lhs, rhs);                            \
        if(res & doctest::detail::fastAssertAction::dbgbreak)                                      \
            DOCTEST_BREAK_INTO_DEBUGGER();                                                         \
        if(res & doctest::detail::fastAssertAction::shouldthrow)                                   \
            doctest::detail::throwException();                                                     \
    } while(doctest::detail::always_false())

#define DOCTEST_FAST_ASSERTION_UNARY(assert_name, val, is_false)                                   \
    do {                                                                                           \
        int res = doctest::detail::fast_assert_unary(assert_name, __FILE__, __LINE__, #val, val,   \
                                                     is_false);                                    \
        if(res & doctest::detail::fastAssertAction::dbgbreak)                                      \
            DOCTEST_BREAK_INTO_DEBUGGER();                                                         \
        if(res & doctest::detail::fastAssertAction::shouldthrow)                                   \
            doctest::detail::throwException();                                                     \
    } while(doctest::detail::always_false())

#define DOCTEST_WARN_EQ(lhs, rhs) DOCTEST_FAST_ASSERTION("WARN_EQ", lhs, rhs, eq)
#define DOCTEST_CHECK_EQ(lhs, rhs) DOCTEST_FAST_ASSERTION("CHECK_EQ", lhs, rhs, eq)
#define DOCTEST_REQUIRE_EQ(lhs, rhs) DOCTEST_FAST_ASSERTION("REQUIRE_EQ", lhs, rhs, eq)
#define DOCTEST_WARN_NE(lhs, rhs) DOCTEST_FAST_ASSERTION("WARN_NE", lhs, rhs, ne)
#define DOCTEST_CHECK_NE(lhs, rhs) DOCTEST_FAST_ASSERTION("CHECK_NE", lhs, rhs, ne)
#define DOCTEST_REQUIRE_NE(lhs, rhs) DOCTEST_FAST_ASSERTION("REQUIRE_NE", lhs, rhs, ne)
#define DOCTEST_WARN_GT(lhs, rhs) DOCTEST_FAST_ASSERTION("WARN_GT", lhs, rhs, gt)
#define DOCTEST_CHECK_GT(lhs, rhs) DOCTEST_FAST_ASSERTION("CHECK_GT", lhs, rhs, gt)
#define DOCTEST_REQUIRE_GT(lhs, rhs) DOCTEST_FAST_ASSERTION("REQUIRE_GT", lhs, rhs, gt)
#define DOCTEST_WARN_LT(lhs, rhs) DOCTEST_FAST_ASSERTION("WARN_LT", lhs, rhs, lt)
#define DOCTEST_CHECK_LT(lhs, rhs) DOCTEST_FAST_ASSERTION("CHECK_LT", lhs, rhs, lt)
#define DOCTEST_REQUIRE_LT(lhs, rhs) DOCTEST_FAST_ASSERTION("REQUIRE_LT", lhs, rhs, lt)
#define DOCTEST_WARN_GE(lhs, rhs) DOCTEST_FAST_ASSERTION("WARN_GE", lhs, rhs, ge)
#define DOCTEST_CHECK_GE(lhs, rhs) DOCTEST_FAST_ASSERTION("CHECK_GE", lhs, rhs, ge)
#define DOCTEST_REQUIRE_GE(lhs, rhs) DOCTEST_FAST_ASSERTION("REQUIRE_GE", lhs, rhs, ge)
#define DOCTEST_WARN_LE(lhs, rhs) DOCTEST_FAST_ASSERTION("WARN_LE", lhs, rhs, le)
#define DOCTEST_CHECK_LE(lhs, rhs) DOCTEST_FAST_ASSERTION("CHECK_LE", lhs, rhs, le)
#define DOCTEST_REQUIRE_LE(lhs, rhs) DOCTEST_FAST_ASSERTION("REQUIRE_LE", lhs, rhs, le)

#define DOCTEST_WARN_UNARY(val) DOCTEST_FAST_ASSERTION_UNARY("WARN_UNARY", val, false)
#define DOCTEST_CHECK_UNARY(val) DOCTEST_FAST_ASSERTION_UNARY("CHECK_UNARY", val, false)
#define DOCTEST_REQUIRE_UNARY(val) DOCTEST_FAST_ASSERTION_UNARY("REQUIRE_UNARY", val, false)
#define DOCTEST_WARN_UNARY_FALSE(val) DOCTEST_FAST_ASSERTION_UNARY("WARN_UNARY_FALSE", val, true)
#define DOCTEST_CHECK_UNARY_FALSE(val) DOCTEST_FAST_ASSERTION_UNARY("CHECK_UNARY_FALSE", val, true)
#define DOCTEST_REQUIRE_UNARY_FALSE(val)                                                           \
    DOCTEST_FAST_ASSERTION_UNARY("REQUIRE_UNARY_FALSE", val, true)

// =================================================================================================
// == WHAT FOLLOWS IS VERSIONS OF THE MACROS THAT DO NOT DO ANY REGISTERING!                      ==
// == THIS CAN BE ENABLED BY DEFINING DOCTEST_CONFIG_DISABLE GLOBALLY!                            ==
// =================================================================================================
#else // DOCTEST_CONFIG_DISABLE

#define DOCTEST_IMPLEMENT_FIXTURE(der, base, func, name)                                           \
    namespace                                                                                      \
    {                                                                                              \
        template <typename T>                                                                      \
        struct der : base                                                                          \
        { void f(); };                                                                             \
    }                                                                                              \
    template <typename T>                                                                          \
    inline void der<T>::f()

#define DOCTEST_CREATE_AND_REGISTER_FUNCTION(f, name)                                              \
    template <typename T>                                                                          \
    static inline void f()

// for registering tests
#define DOCTEST_TEST_CASE(name)                                                                    \
    DOCTEST_CREATE_AND_REGISTER_FUNCTION(DOCTEST_ANONYMOUS(DOCTEST_ANON_FUNC_), name)

// for registering tests with a fixture
#define DOCTEST_TEST_CASE_FIXTURE(x, name)                                                         \
    DOCTEST_IMPLEMENT_FIXTURE(DOCTEST_ANONYMOUS(DOCTEST_ANON_CLASS_), x,                           \
                              DOCTEST_ANONYMOUS(DOCTEST_ANON_FUNC_), name)

// for subcases
#define DOCTEST_SUBCASE(name)

// for starting a testsuite block
#define DOCTEST_TEST_SUITE(name) void DOCTEST_ANONYMOUS(DOCTEST_ANON_FOR_SEMICOLON_)()

// for ending a testsuite block
#define DOCTEST_TEST_SUITE_END void DOCTEST_ANONYMOUS(DOCTEST_ANON_TESTSUITE_END_)

#define DOCTEST_WARN(expr) ((void)0)
#define DOCTEST_WARN_FALSE(expr) ((void)0)
#define DOCTEST_WARN_THROWS(expr) ((void)0)
#define DOCTEST_WARN_THROWS_AS(expr, ex) ((void)0)
#define DOCTEST_WARN_NOTHROW(expr) ((void)0)
#define DOCTEST_CHECK(expr) ((void)0)
#define DOCTEST_CHECK_FALSE(expr) ((void)0)
#define DOCTEST_CHECK_THROWS(expr) ((void)0)
#define DOCTEST_CHECK_THROWS_AS(expr, ex) ((void)0)
#define DOCTEST_CHECK_NOTHROW(expr) ((void)0)
#define DOCTEST_REQUIRE(expr) ((void)0)
#define DOCTEST_REQUIRE_FALSE(expr) ((void)0)
#define DOCTEST_REQUIRE_THROWS(expr) ((void)0)
#define DOCTEST_REQUIRE_THROWS_AS(expr, ex) ((void)0)
#define DOCTEST_REQUIRE_NOTHROW(expr) ((void)0)

#define DOCTEST_WARN_EQ(lhs, rhs) ((void)0)
#define DOCTEST_CHECK_EQ(lhs, rhs) ((void)0)
#define DOCTEST_REQUIRE_EQ(lhs, rhs) ((void)0)
#define DOCTEST_WARN_NE(lhs, rhs) ((void)0)
#define DOCTEST_CHECK_NE(lhs, rhs) ((void)0)
#define DOCTEST_REQUIRE_NE(lhs, rhs) ((void)0)
#define DOCTEST_WARN_GT(lhs, rhs) ((void)0)
#define DOCTEST_CHECK_GT(lhs, rhs) ((void)0)
#define DOCTEST_REQUIRE_GT(lhs, rhs) ((void)0)
#define DOCTEST_WARN_LT(lhs, rhs) ((void)0)
#define DOCTEST_CHECK_LT(lhs, rhs) ((void)0)
#define DOCTEST_REQUIRE_LT(lhs, rhs) ((void)0)
#define DOCTEST_WARN_GE(lhs, rhs) ((void)0)
#define DOCTEST_CHECK_GE(lhs, rhs) ((void)0)
#define DOCTEST_REQUIRE_GE(lhs, rhs) ((void)0)
#define DOCTEST_WARN_LE(lhs, rhs) ((void)0)
#define DOCTEST_CHECK_LE(lhs, rhs) ((void)0)
#define DOCTEST_REQUIRE_LE(lhs, rhs) ((void)0)

#define DOCTEST_WARN_UNARY(val) ((void)0)
#define DOCTEST_CHECK_UNARY(val) ((void)0)
#define DOCTEST_REQUIRE_UNARY(val) ((void)0)
#define DOCTEST_WARN_UNARY_FALSE(val) ((void)0)
#define DOCTEST_CHECK_UNARY_FALSE(val) ((void)0)
#define DOCTEST_REQUIRE_UNARY_FALSE(val) ((void)0)

#endif // DOCTEST_CONFIG_DISABLE

// BDD style macros
// clang-format off
#define DOCTEST_SCENARIO(name)  TEST_CASE("  Scenario: " name)
#define DOCTEST_GIVEN(name)     SUBCASE("   Given: " name)
#define DOCTEST_WHEN(name)      SUBCASE("    When: " name)
#define DOCTEST_AND_WHEN(name)  SUBCASE("And when: " name)
#define DOCTEST_THEN(name)      SUBCASE("    Then: " name)
#define DOCTEST_AND_THEN(name)  SUBCASE("     And: " name)
// clang-format on

// == SHORT VERSIONS OF THE MACROS
#if !defined(DOCTEST_CONFIG_NO_SHORT_MACRO_NAMES)

#define TEST_CASE DOCTEST_TEST_CASE
#define TEST_CASE_FIXTURE DOCTEST_TEST_CASE_FIXTURE
#define SUBCASE DOCTEST_SUBCASE
#define TEST_SUITE DOCTEST_TEST_SUITE
#define TEST_SUITE_END DOCTEST_TEST_SUITE_END
#define WARN DOCTEST_WARN
#define WARN_FALSE DOCTEST_WARN_FALSE
#define WARN_THROWS DOCTEST_WARN_THROWS
#define WARN_THROWS_AS DOCTEST_WARN_THROWS_AS
#define WARN_NOTHROW DOCTEST_WARN_NOTHROW
#define CHECK DOCTEST_CHECK
#define CHECK_FALSE DOCTEST_CHECK_FALSE
#define CHECK_THROWS DOCTEST_CHECK_THROWS
#define CHECK_THROWS_AS DOCTEST_CHECK_THROWS_AS
#define CHECK_NOTHROW DOCTEST_CHECK_NOTHROW
#define REQUIRE DOCTEST_REQUIRE
#define REQUIRE_FALSE DOCTEST_REQUIRE_FALSE
#define REQUIRE_THROWS DOCTEST_REQUIRE_THROWS
#define REQUIRE_THROWS_AS DOCTEST_REQUIRE_THROWS_AS
#define REQUIRE_NOTHROW DOCTEST_REQUIRE_NOTHROW

#define SCENARIO DOCTEST_SCENARIO
#define GIVEN DOCTEST_GIVEN
#define WHEN DOCTEST_WHEN
#define AND_WHEN DOCTEST_AND_WHEN
#define THEN DOCTEST_THEN
#define AND_THEN DOCTEST_AND_THEN

#define WARN_EQ DOCTEST_WARN_EQ
#define CHECK_EQ DOCTEST_CHECK_EQ
#define REQUIRE_EQ DOCTEST_REQUIRE_EQ
#define WARN_NE DOCTEST_WARN_NE
#define CHECK_NE DOCTEST_CHECK_NE
#define REQUIRE_NE DOCTEST_REQUIRE_NE
#define WARN_GT DOCTEST_WARN_GT
#define CHECK_GT DOCTEST_CHECK_GT
#define REQUIRE_GT DOCTEST_REQUIRE_GT
#define WARN_LT DOCTEST_WARN_LT
#define CHECK_LT DOCTEST_CHECK_LT
#define REQUIRE_LT DOCTEST_REQUIRE_LT
#define WARN_GE DOCTEST_WARN_GE
#define CHECK_GE DOCTEST_CHECK_GE
#define REQUIRE_GE DOCTEST_REQUIRE_GE
#define WARN_LE DOCTEST_WARN_LE
#define CHECK_LE DOCTEST_CHECK_LE
#define REQUIRE_LE DOCTEST_REQUIRE_LE
#define WARN_UNARY DOCTEST_WARN_UNARY
#define CHECK_UNARY DOCTEST_CHECK_UNARY
#define REQUIRE_UNARY DOCTEST_REQUIRE_UNARY
#define WARN_UNARY_FALSE DOCTEST_WARN_UNARY_FALSE
#define CHECK_UNARY_FALSE DOCTEST_CHECK_UNARY_FALSE
#define REQUIRE_UNARY_FALSE DOCTEST_REQUIRE_UNARY_FALSE

#endif // DOCTEST_CONFIG_NO_SHORT_MACRO_NAMES

// this is here to clear the 'current test suite' for the current translation unit - at the top
DOCTEST_TEST_SUITE_END();

#endif // DOCTEST_LIBRARY_INCLUDED

#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__

#if defined(__GNUC__) && !defined(__clang__)
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 6)
#pragma GCC diagnostic pop
#endif // > gcc 4.6
#endif // __GNUC__

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER
