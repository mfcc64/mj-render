/*
 * Copyright (C) 2020 Muhammad Faiz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MJ_SURFACE_H
#define MJ_SURFACE_H 1

template<typename T>
class MJ_Surface {
public:
    inline MJ_Surface(int width, int height)
    {
        m_width = width;
        m_height = height;
        m_ptr = new T[width * height];
    }

    inline ~MJ_Surface()
    {
        delete[] m_ptr;
    }

    inline T& operator()(int x, int y) const
    {
        return m_ptr[m_width * y + x];
    }

    inline int width() const
    {
        return m_width;
    }

    inline int height() const
    {
        return m_height;
    }

private:
    T   *m_ptr;
    int m_width;
    int m_height;
};

#endif
