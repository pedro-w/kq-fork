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

#pragma once

#include <filesystem>
#include <map>
#include <stack>
#include <string>
#include <string_view>

class KConfig
{
  public:
    /*! \brief Set the file where the current config state should be stored.
     * \param   filename Full path to the file where this config state should be saved.
     */
    KConfig(const std::filesystem::path& filename);

    /**
     * Destroy this config.
     * Saves to disk if needed
     */
    ~KConfig() { flush(); }

    void set_config_int(std::string_view key, int value);
    int get_config_int(std::string_view key, int defl) const;
    /**
     *  Store config items to the supplied filename
     */
    void flush();

  private:
    using section_t = std::map<std::string, int, std::less<>>;
    section_t items;
    // Full path to file where config is stored.
    std::filesystem::path filename;

    // Whether or not any changes have been made to this configuration.
    bool dirty = false;
};
