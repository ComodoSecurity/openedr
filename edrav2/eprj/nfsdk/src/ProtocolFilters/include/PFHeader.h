//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _PFHEADER
#define _PFHEADER

#include <string>
#include <vector>
#include <map>
#include <list>
  
namespace ProtocolFilters
{

	inline bool safe_iswhitespace(int c) { return (c == (int) ' ' || c == (int) '\t' || c == (int) '\r' || c == (int) '\n'); }

	inline std::string strToLower(const std::string& str)
	{
		std::string result;
		result.reserve(str.size()); // optimisation

		for (std::string::const_iterator it = str.begin(); it != str.end(); it++)
		{
			result.append(1, (char) ::tolower(*it));
		}

		return result;
	}

	/**
	 *	Header item
	 */
	class PFHeaderField
	{
	public:
		inline PFHeaderField(const std::string& name, const std::string& value):
			m_name(name), m_value(value)
		{
		}

		inline PFHeaderField(const PFHeaderField& copyFrom):
			m_name(copyFrom.m_name), m_value(copyFrom.m_value)
		{
		}

		inline PFHeaderField()
		{
		}

		inline PFHeaderField& operator = (const PFHeaderField &assignFrom)
		{
			m_name = assignFrom.m_name;
			m_value = assignFrom.m_value;	
			return *this;
		}

		/**
		 *	Field name
		 */
		inline const std::string& name() const 
		{ 
			return m_name; 
		}

		/**
		 *	Field value
		 */
		inline std::string& value() 
		{ 
			return m_value; 
		}

		inline const std::string& value() const 
		{ 
			return m_value; 
		}

		/**
		 *	Converts the field to plain string, and adds CRLF at the end
		 */
		inline std::string toString() const
		{
			return m_name + ": " + m_value + "\r\n";
		}

		/**
		 *	Returns the field as unfolded plain string
		 */
		inline std::string unfoldedValue() const
		{
			return unfoldValue(m_value);
		}

		/**
		 *	Unfolds multi-line fields
		 */
		inline std::string unfoldValue(const std::string& value) const
		{
			std::string result;
			result.reserve(value.length());

			std::string::const_iterator it = value.begin();
			std::string::const_iterator end = value.end();

			// skip all leading whitespace, including leading \r and \n characters
			while (it < end && safe_iswhitespace(*it)) 
				it++;

			while (it < end)
			{
				if (*it == '\r' && (it + 1) != end && *(it + 1) == '\n')
				{
					// skip the \r\n
					it += 2;

					// *it will now be either a space or a tab, if this was a valid header field value
				}
				else
				{
					// normal character
					result += *(it++);
				}
			}

			// strip whitespace at the end of the value - there's an implicit newline at the end of the value
			while (!result.empty() && safe_iswhitespace(result[result.length() - 1]))
			{
				result.resize(result.length() - 1);
			}

			return result;
		}


	private:
		std::string m_name;
		std::string m_value;
	};


	/**
	 *	Header class provides the necessary methods for working with
	 *	CRLF delimeted lists of "name: value" fields.
	 */
	class PFHeader
	{
	public:
		PFHeader() 
		{
		}

		virtual ~PFHeader() 
		{
		}

		PFHeader(const PFHeader& copyFrom)
		{
			*this = copyFrom;
		}

		PFHeader & operator = (const PFHeader& copyFrom)
		{
			m_fields = copyFrom.m_fields;
			m_fieldsByName.clear();
			for (tPFHeaderFields::iterator it = m_fields.begin(); it != m_fields.end(); ++it)
			{
				m_fieldsByName.insert(make_pair(strToLower(it->name()), it));
			}
			return *this;
		}

		/**
		 *	Parse the next line
		 */
		bool inputLine(const std::string& line)
		{
			if (line.empty()) 
				return true;

			if (!m_fields.empty() && (line[0] == ' ' || line[0] == '\t'))
			{
				PFHeaderField& PFHeaderField = *--m_fields.end();
				PFHeaderField.value() += "\r\n";
				PFHeaderField.value() += line;
				return true;
			}

			std::string::size_type colonPos = line.find(':');

			if (colonPos == 0 || colonPos == std::string::npos)
			{
				return true;
			}

			std::string::size_type nameEnd = colonPos;
			std::string::size_type valueStart = colonPos + 1;

			while (valueStart < line.length() && 
				(line[valueStart] == ' ' || line[valueStart] == '\t')) 
				valueStart++;

			addField(line.substr(0, nameEnd), line.substr(valueStart));
		
			return true;
		}

		/**
		 *	Parse the given data buffer as string
		 */ 
		bool parseString(const char * buf, int len)
		{
			clear();

			std::string line;
			const char * p, * ep;

			for (size_t offset = 0; offset < (size_t)len;)
			{
				p = strchr(buf + offset, '\n');

				if (p == NULL)
				{
					p = buf + len;	
				}
				
				if (buf + offset < p)
				{
					ep = (*(p-1) != '\r')? p : p - 1;		
					line = std::string(buf + offset, ep);
				} else
				{
					line.erase();
				}

				offset = p - buf + 1;
				
				if (line.empty() || !inputLine(line))
					break;
			}

			return true;
		}

		/**
		 *	Convert the header to plain string
		 */
		std::string toString() const
		{
			std::string result;
			result.reserve(m_fields.size() * 60);

			for (tPFHeaderFields::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it)
			{
				result += it->toString();
			}

			return result;
		}
	
		/**
		 *	Add new field
		 */
		void addField(const std::string& fieldName, const std::string& fieldValue, bool start = false)
		{
			tPFHeaderFields::iterator it = m_fields.insert(start ? m_fields.begin() : m_fields.end(), 
				PFHeaderField(fieldName, fieldValue));
			m_fieldsByName.insert(make_pair(strToLower(it->name()), it));
		}
		
		/**
		 *	Remove the fields with given name
		 */
		unsigned int removeFields(const std::string& fieldName, bool exact)
		{
			std::string name = strToLower(fieldName);

			if (!exact)
			{
				tPFHeaderFieldsByName::iterator begin = m_fieldsByName.lower_bound(name);

				if (begin != m_fieldsByName.end())
				{
					tPFHeaderFieldsByName::iterator end = m_fieldsByName.upper_bound(name);

					for (tPFHeaderFieldsByName::iterator it = begin; it != end; ++it)
					{
						m_fields.erase(it->second);
					}

					m_fieldsByName.erase(begin, end);

					return 1;
				}

				return 0;
			}
			else
			{
				tPFHeaderFieldsByName::iterator it;
				unsigned int count = 0;

				while ((it = m_fieldsByName.lower_bound(name)) != m_fieldsByName.end() && 
					it->first.substr(0, name.length()) == name)
				{
					m_fields.erase(it->second);
					m_fieldsByName.erase(it);
					count++;
				}

				return count;
			}
		}
		
		/**
		 *	Returns a vector of pointers to field objects
		 */
		std::vector<PFHeaderField*> getFields()
		{
			std::vector<PFHeaderField*> results;
			results.reserve(m_fields.size());

			for (tPFHeaderFields::iterator it = m_fields.begin(); it != m_fields.end(); ++it)
			{
				results.push_back(&*it);
			}

			return results;
		}
		
		/**
		 *	Returns a vector of const pointers to PFHeaderField objects
		 */
		std::vector<const PFHeaderField*> getFields() const
		{
			std::vector<const PFHeaderField*> results;
			results.reserve(m_fields.size());

			for (tPFHeaderFields::const_iterator it = m_fields.begin(); 
				it != m_fields.end(); ++it)
			{
				results.push_back(&*it);
			}

			return results;
		}
		
		/**
		 *	Find a first field object with given name
		 */
		PFHeaderField* findFirstField(const std::string& fieldName)
		{
			std::string name = strToLower(fieldName);
			
			tPFHeaderFieldsByName::iterator it = m_fieldsByName.find(name);
			if (it == m_fieldsByName.end()) 
				return NULL;

			return &(*it->second);
		}

		const PFHeaderField* findFirstField(const std::string& fieldName) const
		{
			std::string name = strToLower(fieldName);

			tPFHeaderFieldsByName::const_iterator it = m_fieldsByName.find(name);
			if (it == m_fieldsByName.end()) 
				return NULL;

			return &(*it->second);
		}
		
		/**
		 *	Returns all field objects with given name
		 */
		std::vector<PFHeaderField*> findFields(const std::string& fieldName, bool exact)
		{
			std::string name = strToLower(fieldName);

			std::vector<PFHeaderField*> result;

			if (exact)
			{
				tPFHeaderFieldsByName::iterator end = m_fieldsByName.upper_bound(name);
				for (tPFHeaderFieldsByName::iterator it = m_fieldsByName.lower_bound(name); it != end; ++it)
				{
					result.push_back(&(*it->second));
				}
			}
			else
			{
				// we can't use upper_bound as that would return the first element that has a suffix after this prefix
				for (tPFHeaderFieldsByName::iterator it = m_fieldsByName.lower_bound(name); 
					it != m_fieldsByName.end() && it->first.substr(0, name.length()) == name; ++it)
				{
					result.push_back(&(*it->second));
				}
			}

			return result;
		}
		
		std::vector<const PFHeaderField*> findFields(const std::string& fieldName, bool exact) const
		{
			std::string name = strToLower(fieldName);

			std::vector<const PFHeaderField*> result;

			if (exact)
			{
				tPFHeaderFieldsByName::const_iterator end = m_fieldsByName.upper_bound(name);
				for (tPFHeaderFieldsByName::const_iterator it = m_fieldsByName.lower_bound(name); 
					it != end; ++it)
				{
					result.push_back(&(*it->second));
				}
			}
			else
			{
				for (tPFHeaderFieldsByName::const_iterator it = m_fieldsByName.lower_bound(name); 
					it != m_fieldsByName.end() && it->first.substr(0, name.length()) == name; ++it)
				{
					result.push_back(&(*it->second));
				}
			}

			return result;
		}
		
		/**
		 *	Returns a number of fields with given name
		 */
		inline unsigned int countFields(const std::string& fieldName) const
		{
			return (unsigned int)(m_fieldsByName.count(strToLower(fieldName)));
		}

		inline unsigned int size() const
		{
			return (unsigned int)m_fields.size();
		}
	
		inline const PFHeaderField* getField(int index)
		{
			if (index < 0 || index >= (int)m_fields.size())
				return NULL;
			
			int n = 0;

			for (tPFHeaderFields::const_iterator it = m_fields.begin(); 
				it != m_fields.end(); ++it)
			{
				if (n == index)
				{
					return &(*it);
				}
				n++;
			}

			return NULL;
		}

		/**
		 *	Removes all fields
		 */
		inline void clear()
		{
			m_fieldsByName.clear();
			m_fields.clear();
		}

		/**
		 *	Returns true if the header contains no fields
		 */
		inline bool isEmpty()
		{
			return m_fields.empty();
		}

	private:
		typedef std::list<PFHeaderField> tPFHeaderFields;
		tPFHeaderFields m_fields;

		typedef std::multimap<std::string, tPFHeaderFields::iterator> tPFHeaderFieldsByName;
		tPFHeaderFieldsByName m_fieldsByName;
	};

}

#endif 

