#include "csvreader.h"

#include <cstdint>
#include <unordered_map>

namespace ExtCsvLoader
{
	void initialize_header(std::vector<std::string>& header, const std::string& prefix)
	{
		std::size_t size = header.size();
		for (auto i = 0; i < size; ++i)
		{
			header[i] = prefix +  std::to_string(i);
		}
	}

	std::string searchandreplace(std::string _input, const char _search, const char _replace)
	{
		std::string::size_type pos = 0u;
		while ((pos = _input.find(_search, pos)) != std::string::npos)
		{
			_input[pos] = _replace;
		}
		return _input;
	}


	void create_target_index_vector(const std::vector<std::string>& labels, const std::vector<std::string>& selected_labels, std::vector<std::ptrdiff_t>& result)
	{
		std::vector<std::unordered_map<std::string, std::ptrdiff_t>> temp(omp_get_max_threads());
		std::unordered_map<std::string, std::ptrdiff_t>& dimension_labels_map = temp[0];
#pragma omp parallel for
		for (std::ptrdiff_t i = 0; i < selected_labels.size(); ++i)
		{
			auto tid = omp_get_thread_num();
			temp[tid][selected_labels[i]] = i;
		}
		for (std::size_t i = 1; i < temp.size(); ++i)
		{
			dimension_labels_map.merge(temp[i]);
		}
		result.assign(labels.size(), -1);
		// process all buffer items in parallel
#pragma  omp parallel for schedule(dynamic,1)
		for (std::ptrdiff_t i = 0; i < labels.size(); ++i)
		{
			auto found = dimension_labels_map.find(labels[i]);
			if (found != dimension_labels_map.cend())
			{
				result[i] = found->second;
			}
		}
	}

	CSVReader::CSVReader(const QString& _filename, const char _separator, bool with_column_header, bool with_row_header)
	{
		m_filename = _filename;
		m_separator = _separator;
		m_with_column_header = with_column_header;
		m_with_row_header = with_row_header;
		m_nrOfColumns = 0;
		m_nrOfRows = 0;
	};

	std::string CSVReader::GetColumnRowHeader() const
	{
		return m_column_row_header;
	}
	const std::vector<std::string>& CSVReader::GetColumnHeader() const
	{
		return m_column_header;
	}
	const std::vector<std::string>& CSVReader::GetRowHeader() const
	{
		return m_row_header;
	}

	std::size_t CSVReader::columns() const
	{
		return m_nrOfColumns;
	}

	std::size_t CSVReader::rows() const
	{
		return m_nrOfRows;
	};

	void CSVReader::read()
	{
		m_data.clear();
		QFile qFile(m_filename);
		if (!qFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			//qDebug() << "problem reading file " << m_filename;
			return;
		}
		QTextStream FileStream(&qFile);
		

		// read the first line and determine the number of attributes
		 std::string firstLine = FileStream.readLine().toStdString();

		 CsvBuffer header(std::move(firstLine));
		header.process(m_separator);
		 
		
		const std::size_t nrOfBufferItems = header.size();

		if (nrOfBufferItems == 0)
			return;

		if (m_with_row_header)
		{
			if (m_with_column_header)
			{
				header.getAs(0, m_column_row_header);
			}
		}

		m_nrOfColumns = m_with_row_header ? nrOfBufferItems - 1 : nrOfBufferItems;

		//qDebug() << m_nrOfColumns << " columns\n";

		m_column_header.resize(m_nrOfColumns);

		if (!m_with_column_header)
		{
			m_column_header.resize(m_nrOfColumns);
			ExtCsvLoader::initialize_header(m_column_header, "VAR");
		}
		else
		{

			if (m_with_row_header)
			{
#pragma omp parallel for
				for (std::ptrdiff_t column_index = 0; column_index < m_nrOfColumns; ++column_index)
				{
					header.getAs(column_index + 1, m_column_header[column_index]);
				}
			}
			else
			{
#pragma omp parallel for
				for (std::ptrdiff_t column_index = 0; column_index < m_nrOfColumns; ++column_index)
				{
					header.getAs(column_index, m_column_header[column_index]);
				}
			}

			// remove any SPACE's or TAB's at the beginning and end of each header field
			for (auto attribute = 0; attribute < m_nrOfColumns; attribute++)
			{
				// remove any SPACE's or TAB's at the beginning of field
				if (m_column_header[attribute].size())
				{
					while (*(m_column_header[attribute].begin()) == SPACE || *(m_column_header[attribute].begin()) == TAB || *(m_column_header[attribute].begin()) == QUOTE)
					{
						//qDebug() << "space, tab or quote removed";
						m_column_header[attribute].erase(m_column_header[attribute].begin());
					}
				}

				if (m_column_header[attribute].size())
				{
					// and do the same for the end of header[attribute]
					while (*(m_column_header[attribute].rbegin()) == SPACE || *(m_column_header[attribute].rbegin()) == TAB || *(m_column_header[attribute].begin()) == QUOTE)
					{
						//qDebug() << "space, tab or quote removed";
						m_column_header[attribute].resize(m_column_header[attribute].size() - 1, SPACE);
					}
				}

			}
		}
		
		if (!m_with_column_header)
			m_data.push_back(header);
		

		while (!FileStream.atEnd())
		{
			std::string line = FileStream.readLine().toStdString();
			//getline(file, line);
			if (!line.empty())
			{
				m_data.push_back(CsvBuffer(std::move(line)));
			}
		}
		m_nrOfRows = m_data.size();
		m_row_header.resize(m_nrOfRows);
		//qDebug() << QString("data loaded");
		if (m_with_row_header)
		{
			if (m_with_column_header)
			{
				// fix situation where there is a row and column header but no string for the column_row_header_item;
				ExtCsvLoader::CsvBuffer& csvbuffer = m_data[0];
				if (!csvbuffer.processed())
					csvbuffer.process(m_separator, m_nrOfColumns + 1);

				if (csvbuffer.size() == (m_nrOfColumns + 2))
				{
					// header was lacking a row+column header item
					m_column_header.insert(m_column_header.begin(), m_column_row_header);
					m_nrOfColumns += 1;
					m_column_row_header = "";
				}
			}
			const std::size_t nrOfBufferItems = m_with_row_header ? m_nrOfColumns + 1 : m_nrOfColumns;
			#pragma  omp parallel for schedule(dynamic,1)
			for (std::ptrdiff_t i = 0; i < m_nrOfRows; ++i)
			{
				ExtCsvLoader::CsvBuffer& csvbuffer = m_data[i];
				if (!csvbuffer.processed())
					csvbuffer.process(m_separator, nrOfBufferItems);
				csvbuffer.getAs(0, m_row_header[i]);
			}
			//qDebug() << QString("data processed");
		}
		else
		{
			ExtCsvLoader::initialize_header(m_row_header, "");
		}
		qDebug() << m_nrOfColumns << " x " << m_nrOfRows << " loaded and processed";
	}
}







