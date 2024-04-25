#include "csvbuffer.h"



namespace ExtCsvLoader
{
	const char QuoteChar = '\"';
	const char SpaceChar = ' ';
	const char TabChar = '\t';

	bool CsvBuffer::skip_character(const char c) const
	{
		if (c != m_separator)
		{
			if (c == SpaceChar || (c == TabChar))
				return true;
		}
		return false;
	}

	CsvBuffer::CsvBuffer()
		:m_empty('\0')
		,m_separator('\0')
	{
	}
	
	CsvBuffer::CsvBuffer(const char separator)
		:m_empty('\0')
		,m_separator(separator)
	{
	}
	
	CsvBuffer::CsvBuffer(std::string&& input)
		:m_buffer(std::move(input))
		,m_empty('\0')
		,m_separator('\0')
	{
	}
	
	CsvBuffer::CsvBuffer(std::string&& input, const char separator)
		:m_buffer(std::move(input))
		,m_empty('\0')
		,m_separator(separator)
	{
	}
	
	std::string& CsvBuffer::buffer()
	{
		m_item.clear();
		return m_buffer;
	}

	
	bool CsvBuffer::processed() const
	{
		return !m_item.empty();
	}

	void CsvBuffer::process(const char& separator, std::size_t expectedNrOfItems)
	{
		m_separator = separator;
		m_item.clear();
		if (expectedNrOfItems)
			m_item.reserve(expectedNrOfItems);
		std::size_t start_pos = 0;
		std::size_t end_pos = 0;
		const std::size_t positions = m_buffer.size();
		bool insideQuote = false;

		for (std::size_t pos = 0; pos < positions; ++pos)
		{
			const char c = m_buffer[pos];

			if (skip_character(c))
			{
				// remove skip characters at the start
				if (start_pos == pos)
				{
					++start_pos;
				}
			}
			else
			{
				end_pos = pos;
				if (c == QuoteChar)
				{
					insideQuote = !insideQuote;
					m_buffer[pos] = '\0';
					if (start_pos == pos)
					{
						++start_pos;
					}
				}
				else if (!insideQuote && c == separator)
				{
					if (start_pos < end_pos)
					{
						m_buffer[pos] = '\0';
						if (pos > 0)
						{
							// remove skip characters at the end
							std::size_t p = pos - 1;
							while (skip_character(m_buffer[p]))
							{
								m_buffer[p] = '\0';
								if (p > 0)
									--p;
							}
						}
						m_item.push_back(m_buffer.data() + start_pos);
					}
					else
					{
						m_item.push_back(&m_empty);
					}
					start_pos = pos + 1;
				}
			}

		}
		m_item.push_back(m_buffer.data() + start_pos);
	}

	const char* CsvBuffer::operator[](const std::size_t _index) const
	{
		return m_item[_index];
	}

	std::ptrdiff_t CsvBuffer::size() const
	{
		return m_item.size();
	}

	void CsvBuffer::getAs(const std::size_t _index, int& v) const
	{
		if(m_item[_index])
			v = atoi(m_item[_index]);
	}

	void CsvBuffer::getAs(const std::size_t _index, float& v) const
	{
		if (m_item[_index])
		{
			const double d = atof(m_item[_index]);
			if (d > std::numeric_limits<float>::max())
				v = std::numeric_limits<float>::max();
			else if (d < std::numeric_limits<float>::lowest())
				v = std::numeric_limits<float>::lowest();
			else
				v = float(d);
		}
			
	}

	void CsvBuffer::getAs(const std::size_t _index, biovault::bfloat16_t & v) const
	{
		float f;
		getAs(_index, f);
		v = f;
	}
	


	void CsvBuffer::getAs(const std::size_t _index, double& v) const
	{
		if (m_item[_index])
			v = atof(m_item[_index]);
	}
	void CsvBuffer::getAs(const std::size_t _index, std::string& v) const
	{
		if (m_item[_index])
		{
			
			const char* ptr = m_item[_index];
			auto size = strlen(ptr);
			v = m_item[_index];
			//v = std::string(ptr, ptr+size);
			
		}
			
	}

	

}
