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
		std::vector<CsvBuffer> m_data;
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
		T* get_data(bool transposed, std::vector<std::string>& column_header, std::vector<std::string>& row_header, const std::vector<std::string> &parent_labels = {}, const std::vector<std::string> &dimension_labels={});
		
	};

	template <typename T>
	T* CSVReader::get_data(bool transposed, std::vector<std::string> &column_header, std::vector<std::string> &row_header, const std::vector<std::string> &parent_labels, const std::vector<std::string> &dimension_labels)
	{
		assert(m_nrOfRows);
		assert(m_nrOfColumns);
		
		const std::size_t nrOfBufferItems = m_with_row_header ? m_nrOfColumns + 1 : m_nrOfColumns;


		std::vector<std::ptrdiff_t> target_row_index(m_nrOfRows);
		std::iota(target_row_index.begin(), target_row_index.end(), std::ptrdiff_t(0));
		std::vector<std::ptrdiff_t> target_column_index(m_nrOfColumns);
		std::iota(target_column_index.begin(), target_column_index.end(), std::ptrdiff_t(0));
		

		if(parent_labels.empty())
		{
			column_header = m_column_header;
			row_header = m_row_header;
		}
		else
		{
			if (transposed && m_with_column_header)
			{
				target_column_index.assign(m_nrOfColumns, -1);
				#pragma  omp parallel for schedule(dynamic,1)
				for (std::ptrdiff_t i = 0; i < m_nrOfColumns; ++i)
				{
					auto selected = std::find(parent_labels.cbegin(), parent_labels.cend(), m_column_header[i]);
					if (selected != parent_labels.cend())
					{
						target_column_index[i] = (selected - parent_labels.cbegin());
					}
				}

				column_header = parent_labels;
				row_header = m_row_header;
			}
			else if (!transposed && m_with_row_header)
			{
				target_row_index.assign(m_nrOfRows, -1);
				// process all buffer items in parallel
				#pragma  omp parallel for schedule(dynamic,1)
				for (std::ptrdiff_t i = 0; i < m_nrOfRows; ++i)
				{
					auto selected = std::find(parent_labels.cbegin(), parent_labels.cend(), m_row_header[i]);
					if (selected != parent_labels.cend())
					{
						target_row_index[i] = selected - parent_labels.cbegin();
					}
				}
				
				row_header = parent_labels;
				column_header = m_column_header;
			}
			
		}

		if(!dimension_labels.empty())
		{
			if (!transposed && m_with_column_header)
			{
				target_column_index.assign(m_nrOfColumns, -1);
				// process all buffer items in parallel
				#pragma  omp parallel for schedule(dynamic,1)
				for (std::ptrdiff_t i = 0; i < m_nrOfColumns; ++i)
				{
					auto selected = std::find(dimension_labels.cbegin(), dimension_labels.cend(), m_column_header[i]);
					if (selected != dimension_labels.cend())
					{
						target_column_index[i] = selected - dimension_labels.cbegin();
					}
				}
				
				column_header = dimension_labels;
			}
		}


		const std::size_t nrOfTargetColumns = column_header.size();
		const std::size_t nrOfTargetRows = row_header.size();
		// wait with data allocation until we are sure about the resulting number of columns and rows
		if (nrOfTargetColumns == 0)
			return nullptr;
		if (nrOfTargetRows == 0)
			return nullptr;
		T *data = new T[nrOfTargetColumns*nrOfTargetRows];
		 
		const std::ptrdiff_t column_offset = m_with_row_header ? 1 : 0;
		#pragma  omp parallel for schedule(dynamic,1)
		for (std::ptrdiff_t i = 0; i < (std::ptrdiff_t)m_nrOfRows; ++i)
		{
			std::ptrdiff_t row_index = target_row_index[i];
			if(row_index >=0)
			{
				ExtCsvLoader::CsvBuffer& csvbuffer = m_data[i];
				if (!csvbuffer.processed())
					csvbuffer.process(m_separator, nrOfBufferItems);

				std::vector<T> transposebuffer;
				T* row_ptr = data + (row_index * nrOfTargetColumns);
				if (transposed)
				{
					transposebuffer.resize(nrOfTargetColumns);
					row_ptr = &(transposebuffer[0]);
				}

				for (std::size_t j = 0; j < m_nrOfColumns; ++j)
				{
					auto column_index = target_column_index[j];
					if (column_index >= 0)
						csvbuffer.getAs(j + column_offset, row_ptr[column_index]);
				}

				if (transposed)
				{
					for (std::size_t j = 0; j < nrOfTargetColumns; ++j)
					{
						data[(j * nrOfTargetRows) + row_index] = row_ptr[j];
					}
				}
			}
		}

		if (transposed)
			std::swap(column_header, row_header);
		return data;
	};


}

