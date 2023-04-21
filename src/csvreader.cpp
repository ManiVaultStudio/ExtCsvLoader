#include "csvreader.h"


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

	std::size_t CSVReader::columns()
	{
		return m_nrOfColumns;
	}

	std::size_t CSVReader::rows()
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
		std::string line;

		// read the first line and determine the number of attributes
		line = FileStream.readLine().toStdString();

		ExtCsvLoader::CsvBuffer buffer(line, m_separator);
		const std::size_t nrOfBufferItems = buffer.size();

		if (nrOfBufferItems == 0)
			return;

		if (m_with_row_header)
		{
			if (m_with_column_header)
			{
				buffer.getAs(0, m_column_row_header);
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
				for (int column_index = 0; column_index < m_nrOfColumns; ++column_index)
				{
					buffer.getAs(column_index + 1, m_column_header[column_index]);
				}
			}
			else
			{
#pragma omp parallel for
				for (int column_index = 0; column_index < m_nrOfColumns; ++column_index)
				{
					buffer.getAs(column_index, m_column_header[column_index]);
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
			m_data.push_back(line);
		m_nrOfRows = m_data.size();

		while (!FileStream.atEnd())
		{
			line = FileStream.readLine().toStdString();
			//getline(file, line);
			if (!line.empty())
			{
				m_data.push_back(line);
				++m_nrOfRows;
			}
		}
	
		m_row_header.resize(m_nrOfRows);
		if (!m_with_row_header)
			ExtCsvLoader::initialize_header(m_row_header, "");

	}
}






