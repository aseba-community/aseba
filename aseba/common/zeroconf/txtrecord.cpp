/*
    Aseba - an event-based framework for distributed robot control
    Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
    with contributions from the community.
    Copyright (C) 2007--2018 the authors, see authors.txt for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#include <dashel/dashel.h>
#include "zeroconf.h"

using namespace std;

namespace Aseba {
//! TXT data for an Aseba target, with node ids and product ids
Zeroconf::TxtRecord::TxtRecord(const unsigned int protovers, const std::string& type, bool busy,
                               const std::vector<unsigned int>& ids, const std::vector<unsigned int>& pids) {
    assign("txtvers", 1);
    assign("protovers", int(protovers));
    assign("type", type);
    assign("busy", busy);
    assign("ids", ids);
    assign("pids", pids);
}

//! TXT data for an Aseba target, deserialized from a TXT record string
Zeroconf::TxtRecord::TxtRecord(const std::string& txtRecord) {
    istringstream txt{txtRecord};
    do {
        char length;
        txt >> length;
        string data;
        txt >> setw(int(length)) >> data;
        auto key_pos = data.find('=');
        fields[data.substr(0, key_pos)] = (key_pos == std::string::npos) ? "" : data.substr(key_pos + 1);
    } while(txt.peek() != std::char_traits<char>::eof());
}

//! TXT data for an Aseba target, deserialized from a TXT record buffer
Zeroconf::TxtRecord::TxtRecord(const unsigned char* txtRecord, uint16_t txtLen) {
    size_t pos = 0;
    while(pos < txtLen) {
        size_t length{size_t(txtRecord[pos++])};
        string data{reinterpret_cast<const char*>(txtRecord) + pos, min(length, txtLen - pos)};
        auto key_pos = data.find('=');
        fields[data.substr(0, key_pos)] = (key_pos == std::string::npos) ? "" : data.substr(key_pos + 1);
        pos += length;
    }
}

//! A string value in the TXT record is a sequence of bytes
void Zeroconf::TxtRecord::assign(const std::string& key, const std::string& value) {
    fields[key] = value.substr(0, 20);  // silently truncate name to 20 characters
}

//! A simple integer value in the TXT record is the string of its decimal value
void Zeroconf::TxtRecord::assign(const std::string& key, const int value) {
    ostringstream field;
    field << value;
    assign(key, field.str());
}

//! A vector of integers in the TXT record is a sequence of big-endian 16-bit values
//! note that the size of the vector is implicit in the record length:
//! (record.length()-record.find("=")-1)/2.
void Zeroconf::TxtRecord::assign(const std::string& key, const std::vector<unsigned int>& values) {
    ostringstream field;
    for(const auto& value : values)
        field.put(value << 8), field.put(value % 0xff);
    assign(key, field.str());
}

//! A boolean value in the TXT record is either the key (true) or nothing (false)
void Zeroconf::TxtRecord::assign(const std::string& key, const bool value) {
    if(value)
        fields[key] = "";  // length will be 0, so no "=" in the record
}

//! Retrieve the encoded TXT record.
//! A serialized TXT record is composed of length-prefixed strings of the form KEY[=VALUE]
string Zeroconf::TxtRecord::record() const {
    ostringstream txt;
    serializeField(txt, "txtvers");
    serializeField(txt, "protovers");
    serializeField(txt, "type");
    serializeField(txt, "busy");
    serializeField(txt, "ids");
    serializeField(txt, "pids");
    return txt.str();
}

//!< serialize a field into the encoded TXT record
void Zeroconf::TxtRecord::serializeField(ostringstream& txt, const std::string& key) const {
    if(fields.find(key) != fields.end())  // key will be absent if it was boolean and false
    {
        string record{key};
        if(fields.at(key).length() > 0)
            record += "=" + fields.at(key);
        txt.put(record.length());
        txt << record;
    }
}

}  // namespace Aseba
