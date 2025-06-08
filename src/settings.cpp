/**
   KQ is Copyright (C) 2002 by Josh Bolduc

   This file is part of KQ... a freeware RPG.

   KQ is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 2, or (at your
   option) any later version.

   KQ is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with KQ; see the file COPYING.  If not, write to
   the Free Software Foundation,
       675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "settings.h"
#include "kq.h"
#include "platform.h"

#include <SDL.h>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <string>

using std::filesystem::path;

static bool init_path = false;
static path user_dir;
static path data_dir;
static path lib_dir;

static std::string_view strip(std::string_view);

KConfig::KConfig(const path& _filename)
    : filename { _filename }
{
    std::ifstream is(filename);
    while (is)
    {
        std::string line_string;
        std::getline(is, line_string);
        auto line = strip(line_string);
        if (!line.empty())
        {
            if (line.front() == '[' && line.back() == ']')
            {
                // Have hit an unnamed section, therefore
                // don't load anything else
                break;
            }
            else
            {
                auto pos = line.find('=');
                if (pos != std::string_view::npos)
                {
                    std::string key { strip(line.substr(0, pos)) };
                    auto val = strip(line.substr(pos + 1));
                    int iv;
                    auto [_ptr, ec] = std::from_chars(val.begin(), val.end(), iv);
                    if (ec == std::errc {})
                    {
                        items.insert_or_assign(key, iv);
                    } // else: an error, what do we do with it?
                }
            }
        }
    }
}

int KConfig::get_config_int(std::string_view key, int defl) const
{
    auto it = items.find(key);
    if (it != items.end())
    {
        return it->second;
    }
    else
    {
        return defl;
    }
}

void KConfig::set_config_int(std::string_view key, int value)
{
    auto [it, inserted] = items.try_emplace(std::string { key });
    if (inserted || it->second != value)
    {
        dirty = true;
    }
    it->second = value;
}

void KConfig::flush()
{
    if (dirty && !filename.empty())
    {
        std::ofstream os(filename);
        for (const auto& i : items)
        {
            os << i.first << "=" << i.second << std::endl;
        }
    }
    dirty = false;
}

static std::string_view strip(std::string_view s)
{
    auto l = s.find_first_not_of(" \t");
    if (l == std::string_view::npos)
    {
        return {};
    }
    auto r = s.find_last_not_of(" \t");
    return s.substr(l, 1 + r - l);
}

/*! \brief Returns the full path for this file.
 *
 * This function first checks if the file can be found in the user's directory.
 * If it can not, it checks the relevant game directory (data, music, lib, etc).
 *
 * \param   str1 The first part of the path (i.e. the install dir, for example "/usr/local/share/kq/").
 * \param   str2 The second part of the string (eg. "maps").
 * \param   file The filename.
 * \returns The combined path.
 */
static path get_resource_file_path(const path& str1, const path& str2, const path& file)
{
    auto tail = str2 / file;
    auto ans = user_dir / tail;

    if (std::filesystem::exists(ans))
    {
        return ans;
    }
    else
    {
        return str1 / tail;
    }
}

/*! \brief Returns the full path for this lua file.
 *
 * This function first checks if the lua file can be found in the user's directory.
 * If it can not, it checks the relevant game directory (scripts).
 *
 * For each directory, it first checks for a lob file, and then it checks for a lua file.
 *
 * This function is similar to get_resource_file_path(), but takes special considerations for lua files.
 *
 * Whereas get_resource_file_path() takes the full filename (eg. "main.map"), this function takes the filename without
 * extension (eg "main").
 *
 * \param   str1 The first part of the path (the install path, eg. "/usr/local/lib/kq").
 * \param   file The filename.
 * \returns The combined path.
 */
static path get_lua_file_path(const path& str1, const path& file)
{
    std::string ans;
    std::string scripts { "scripts" };
    std::string lob { ".lob" };
    std::string lua { ".lua" };
    auto script = path { file };
    auto base = user_dir / path { "scripts" };

    ans = base / script.replace_extension(".lob");

    if (!std::filesystem::exists(ans))
    {
        ans = base / script.replace_extension(".lua");

        if (!std::filesystem::exists(ans))
        {
            base = str1 / scripts;
            ans = base / script.replace_extension(".lob");

            if (!std::filesystem::exists(ans))
            {
                ans = base / script.replace_extension(".lua");
                if (!std::filesystem::exists(ans))
                {
                    return {};
                }
            }
        }
    }

    return ans;
}

/*! \brief Return the name of 'significant' directories.
 *
 * \param   dir Enumerated constant for directory type \sa DATA_DIR et al.
 * \param   file File name below that directory.
 * \returns the combined path
 */
const path kqres(enum eDirectories dir, const path& file)
{
    if (!init_path)
    {
#ifdef KQ_SAVEDIR
        const char* save_folder = KQ_SAVEDIR;
#else
        const char* save_folder = "kq";
#endif /* KQ_SAVEDIR */
        user_dir = path { SDL_GetPrefPath("kq-fork", save_folder) };
        /* Always try to make the directory, just to be sure. */
        std::error_code ec;
        std::filesystem::create_directories(user_dir, ec);

        if (ec)
        {
            Game.program_death("Could not create user directory");
        }

/* Now the data directory */
#ifdef KQ_DATADIR
        /* We specified where... */
        data_dir = lib_dir = path { KQ_DATADIR };
#else  /* !KQ_DATADIR */
        /* ...or, use SDL's idea */
        data_dir = lib_dir = path { SDL_GetBasePath() };
#endif /* KQ_DATADIR */
        init_path = true;
    }
    switch (dir)
    {
    case eDirectories::DATA_DIR:
        return get_resource_file_path(data_dir, "data", file);
    case eDirectories::MUSIC_DIR:
        return get_resource_file_path(data_dir, "music", file);
    case eDirectories::MAP_DIR:
        return get_resource_file_path(data_dir, "maps", file);
    case eDirectories::SAVE_DIR:
    case eDirectories::SETTINGS_DIR:
        return get_resource_file_path(user_dir, "", file);
    case eDirectories::SCRIPT_DIR:
        return get_lua_file_path(lib_dir, file);
    }
}
