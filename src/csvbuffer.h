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
		const char m_seperator;

		bool skip_character(const char c) const;
		
	public:
		CsvBuffer() = default;
		~CsvBuffer() = default;
		
		CsvBuffer(const std::string& _source, const char& separator, std::size_t expectedNrOfItems = 0);
		
		std::string& buffer();
		void process(const char& separator, std::size_t expectedNrOfItems = 0);
		const char* operator[](const std::size_t _index) const;
		std::ptrdiff_t size() const;

		void getAs(const std::size_t index, int& v);
		void getAs(const std::size_t index, float& v);
		void getAs(const std::size_t index, biovault::bfloat16_t& v);
		void getAs(const std::size_t index, double& v);
		void getAs(const std::size_t index, std::string& v);
		
		template<typename T>
		void getAs(std::vector<T>& result);
	};

	template <typename T>
	void CsvBuffer::getAs(std::vector<T>& result)
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