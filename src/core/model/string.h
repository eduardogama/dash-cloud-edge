/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef NS3_STRING_H
#define NS3_STRING_H

#include <zlib.h>


#include <vector>
#include <string>
#include "attribute-helper.h"

/**
 * \file
 * \ingroup attribute_String
 * ns3::StringValue attribute value declarations.
 */

namespace ns3 {


extern std::vector<std::string> Split(const std::string& s, const std::string& delimiter, const bool& removeEmptyEntries = false);

extern bool string_ends_width (std::string const &fullString, std::string const &ending);


extern std::string zlib_compress_string(const std::string& str,
                            int compressionlevel = Z_BEST_COMPRESSION);

extern std::string zlib_decompress_string(const std::string& str);

//  Additional docs for class StringValue:
/**
 * Hold variables of type string
 *
 * This class can be used to hold variables of type string,
 * that is, either char * or std::string.
 */
ATTRIBUTE_VALUE_DEFINE_WITH_NAME (std::string, String);
ATTRIBUTE_ACCESSOR_DEFINE (String);
ATTRIBUTE_CHECKER_DEFINE (String);

} // namespace ns3


#endif /* NS3_STRING_H */
