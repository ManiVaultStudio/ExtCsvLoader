#pragma once
#include <string>
#include <vector>
#include <sstream>

#include <biovault_bfloat16/biovault_bfloat16.h>

namespace ExtCsvLoader
{
	class CsvBuffer
	{
		std::string m_buffer;
		const char m_empty;
		std::vector<const char*> m_item;
		char m_separator;

		bool skip_character(const char c) const;
		
	public:
		CsvBuffer();
		CsvBuffer(const char separator);
		CsvBuffer(std::string&& input);
		CsvBuffer(std::string&& input, const char separator);
		CsvBuffer(const std::string& input);
		CsvBuffer(const std::string& input, const char separator);
		~CsvBuffer() = default;
		
		std::string& buffer();
		
		bool processed() const;
		void process(const char& separator, std::size_t expectedNrOfItems = 0);
		const char* operator[](const std::size_t _index) const;
		std::ptrdiff_t size() const;

		void getAs(const std::size_t index, int& v) const;
		void getAs(const std::size_t index, float& v) const;
		void getAs(const std::size_t index, biovault::bfloat16_t& v) const;
		void getAs(const std::size_t index, double& v) const;
		void getAs(const std::size_t index, std::string& v) const;
		
		template<typename T>
		void getAs(std::vector<T>& result) const;
	};

	template <typename T>
	void CsvBuffer::getAs(std::vector<T>& result) const
	{
		std::ptrdiff_t items = size();
		result.resize(items);
		#pragma omp parallel for
		for (long long i = 0; i < items; ++i)
		{
			getAs(i, result[i]);
		}
	}

}