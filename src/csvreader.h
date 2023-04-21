#pragma once
#include "csvbuffer.h"
#include <QFile>
#include "QTextStream"
#include <omp.h>

#ifdef _DEBUG
#define OUTPUT_PROGRESS
#endif


#define SPACE ' '
#define TAB '\t'
#define REPLACEMENT_SEPARATOR '_'
#define QUOTE '\"'

namespace ExtCsvLoader
{

	template <typename T>
	void ompsplit(const std::string& input, const char separator, std::vector<T>& output)
	{
		ExtCsvLoader::CsvBuffer sb(input, separator);
		long long size = sb.size();
		output.resize(sb.size());
		#pragma omp parallel for
		for (long long i = 0; i < size; ++i)
		{
			sb.getAs(i, output[i]);
		}
	}

	void initialize_header(std::vector<std::string>& header, const std::string& prefix);
	std::string searchandreplace(std::string _input, const char _search, const char _replace);

	class CSVReader
	{
		// everything public for optimal flexibilty
	public:
		struct TRANSFORM
		{
			enum { NONE, LOG, SQRT, ARCSIN5 };
		};
		
	private:
		std::vector<std::string> m_data;
		std::string m_column_row_header;
		std::vector<std::string> m_column_header;
		std::vector<std::string> m_row_header;

		std::size_t m_nrOfColumns;
		std::size_t m_nrOfRows;

		QString m_filename;
		char m_separator;
		bool m_with_column_header;
		bool m_with_row_header;
		
		

		CSVReader() = delete;
	public:
		explicit CSVReader(const QString& filename, const char separator = ',', bool with_column_header = true, bool with_row_header = true);
		~CSVReader() = default;

		std::string GetColumnRowHeader() const;
		const std::vector<std::string>& GetColumnHeader() const;
		const std::vector<std::string>& GetRowHeader() const;
		std::size_t rows();
		std::size_t columns();

		void read();
		template<typename T>
		T* get_data(bool transposed);
		
	};

	template <typename T>
	T* CSVReader::get_data(bool transposed)
	{
		T* data = new T[m_nrOfRows * m_nrOfColumns];
		std::ptrdiff_t nrOfLines = static_cast<std::ptrdiff_t>(m_nrOfRows);


		std::size_t nrOfBufferItems = m_with_row_header ? m_nrOfColumns - 1 : m_nrOfColumns;
		#pragma  omp parallel for schedule(dynamic,1)
		for (long long i = 0; i < nrOfLines; ++i)
		{
			std::vector<T> transposebuffer(m_nrOfColumns);
			T* row = data + (i * m_nrOfColumns);
			if (transposed)
			{
				row = &(transposebuffer[0]);
			}

			ExtCsvLoader::CsvBuffer csvbuffer(m_data[i], m_separator, nrOfBufferItems);
			if (!m_with_row_header)
			{
				for (std::size_t j = 0; j < m_nrOfColumns; ++j)
				{
					csvbuffer.getAs(j, row[j]);
				}
			}
			else
			{
				csvbuffer.getAs(0, m_row_header[i]);
				for (std::size_t j = 0; j < m_nrOfColumns; ++j)
				{
					csvbuffer.getAs(j + 1, row[j]);
				}
			}
			if (transposed)
			{
				for (std::size_t j = 0; j < m_nrOfColumns; ++j)
				{
					data[j * m_nrOfRows + i] = row[j];
				}
			}
			
		}

		

		return data;
	};


}

