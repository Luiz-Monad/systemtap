#ifndef UTIL_H
#define UTIL_H

#include "config.h"
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <set>
#include <iomanip>
#include <map>
#include <algorithm>
#include <limits>

extern "C" {
#if ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#endif
#include <signal.h>
#include <stdint.h>
#include <spawn.h>
#include <assert.h>
#include <poll.h>
}

#include <sys/types.h>

// Sanity check C++11 support.  We're only requiring GCC 4.4's level of
// functionality, so trust either __cplusplus or the GNUC macros.
#if !(__cplusplus >= 201103L || \
  (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)))
#error "C++11 support is required!"
#endif

// NB: GCC didn't add C++11 final/override until 4.7, but until then it also
// only had __cplusplus=1 regardless of -std (GCC PR1773).  So checking new
// __cplusplus is probably good enough here; otherwise we should autoconf it.
#if __cplusplus >= 201103L
#define cxx_final final
#define cxx_override override
#else
#define cxx_final
#define cxx_override
#endif

#include "privilege.h"
#include "stringtable.h"

#if ENABLE_NLS
#define _(string) gettext(string)
#define _N(string, string_plural, count) \
        ngettext((string), (string_plural), (count))
#else
#define _(string) (string)
#define _N(string, string_plural, count) \
        ( (count) == 1 ? (string) : (string_plural) )
#endif
#define _F(format, ...) autosprintf(_(format), __VA_ARGS__)
#define _NF(format, format_plural, count, ...) \
        autosprintf(_N((format), (format_plural), (count)), __VA_ARGS__)
#define _STRINGIFY_MORE(s) #s
#define _STRINGIFY(s) _STRINGIFY_MORE(s)
#define ERR_SRC (std::string(__FILE__) + ":" + _STRINGIFY(__LINE__))
#define SEMANTIC_ERROR(...) semantic_error(ERR_SRC, __VA_ARGS__)
#define PARSE_ERROR(...) parse_error(ERR_SRC, __VA_ARGS__)

const char *get_home_directory(void);
size_t get_file_size(const std::string &path);
size_t get_file_size(int fd);
bool file_exists (const std::string &path);
bool dir_exists(const std::string &path);
bool copy_file(const std::string& src, const std::string& dest,
               bool verbose=false);
int create_dir(const char *dir, int mode = 0777);
int remove_file_or_dir(const char *dir);
void split_path(std::string &path, std::string &dir, std::string &entry);
extern "C" gid_t get_gid (const char *group_name);
int appendenv (const char *env_name, const std::string source);
bool in_group_id (gid_t target_gid);
std::string getmemusage ();
void tokenize(const std::string& str, std::vector<std::string>& tokens,
	      const std::string& delimiters = " ");
void tokenize_full(const std::string& str, std::vector<std::string>& tokens,
	      const std::string& delimiters = " ");
void tokenize_cxx(const std::string& str, std::vector<std::string>& tokens);
std::vector<std::pair<const char*,int> > split_lines(const char *buf, size_t n);
std::string find_executable(const std::string& name);
std::string find_executable(const std::string& name,
			    const std::string& sysroot,
			    const std::map<std::string,std::string>& sysenv,
			    const std::string& env_path = "PATH");
bool is_fully_resolved(const std::string& name,
                       const std::string& sysroot,
                       const std::map<std::string,std::string>& sysenv,
                       const std::string& env_path = "PATH");
const std::string cmdstr_quoted(const std::string& cmd);
const std::string detox_path(const std::string& str);
const std::string cmdstr_join(const std::vector<std::string>& cmds);
const std::string join(const std::vector<std::string>& cmds,
		       const std::string& delim);
int stap_waitpid(int verbose, pid_t pid);
pid_t stap_spawn(int verbose, const std::vector<std::string>& args);
pid_t stap_spawn(int verbose, const std::vector<std::string>& args,
		 posix_spawn_file_actions_t* fa, const std::vector<std::string>& envVec = std::vector<std::string> ());
pid_t stap_spawn_piped(int verbose, const std::vector<std::string>& args,
                       int* child_in=NULL, int* child_out=NULL, int* child_err=NULL);
int stap_system(int verbose, const std::string& description,
                const std::vector<std::string>& args,
                bool null_out=false, bool null_err=false);
inline int stap_system(int verbose, const std::vector<std::string>& args,
                       bool null_out=false, bool null_err=false)
{ return stap_system(verbose, args.front(), args, null_out, null_err); }
int stap_system_read(int verbose, const std::vector<std::string>& args, std::ostream& out);
std::pair<bool,int> stap_fork_read(int verbose, std::ostream& out);
int kill_stap_spawn(int sig);
bool is_build_id(const std::string& str);
void assert_regexp_match (const std::string& name, const std::string& value, const std::string& re);
int regexp_match (const std::string& value, const std::string& re, std::vector<std::string>& matches);
bool contains_glob_chars (const std::string &str);
std::string escape_glob_chars (const std::string& str);
std::string unescape_glob_chars (const std::string& str);
std::string csh_to_ksh (const std::string& csh);
bool identifier_string_needs_escape (const std::string& str);
std::string escaped_identifier_string (const std::string& str);
std::string escaped_character (unsigned c);
std::string escaped_literal_string (const std::string& str);
std::string kernel_release_from_build_tree (const std::string &kernel_build_tree, int verbose = 0);
std::string normalize_machine(const std::string& machine);
int elf_class_from_normalized_machine(const std::string& machine);
std::string autosprintf(const char* format, ...) __attribute__ ((format (printf, 1, 2)));
const std::set<std::string>& localization_variables();
std::string get_self_path();
bool is_valid_pid (pid_t pid, std::string& err_msg);

// stringification generics


template <typename IN>
inline std::string lex_cast(IN const & in)
{
  std::ostringstream ss;
  if (!(ss << in))
    throw std::runtime_error(_("bad lexical cast"));
  return ss.str();
}

#if __cplusplus < 201103L
// Older C++0x only had the "long long" implementations, so we cast up.
#define INT_TO_STRING(IN) \
  LEX_CAST_TO_STRING(signed IN, long long) \
  LEX_CAST_TO_STRING(unsigned IN, unsigned long long)
#else
// Otherwise, keep the values native.
#define INT_TO_STRING(IN) \
  LEX_CAST_TO_STRING(signed IN, signed IN) \
  LEX_CAST_TO_STRING(unsigned IN, unsigned IN)
#endif

#define LEX_CAST_TO_STRING(IN, CAST) \
  template <> \
  inline std::string lex_cast(IN const & in) \
  { \
    return std::to_string(static_cast<CAST>(in)); \
  }

INT_TO_STRING(char)
INT_TO_STRING(short)
INT_TO_STRING(int)
INT_TO_STRING(long)
INT_TO_STRING(long long)

#undef INT_TO_STRING
#undef LEX_CAST_TO_STRING


template <typename OUT>
inline OUT lex_cast(std::string const & in)
{
  std::istringstream ss(in);
  OUT out;
  if (!(ss >> out && ss.eof()))
    throw std::runtime_error(_("bad lexical cast"));
  return out;
}


// We want [u]int8_t to be treated numerically, not just extracting a char.
template <>
inline int8_t lex_cast(std::string const & in)
{
  int16_t out = lex_cast<int16_t>(in);
  if (out < -128 || out > 127)
    throw std::runtime_error(_("bad lexical cast"));
  return out;
}
template <>
inline uint8_t lex_cast(std::string const & in)
{
  uint16_t out = lex_cast<uint16_t>(in);
  if (out > 0xff && out < 0xff80) // don't error if it looks sign-extended
    throw std::runtime_error(_("bad lexical cast"));
  return out;
}


template <typename IN>
inline std::string
lex_cast_hex(IN const & in)
{
  std::ostringstream ss;
  if (!(ss << std::showbase << std::hex << in << std::dec))
    throw std::runtime_error(_("bad lexical cast"));
  return ss.str();
}

//Convert binary data to hex data.
template <typename IN>
inline std::string
hex_dump(IN const & in,  size_t len)
{
  std::ostringstream ss;
  unsigned i;
  if (!(ss << std::hex << std::setfill('0')))
    throw std::runtime_error(_("bad lexical cast"));

  for(i = 0; i < len; i++)
  {
    int temp = in[i];
    ss << std::setw(2) << temp;
  }
  std::string hex = ss.str();
  assert(hex.length() == 2 * len);
  return hex;
}

// Return as quoted string, so that when compiled as a C literal, it
// would print to the user out nicely.
template <typename IN>
inline std::string
lex_cast_qstring(IN const & in)
{
  std::stringstream ss;
  if (!(ss << in))
    throw std::runtime_error(_("bad lexical cast"));
  return lex_cast_qstring(ss.str());
}


template <>
inline std::string
lex_cast_qstring(std::string const & in)
{
  std::string out;
  out += '"';
  for (const char *p = in.c_str(); *p; ++p)
    {
      unsigned char c = *p;
      if (! isprint(c))
        {
          out += '\\';
          // quick & dirty octal converter
          out += "01234567" [(c >> 6) & 0x07];
          out += "01234567" [(c >> 3) & 0x07];
          out += "01234567" [(c >> 0) & 0x07];
        }
      else if (c == '"' || c == '\\')
        {
          out += '\\';
          out += c;
        }
      else
        out += c;
    }
  out += '"';
  return out;
}


// Delete all values from a map-like container and clear it
// (The template is permissive -- be good!)
template <typename T>
void delete_map(T& t)
{
  for (typename T::iterator i = t.begin(); i != t.end(); ++i)
    delete i->second;
  t.clear();
}


// Automatically save a variable, and restore it at the
// end of the function.
template <class V>
class save_and_restore
{
    V* ptr;
    V previous_value;

  public:
    // Optionally pass a second argument to the constructor to initialize the
    // variable to some value, after saving its old value.
    save_and_restore(V* ptr_in, V value_in): ptr(ptr_in), previous_value(*ptr_in) { *ptr_in = value_in; }
    save_and_restore(V*ptr_in): ptr(ptr_in), previous_value(*ptr_in){}

    // Retrieve the old value and restore it in the destructor
    ~save_and_restore() { *ptr = previous_value; }
};

template <typename T>
inline bool vector_has(std::vector<T>& v, T item)
{
  return std::find(v.begin(), v.end(), item) != v.end();
}

// Returns whether a string starts with the given prefix
inline bool
startswith(const std::string & s, const char * prefix)
{
  return (s.compare(0, std::strlen(prefix), prefix) == 0);
}

inline bool
startswith(const std::string & s, const std::string & prefix)
{
  return (s.compare(0, prefix.length(), prefix) == 0);
}

// Returns whether a string ends with the given suffix
inline bool
endswith(const std::string & s, const char * suffix)
{
  size_t s_len = s.size(), suffix_len = std::strlen(suffix);
  if (suffix_len > s_len)
    return false;
  return (s.compare(s_len - suffix_len, suffix_len, suffix) == 0);
}


// Mask our usual signals for the life of this object.
struct stap_sigmasker {
    sigset_t old;
    stap_sigmasker()
      {
        sigset_t mask;
        sigemptyset (&mask);
        sigaddset (&mask, SIGHUP);
        sigaddset (&mask, SIGPIPE);
        sigaddset (&mask, SIGINT);
        sigaddset (&mask, SIGTERM);
        sigprocmask (SIG_BLOCK, &mask, &old);
      }
    stap_sigmasker(const sigset_t *mask)
      {
        sigprocmask (SIG_BLOCK, mask, &old);
      }
    ~stap_sigmasker()
      {
        sigprocmask (SIG_SETMASK, &old, NULL);
      }
};

// Convert a possibly-relative path to a full path
inline std::string
resolve_path(const std::string& path)
{
  std::string result(path);
  char* resolved_path = realpath(path.c_str(), NULL);
  if (resolved_path)
    {
      result = resolved_path;
      std::free(resolved_path);
    }
  return result;
}

// Used in levenshtein
template<typename T>
class Array2D
{
  private:
    T * data;
  public:
    const unsigned width;
    const unsigned height;
    T& operator() (unsigned x, unsigned y) { return data[y*width + x]; }
    Array2D(const unsigned w, const unsigned h) : width(w), height(h) { data = new T[w*h]; }
    ~Array2D() { delete [] data; }
};

// String sorter using the Levenshtein algorithm
unsigned levenshtein(const std::string& a, const std::string& b);

// Returns comma-separated list of set elements closest to the target string.
// Print a maximum amount of 'max' elements, with a maximum levenshtein score
// of 'threshold'.
std::string levenshtein_suggest(const std::string& target,
                                const std::set<std::string>& elems,
                                unsigned max = std::numeric_limits<unsigned>::max(),
                                unsigned threshold = std::numeric_limits<unsigned>::max());

std::string levenshtein_suggest(const std::string& target,
                                const std::set<interned_string>& elems,
                                unsigned max = std::numeric_limits<unsigned>::max(),
                                unsigned threshold = std::numeric_limits<unsigned>::max());

#ifndef HAVE_PPOLL
// This is a poor-man's ppoll; see the implementation for more details...
int ppoll(struct pollfd *fds, nfds_t nfds,
          const struct timespec *timeout_ts,
          const sigset_t *sigmask);
#endif


int read_from_file (const std::string &fname, int &data);
template <class T>
int write_to_file (const std::string &fname, const T &data);
int flush_to_stream (const std::string &fname, std::ostream &o);

void ltrim(std::string &s);
void rtrim(std::string &s);
void trim(std::string &s);

bool get_distro_info(std::vector<std::string> &info);

#endif // UTIL_H
/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
