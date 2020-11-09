//  Copyright (C) 2010-2017, Vaclav Haisman. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modifica-
//  tion, are permitted provided that the following conditions are met:
//
//  1. Redistributions of  source code must  retain the above copyright  notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
//  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//  FITNESS  FOR A PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN NO  EVENT SHALL  THE
//  APACHE SOFTWARE  FOUNDATION  OR ITS CONTRIBUTORS  BE LIABLE FOR  ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLU-
//  DING, BUT NOT LIMITED TO, PROCUREMENT  OF SUBSTITUTE GOODS OR SERVICES; LOSS
//  OF USE, DATA, OR  PROFITS; OR BUSINESS  INTERRUPTION)  HOWEVER CAUSED AND ON
//  ANY  THEORY OF LIABILITY,  WHETHER  IN CONTRACT,  STRICT LIABILITY,  OR TORT
//  (INCLUDING  NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT OF THE  USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <log4cplus/internal/env.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/fileinfo.h>
#include <log4cplus/streams.h>

#ifdef LOG4CPLUS_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef LOG4CPLUS_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef LOG4CPLUS_HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef LOG4CPLUS_HAVE_WCHAR_H
#include <wchar.h>
#endif

#ifdef LOG4CPLUS_HAVE_DIRECT_H
#include <direct.h>
#endif

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <memory>


namespace log4cplus { namespace internal {

#if defined(_WIN32)
tstring const dir_sep(LOG4CPLUS_TEXT("\\"));
#else
tstring const dir_sep(LOG4CPLUS_TEXT("/"));
#endif


namespace
{

struct free_deleter
{
    void
    operator () (void * ptr) const
    {
        std::free(ptr);
    }
};

} // namespace


#if defined (_WIN32) && defined (_MSC_VER)
static inline
errno_t
dup_env_var (wchar_t ** buf, std::size_t * buf_len, wchar_t const * name)
{
    return _wdupenv_s (buf, buf_len, name);
}


static inline
errno_t
dup_env_var (char ** buf, std::size_t * buf_len, char const * name)
{
    return _dupenv_s (buf, buf_len, name);
}

#endif

bool
get_env_var (tstring & value, tstring const & name)
{
#if defined (_WIN32) && defined (_MSC_VER)
    tchar * buf = nullptr;
    std::size_t buf_len = 0;
    errno_t eno = dup_env_var (&buf, &buf_len, name.c_str ());
    std::unique_ptr<tchar, free_deleter> val (buf);
    switch (eno)
    {
    case 0:
        // Success of the _dupenv_s() call but the variable might still
        // not be defined.
        if (buf)
            value.assign (buf, buf_len - 1);

        break;

    case ENOMEM:
        helpers::getLogLog ().error (
            LOG4CPLUS_TEXT ("_dupenv_s failed to allocate memory"));
        throw std::bad_alloc ();

    default:
        helpers::getLogLog().error(
            LOG4CPLUS_TEXT ("_dupenv_s failed. Error: ")
            + helpers::convertIntegerToString (eno), true);
    }

    return !! buf;

#elif defined (_WIN32) && defined (UNICODE)
    tchar const * val = _wgetenv (name.c_str ());
    if (val)
        value = val;

    return !! val;

#else
    char const * val
        = std::getenv (LOG4CPLUS_TSTRING_TO_STRING (name).c_str ());
    if (val)
        value = LOG4CPLUS_STRING_TO_TSTRING (val);

    return !! val;

#endif
}


bool
parse_bool (bool & val, tstring const & str)
{
    log4cplus::tistringstream iss (str);
    log4cplus::tstring word;

    // Read a single "word".

    if (! (iss >> word))
        return false;

    // Following extraction of a character should fail
    // because there should only be a single "word".

    tchar ch;
    if (iss >> ch)
        return false;

    // Compare with "true" and "false".

    word = helpers::toLower (word);
    bool result = true;
    if (word == LOG4CPLUS_TEXT ("true"))
        val = true;
    else if (word == LOG4CPLUS_TEXT ("false"))
        val = false;

    // Try to interpret the "word" as a number.

    else
    {
        // Seek back to the start of stream.

        iss.clear ();
        iss.seekg (0);
        assert (iss);

        // Extract value.

        long lval;
        iss >> lval;

        // The extraction should be successful and
        // following extraction of a characters should fail.

        result = !! iss && ! (iss >> ch);
        if (result)
            val = !! lval;
    }

    return result;
}


namespace
{

struct path_sep_comp
{
    path_sep_comp ()
    { }

    bool
    operator () (tchar ch) const
    {
#if defined (_WIN32)
        return ch == LOG4CPLUS_TEXT ('\\') || ch == LOG4CPLUS_TEXT ('/');
#else
        return ch == LOG4CPLUS_TEXT ('/');
#endif
    }
};

} // namespace


template <typename Cont>
static
void
remove_empty (Cont & cont, std::size_t special)
{
    cont.erase (
        std::remove_if (cont.begin () + special, cont.end (),
            [](tstring const & str) { return str.empty (); }),
        cont.end ());
}


#if defined(_WIN32)
static
bool
is_drive_letter (tchar ch)
{
    tchar dl = helpers::toUpper (ch);
    return LOG4CPLUS_TEXT ('A') <= dl && dl <= LOG4CPLUS_TEXT ('Z');
}


static
tstring
get_drive_cwd (tchar drive)
{
    drive = helpers::toUpper (drive);

#ifdef UNICODE
    std::unique_ptr<wchar_t, free_deleter> cstr (
        _wgetdcwd(drive - LOG4CPLUS_TEXT('A') + 1, 0, 0x7FFF));
    if (! cstr)
    {
        int const eno = errno;
        helpers::getLogLog ().error (
            LOG4CPLUS_TEXT ("_wgetdcwd: ")
            + helpers::convertIntegerToString (eno),
            true);
    }

#else
    std::unique_ptr<char, free_deleter> cstr(
        _getdcwd (drive - LOG4CPLUS_TEXT ('A') + 1, 0, 0x7FFF));
    if (! cstr)
    {
        int const eno = errno;
        helpers::getLogLog ().error (
            LOG4CPLUS_TEXT ("_getdcwd: ")
            + helpers::convertIntegerToString (eno),
            true);
    }

#endif

    return cstr.get ();
}

#endif


static
tstring
get_current_dir ()
{
#if defined (WIN32)
    tstring result (0x8000, LOG4CPLUS_TEXT ('\0'));
    DWORD len = GetCurrentDirectory (static_cast<DWORD>(result.size ()),
        &result[0]);
    if (len == 0 || len >= result.size())
        throw std::runtime_error ("GetCurrentDirectory");

    result.resize (len);
    return result;

#else
    std::string buf;
    std::string::size_type buf_size = 1024;
    char * ret;
    do
    {
        buf.resize (buf_size);
        ret = getcwd (&buf[0], buf.size ());
        if (! ret)
        {
            int const eno = errno;
            if (eno == ERANGE)
                buf_size *= 2;
            else
                helpers::getLogLog ().error (
                    LOG4CPLUS_TEXT ("getcwd: ")
                    + helpers::convertIntegerToString (eno),
                    true);
        }
    }
    while (! ret);

    buf.resize (std::strlen (buf.c_str ()));
    return LOG4CPLUS_STRING_TO_TSTRING (buf);

#endif
}


#if defined (WIN32)
static
tchar
get_current_drive ()
{
    tstring const cwd = get_current_dir ();
    if (cwd.size () >= 2
        && cwd[1] == LOG4CPLUS_TEXT (':'))
        return cwd[0];
    else
        return 0;
}
#endif


template <typename PathSepPred, typename Container>
static
void
split_into_components(Container & components, tstring const & path,
    PathSepPred is_sep = PathSepPred ())
{
    tstring::const_iterator const end = path.end ();
    tstring::const_iterator it = path.begin ();
    while (it != end)
    {
        tstring::const_iterator sep = std::find_if (it, end, is_sep);
        components.push_back (tstring (it, sep));
        it = sep;
        if (it != end)
            ++it;
    }
}


template <typename PathSepPred, typename Container>
static
void
expand_drive_relative_path (Container & components, std::size_t rel_path_index,
    PathSepPred is_sep = PathSepPred ())
{
    // Save relative path attached to drive,
    // e.g., relpath in "C:relpath\foo\bar".

    tstring relative_path_first_component (components[rel_path_index], 2);

    // Get current working directory of a drive.

    tstring const drive_path = get_drive_cwd (components[rel_path_index][0]);

    // Split returned path.

    std::vector<tstring> drive_cwd_components;
    split_into_components (drive_cwd_components, drive_path,
        is_sep);

    // Move the saved relative path into place.

    components[rel_path_index] = std::move (relative_path_first_component);

    // Insert the current working directory for a drive.

    components.insert (components.begin () + rel_path_index,
        drive_cwd_components.begin (),
        drive_cwd_components.end ());
}


template <typename PathSepPred, typename Container>
static
void
expand_relative_path (Container & components,
    PathSepPred is_sep = PathSepPred ())
{
    // Get the current working director.

    tstring const cwd = get_current_dir ();

    // Split the CWD.

    std::vector<tstring> cwd_components;

    // Use qualified call to appease IBM xlC.
    internal::split_into_components (cwd_components, cwd, is_sep);

    // Insert the CWD components at the beginning of components.

    components.insert (components.begin (), cwd_components.begin (),
        cwd_components.end ());
}


bool
split_path (std::vector<tstring> & components, std::size_t & special,
    tstring const & path)
{
    components.reserve (10);
    special = 0;

    // First split the path into individual components separated by
    // system specific separator.

    const path_sep_comp is_sep;
    split_into_components (components, path, is_sep);

    // Try to recognize the path to find out how many initial components
    // of the path are special and should not be attempted to be created
    // using mkdir().

retry_recognition:;
    std::size_t const comp_count = components.size ();

#if defined (_WIN32)
    std::size_t comp_0_size;

    // Special Windows paths recognition:
    // \\?\UNC\hostname\share\ - long UNC path
    // \\?\ - UNC path
    // \\.\ - for special devices like COM1
    // \\hostname\share\ - shared folders

    // "" "" "?" "UNC" "hostname" "share" "file or dir"
    // "" "" "?" "UNC" "server" "volume" "file or dir"
    if (comp_count >= 7
        && components[0].empty ()
        && components[1].empty ()
        && components[2] == LOG4CPLUS_TEXT ("?")
        && (components[3].size () == 3
            && helpers::toUpper (components[3][0]) == LOG4CPLUS_TEXT ('U')
            && helpers::toUpper (components[3][1]) == LOG4CPLUS_TEXT ('N')
            && helpers::toUpper (components[3][2]) == LOG4CPLUS_TEXT ('C')))
    {
        remove_empty (components, 2);
        special = 6;
        return components.size () >= special + 1;
    }
    // comp_count >= 6: "" "" "?" "hostname" "share" "file or dir"
    // comp_count >= 5: "" "" "?" "C:" "file or dir"
    else if (comp_count >= 5
        && components[0].empty ()
        && components[1].empty ()
        && components[2] == LOG4CPLUS_TEXT ("?"))
    {
        remove_empty (components, 2);

        std::size_t const comp_3_size = components[3].size ();
        // "C:"
        if (comp_3_size >= 2
            && is_drive_letter (components[3][0])
            && components[3][1] == LOG4CPLUS_TEXT (':'))
        {
            if (comp_3_size > 2)
                expand_drive_relative_path (components, 3, is_sep);

            special = 4;
        }
        // "hostname" "share"
        else
            special = 5;

        return components.size () >= special + 1;
    }
    // "" "" "hostname" "share" "file or dir"
    else if (comp_count >= 5
        && components[0].empty ()
        && components[1].empty ())
    {
        remove_empty (components, 2);
        special = 4;
        return components.size () >= special + 1;
    }
    // "" "" "." "device"
    else if (comp_count >= 4
        && components[0].empty ()
        && components[1].empty ()
        && components[2] == LOG4CPLUS_TEXT ("."))
    {
        remove_empty (components, 3);
        special = 3;
        return components.size () >= special + 1;
    }
    // "" "file", i.e. "\path\to\file"
    else if (comp_count >= 2
        && components[0].empty ()
        && ! components[1].empty ())
    {
        remove_empty (components, 1);

        tchar const cur_drive = get_current_drive ();
        if (! cur_drive)
            // Current path is not on a drive. It is likely on a share.
            return false;

        // Replace the first empty component with the current drive.
        components[0] = cur_drive;
        components[0] += LOG4CPLUS_TEXT (':');

        special = 1;

        return true;
    }
    // comp_count >= 2: "C:" "file"
    // comp_count >= 2: "C:relpath" "file"
    // comp_count >= 1: "C:relpath"
    else if (comp_count >= 1
        && (comp_0_size = components[0].size ()) >= 2
        && is_drive_letter (components[0][0])
        && components[0][1] == LOG4CPLUS_TEXT (':'))
    {
        remove_empty (components, 1);

        // "C:relpath"
        if (comp_0_size > 2)
            expand_drive_relative_path (components, 0, is_sep);

        special = 1;
        return components.size () >= special + 1;
    }
#else
#if defined (__CYGWIN__)
    // Cygwin treats //foo/bar/baz as a path
    // to "baz" on Windows share "bar" on host "foo".

    // "" "host" "share" "file"
    if (comp_count >= 4
        && components[0].empty ()
        && components[1].empty ())
    {
        remove_empty (components, 3);
        special = 3;
        return components.size () >= special + 1;
    }
    else
#endif
    // "" "file", e.g., "/var/log/foo.0"
    if (comp_count >= 2
        && components[0].empty ())
    {
        remove_empty (components, 1);
        special = 1;
        return components.size () >= special + 1;
    }
#endif

    // "relative\path\to\some\file.log
    else
    {
        remove_empty (components, 0);
        expand_relative_path (components, is_sep);
        goto retry_recognition;
    }
}


static
long
make_directory (tstring const & dir)
{
#if defined (_WIN32)
#  if defined (UNICODE)
    if (_wmkdir (dir.c_str ()) == 0)
#  else
    if (_mkdir (dir.c_str ()) == 0)
#  endif

#else
    if (mkdir (LOG4CPLUS_TSTRING_TO_STRING (dir).c_str (), 0777) == 0)

#endif
        return 0;
    else
        return errno;
}


static
void
loglog_make_directory_result (helpers::LogLog & loglog, tstring const & path,
    long ret)
{
    if (ret == 0)
    {
        loglog.debug (
            LOG4CPLUS_TEXT("Created directory ")
            + path);
    }
    else
    {
        tostringstream oss;
        oss << LOG4CPLUS_TEXT("Failed to create directory ")
            << path
            << LOG4CPLUS_TEXT("; error ")
            << ret;
        loglog.error (oss.str ());
    }
}


//! Creates missing directories in file path.
void
make_dirs (tstring const & file_path)
{
    std::vector<tstring> components;
    std::size_t special = 0;
    helpers::LogLog & loglog = helpers::getLogLog();

    // Split file path into components.

    if (! internal::split_path (components, special, file_path))
        return;

    // Remove file name from path components list.

    components.pop_back ();

    // Loop over path components, starting first non-special path component.

    tstring path;
    helpers::join (path, components.begin (), components.begin () + special,
        dir_sep);

    for (std::size_t i = special, components_size = components.size ();
        i != components_size; ++i)
    {
        path += dir_sep;
        path += components[i];

        // Check whether path exists.

        helpers::FileInfo fi;
        if (helpers::getFileInfo (&fi, path) == 0)
            // This directory exists. Move forward onto another path component.
            continue;

        // Make new directory.

        long const eno = make_directory (path);
        loglog_make_directory_result (loglog, path, eno);
    }
}


} } // namespace log4cplus { namespace internal {
