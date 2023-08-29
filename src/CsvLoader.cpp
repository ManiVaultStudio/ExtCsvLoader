#include "CsvLoader.h"

#include <PointData/PointData.h>
#include <ClusterData/ClusterData.h>
#include <Dataset.h>

#include <util/Icon.h>
#include <QtCore>
#include "csvreader.h"

Q_PLUGIN_METADATA(IID "nl.tudelft.ExtCsvLoader")

// =============================================================================
// Loader
// =============================================================================

using namespace hdps;
using namespace hdps::gui;

namespace 
{

    std::vector<QString> toQStringVector(const std::vector<std::string> &v)
    {
        std::vector<QString> result(v.size());
        for (std::size_t i = 0; i < v.size(); ++i)
            result[i] = v[i].c_str();
        return result;
    }
    // Call the specified function on the specified value, if it is valid.
    template <typename TFunction>
    void IfValid(const QVariant& value, const TFunction& function)
    {
        if (value.isValid())
        {
            function(value);
        }
    }

  
    Dataset<Points> createPointsDataset(hdps::CoreInterface* core, bool ask, QString suggestion)
    {
        QString dataSetName = suggestion;

        
        hdps::Dataset<Points> newDataset = core->addDataset("Points", dataSetName);
        events().notifyDatasetAdded(newDataset);

        return newDataset;
    }

    void CreateColorVector(std::size_t nrOfColors, std::vector<QColor>& colors)
    {

        if (nrOfColors)
        {
            colors.resize(nrOfColors);
            std::size_t index = 0;
            for (std::size_t i = 0; i < nrOfColors; ++i)
            {
                const float h = std::min<float>((1.0 * i / (nrOfColors + 1)), 1.0f);
                if (h > 1 || h < 0)
                {
                    int bp = 0;
                    bp++;
                }
                colors[i] = QColor::fromHsvF(h, 0.5f, 1.0f);
            }
        }
        else
            colors.clear();
    }

    bool is_number(const std::string& s)
    {
        if (s.empty())
            return true;
        char* end = nullptr;
        strtod(s.c_str(), &end);
        return *end == '\0';
    }

}

CsvLoader::~CsvLoader(void)
{
    
}

// Alphabetic list of keys used to access settings from QSettings.
namespace Keys
{
    const QLatin1String columnHeaderValueKey("columnHeader");
    const QLatin1String fileNameKey("fileName");
    const QLatin1String hierarchyValueKey("hierarchy");
    const QLatin1String rowHeaderValueKey("rowHeader");
    const QLatin1String selectedNameFilterKey("selectedNameFilter");
    const QLatin1String separatorValueKey("separatorValue");
    const QLatin1String sourceValueKey("sourceValue");
    const QLatin1String storageValueKey("storageValue");
    const QLatin1String transposeValueKey("transposeValue");
}

void CsvLoader::init()
{
    QStringList fileTypeOptions;
    fileTypeOptions.append("CSV (*.csv *.txt)");
    fileTypeOptions.append("TSV (*.tsv)");
    _fileDialog.setOption(QFileDialog::DontUseNativeDialog);
    _fileDialog.setFileMode(QFileDialog::ExistingFile);
    _fileDialog.setOption(QFileDialog::DontUseNativeDialog, true);
    _fileDialog.setOption(QFileDialog::DontResolveSymlinks, true);
    _fileDialog.setOption(QFileDialog::DontUseCustomDirectoryIcons, true);
    _fileDialog.setNameFilters(fileTypeOptions);

    QSettings settings(QString::fromLatin1("HDPS"), QString::fromLatin1("Plugins/ExtCsvLoader"));
    QGridLayout* fileDialogLayout = dynamic_cast<QGridLayout*>(_fileDialog.layout());

    int rowCount = fileDialogLayout->rowCount();

    QLabel* separatorLabel = new QLabel("separator");
    _separatorLineEdit = new QLineEdit;
    _separatorLineEdit->setMaximumWidth(13);
    _separatorLineEdit->setMaxLength(1);
    {
        const auto value = settings.value(Keys::separatorValueKey);
        if (value.isValid())
            _separatorLineEdit->setText(value.toChar());
        else
            _separatorLineEdit->setText(",");
    }
    fileDialogLayout->addWidget(separatorLabel, rowCount, 0);
    fileDialogLayout->addWidget(_separatorLineEdit, rowCount++, 1);

    QLabel* columnHeaderLabel = new QLabel("column header");
    _columnHeaderCheckBox = new QCheckBox();
    {
        const auto value = settings.value(Keys::columnHeaderValueKey);
        if (value.isValid())
            _columnHeaderCheckBox->setChecked(value.toBool());
    }
    fileDialogLayout->addWidget(columnHeaderLabel, rowCount, 0);
    fileDialogLayout->addWidget(_columnHeaderCheckBox, rowCount++, 1);

    QLabel* rowHeaderLabel = new QLabel("row header");
    _rowHeaderCheckBox = new QCheckBox();
    {
        const auto value = settings.value(Keys::rowHeaderValueKey);
        if (value.isValid())
            _rowHeaderCheckBox->setChecked(value.toBool());
    }
    fileDialogLayout->addWidget(rowHeaderLabel, rowCount, 0);
    fileDialogLayout->addWidget(_rowHeaderCheckBox, rowCount++, 1);

    QLabel* transposeLabel = new QLabel("transpose");
    _transposeCheckBox = new QCheckBox();
    {
        const auto value = settings.value(Keys::transposeValueKey);
        if (value.isValid())
            _transposeCheckBox->setChecked(value.toBool());
    }
    fileDialogLayout->addWidget(transposeLabel, rowCount, 0);
    fileDialogLayout->addWidget(_transposeCheckBox, rowCount++, 1);


    QLabel* sourceTypeLabel = new QLabel("Source Data");
    _sourceTypeComboBox = new QComboBox;
    _sourceTypeComboBox->addItem("Mixed (auto-detect)", 0);
    _sourceTypeComboBox->addItem("Numerical", 1);
    _sourceTypeComboBox->addItem("Categorical", 2);
   
    fileDialogLayout->addWidget(sourceTypeLabel, rowCount, 0);
    fileDialogLayout->addWidget(_sourceTypeComboBox, rowCount++, 1);

    QLabel* mixedDataHierarchyLabel = new QLabel("Mixed Hierarchy");
    _mixedDataHierarchyCheckbox = new QCheckBox();
    {
        const auto value = settings.value(Keys::hierarchyValueKey);
        if (value.isValid())
            _mixedDataHierarchyCheckbox->setChecked(value.toBool());
    }
    QObject::connect(_sourceTypeComboBox, &QComboBox::currentIndexChanged, [mixedDataHierarchyLabel, this](int index)
        {
            mixedDataHierarchyLabel->setVisible(index == 0);
            this->_mixedDataHierarchyCheckbox->setVisible(index == 0);
        });

    fileDialogLayout->addWidget(mixedDataHierarchyLabel, rowCount, 0);
    fileDialogLayout->addWidget(_mixedDataHierarchyCheckbox, rowCount++, 1);

    QLabel* storageTypeLabel = new QLabel("Numerical Storage");
    _storageTypeComboBox = new QComboBox;
    _storageTypeComboBox->addItem("Float (32-bits)", 1);
    _storageTypeComboBox->addItem("BFloat16 (16-bits)", 2);
    _storageTypeComboBox->setCurrentIndex([&settings]
        {
            const auto value = settings.value(Keys::storageValueKey);
            if (value.isValid())return value.toInt();
            return 1;
        }());

    fileDialogLayout->addWidget(storageTypeLabel, rowCount, 0);
    fileDialogLayout->addWidget(_storageTypeComboBox, rowCount, 1);


    QFileDialog& fileDialogRef = _fileDialog;
    IfValid(settings.value(Keys::selectedNameFilterKey), [&fileDialogRef](const QVariant& value)
        {
            fileDialogRef.selectNameFilter(value.toString());
        });
    IfValid(settings.value(Keys::fileNameKey), [&fileDialogRef](const QVariant& value)
        {
            fileDialogRef.selectFile(value.toString());
        });

    _sourceTypeComboBox->setCurrentIndex([&settings]
        {
            const auto value = settings.value(Keys::sourceValueKey);
            if (value.isValid())return value.toInt();
            return 0;
        }());

    const auto onFilterSelected = [separatorLabel, this](const QString& nameFilter)
    {
        const bool isTSVSelected{ nameFilter == "TSV (*.tsv)" };
        this->_separatorLineEdit->setVisible(!isTSVSelected);
        separatorLabel->setVisible(!isTSVSelected);
    };

    QObject::connect(&_fileDialog, &QFileDialog::filterSelected, onFilterSelected);
    onFilterSelected(_fileDialog.selectedNameFilter());
}





void CsvLoader::loadData()
{
    QSettings settings(QString::fromLatin1("HDPS"), QString::fromLatin1("Plugins/ExtCsvLoader"));


   

    if (_fileDialog.exec())
    {
        QStringList fileNames = _fileDialog.selectedFiles();

        if (fileNames.empty())
        {
            return;
        }
        const QString firstFileName = fileNames.constFirst();

        bool result = true;
        QString selectedNameFilter = _fileDialog.selectedNameFilter();

        const char sep = _separatorLineEdit->text().toStdString()[0];
        settings.setValue(Keys::separatorValueKey, sep);
        settings.setValue(Keys::columnHeaderValueKey, _columnHeaderCheckBox->isChecked());
        settings.setValue(Keys::rowHeaderValueKey, _rowHeaderCheckBox->isChecked());

    	settings.setValue(Keys::transposeValueKey, _transposeCheckBox->isChecked());
        settings.setValue(Keys::sourceValueKey, _sourceTypeComboBox->currentIndex());
        settings.setValue(Keys::storageValueKey, _storageTypeComboBox->currentIndex());
        settings.setValue(Keys::fileNameKey, firstFileName);
        settings.setValue(Keys::selectedNameFilterKey, selectedNameFilter);

        char selected_separator = _separatorLineEdit->text()[0].toLatin1();
        if (selectedNameFilter == "TSV (*.tsv)")
        {
            selected_separator = '\t';
        }
        ExtCsvLoader::CSVReader reader(firstFileName, selected_separator, _columnHeaderCheckBox->isChecked(), _rowHeaderCheckBox->isChecked());
        reader.read();
       

        int sourceType = _sourceTypeComboBox->currentData().toInt();
        bool transposed = _transposeCheckBox->isChecked();

        if(sourceType ==1)
        {
            int storageType = _storageTypeComboBox->currentData().toInt();

            Dataset<Points> pointsDataset = ::createPointsDataset(_core, true, QFileInfo(firstFileName).baseName());

            if (storageType == 1)
            {
                pointsDataset->setDataElementType<float>();
                float* data_ptr = reader.get_data<float>(transposed);
                if (transposed)
                {
                    pointsDataset->setData(data_ptr, reader.columns(), reader.rows());
                }
                else
                {
                    pointsDataset->setData(data_ptr, reader.rows(), reader.columns());
                }
                delete[] data_ptr;
            }
            else if (storageType == 2)
            {
                pointsDataset->setDataElementType<biovault::bfloat16_t>();
                biovault::bfloat16_t* data_ptr = reader.get_data< biovault::bfloat16_t>(transposed);
                if (transposed)
                {
                    pointsDataset->setData(data_ptr, reader.columns(), reader.rows());
                }
                else
                {
                    pointsDataset->setData(data_ptr, reader.rows(), reader.columns());
                }
                delete [] data_ptr;
            }

            if (transposed)
            {
                pointsDataset->setDimensionNames(toQStringVector(reader.GetRowHeader()));
                std::vector<QString> sampleNames = toQStringVector(reader.GetColumnHeader());
                pointsDataset->setProperty("Sample Names", QList<QVariant>(sampleNames.cbegin(), sampleNames.cend()));
            }
            else
            {
                pointsDataset->setDimensionNames(toQStringVector(reader.GetColumnHeader()));
                std::vector<QString> sampleNames = toQStringVector(reader.GetRowHeader());
                pointsDataset->setProperty("Sample Names", QList<QVariant>(sampleNames.cbegin(), sampleNames.cend()));
            }


            // Notify others that the clusters have changed
#if defined(MANIVAULT_API_Old)
            events().notifyDatasetChanged(pointsDataset);
#elif defined(MANIVAULT_API_New)
            events().notifyDatasetDataChanged(pointsDataset);
#endif
            
        }
        else  
        {
            std::string* data_ptr = reader.get_data<std::string>(transposed);
			std::ptrdiff_t items = transposed ? reader.rows() : reader.columns();
            std::size_t size = transposed ? reader.columns() : reader.rows();
			std::vector<std::string> clusterNames = transposed ? reader.GetRowHeader() : reader.GetColumnHeader();

            enum{DT_UNKNOWN, DT_NUMERICAL, DT_CATEGORICAL, DT_COLOR};

            std::vector<uint8_t> detectedDataType(items, DT_UNKNOWN);

            std::vector<std::map<std::string, std::vector<unsigned int>>> cluster_info(items);
            std::vector<std::ptrdiff_t> nrOfColors(items, 0);
            std::vector<std::ptrdiff_t> hasColor(items);
            std::vector<uint8_t> processed(items, 0);

            if(sourceType == 0) // autodetect
            {
				#pragma omp parallel for schedule(dynamic,1)
                for (std::ptrdiff_t i = 0; i < items; ++i)
                {
                    bool isNumerical = true;
                    bool isColor = true;
                    for (std::size_t s = 0; s < size; ++s)
                    {
                        std::string value = data_ptr[(s * items) + i];

                        if (!value.empty())
                        {
                            if (isNumerical)
                                isNumerical &= is_number(value);
                            if (isColor)
                                isColor &= QColor::isValidColor(value.c_str());
                        }
                    }
                    if (isNumerical)
                        detectedDataType[i] = DT_NUMERICAL;
                    else if (isColor)
                        detectedDataType[i] = DT_COLOR;
                    else
                        detectedDataType[i] = DT_CATEGORICAL;
                }
            }
            else // treat everything as categorical or color
            {
				#pragma omp parallel for schedule(dynamic,1)
                for (std::ptrdiff_t i = 0; i < items; ++i)
                {
                    bool isColor = true;
                    for (std::size_t s = 0; s < size; ++s)
                    {
                        std::string value = data_ptr[(s * items) + i];

                        if (!value.empty())
                        {
                            if (isColor)
                                isColor &= QColor::isValidColor(value.c_str());
                        }
                    }
					if (isColor)
                        detectedDataType[i] = DT_COLOR;
                    else
                        detectedDataType[i] = DT_CATEGORICAL;
                }
            }

            const std::ptrdiff_t nrOfNumericalItems = std::count(detectedDataType.cbegin(), detectedDataType.cend(), DT_NUMERICAL);

            Dataset<Points> pointsDataset;
            if(nrOfNumericalItems)
            {
                int storageType = _storageTypeComboBox->currentData().toInt();

                pointsDataset = ::createPointsDataset(_core, true, QFileInfo(firstFileName).baseName());

                std::vector<std::string> sourceColumnHeader = transposed ? reader.GetRowHeader() : reader.GetColumnHeader();
                std::vector<QString> columnHeader(nrOfNumericalItems);

                if (storageType == 1)
                {
                    pointsDataset->setDataElementType<float>();

                    std::vector<float> temp(nrOfNumericalItems* size);
                    std::ptrdiff_t numericalIndex = 0;
                    for (std::ptrdiff_t i = 0; i < items; ++i)
                    {
                        if ((detectedDataType[i] == DT_NUMERICAL))
                        {
							#pragma omp parallel for schedule(dynamic,1)
                            for (std::ptrdiff_t s = 0; s < size; ++s)
                            {
                                std::string value = data_ptr[(s * items) + i];
                                if (value.empty())
                                    temp[(nrOfNumericalItems * s) + numericalIndex] = 0;
                                else
									temp[(nrOfNumericalItems * s) + numericalIndex] = std::stof(value);
                            }

                            columnHeader[numericalIndex] = sourceColumnHeader[i].c_str();
                            ++numericalIndex;
                        }
                    }
                    if (transposed)
                        pointsDataset->setData(temp.data(), nrOfNumericalItems, size);
                    else
                        pointsDataset->setData(temp.data(), size, nrOfNumericalItems);
                }
                else
                {
                    pointsDataset->setDataElementType<biovault::bfloat16_t>();
                    std::vector<biovault::bfloat16_t> temp(nrOfNumericalItems* size);
                    std::ptrdiff_t numericalIndex = 0;

                   
                    for (std::ptrdiff_t i = 0; i < items; ++i)
                    {
                        if ((detectedDataType[i] == DT_NUMERICAL))
                        {
							#pragma omp parallel for schedule(dynamic,1)
                            for (std::ptrdiff_t s = 0; s < size; ++s)
                            {
                                std::string value = data_ptr[(s * items) + i];
                                temp[(nrOfNumericalItems * s) + numericalIndex] = std::stof(value);
                            }

                            columnHeader[numericalIndex] = sourceColumnHeader[i].c_str();

                            ++numericalIndex;
                            processed[i] = 1;
                        }
                    }
                    if (transposed)
                    {
                        pointsDataset->setData(temp.data(), nrOfNumericalItems, size);
                        
                    }
                    else
                    {
                        pointsDataset->setData(temp.data(), size, nrOfNumericalItems);
                        
                    }
                }

                pointsDataset->setDimensionNames(columnHeader);

                if (transposed)
                {
                    std::vector<QString> sampleNames = toQStringVector(reader.GetColumnHeader());
                    pointsDataset->setProperty("Sample Names", QList<QVariant>(sampleNames.cbegin(), sampleNames.cend()));
                }
                else
                {
                    std::vector<QString> sampleNames = toQStringVector(reader.GetRowHeader());
                    pointsDataset->setProperty("Sample Names", QList<QVariant>(sampleNames.cbegin(), sampleNames.cend()));
                }
            }

            

            
            const std::size_t nrOfCategoricalItems = std::count(detectedDataType.cbegin(), detectedDataType.cend(), DT_CATEGORICAL);
            const std::size_t nrOfColorItems = std::count(detectedDataType.cbegin(), detectedDataType.cend(), DT_COLOR);

            
                

            if (nrOfCategoricalItems || nrOfColorItems)
            {
#pragma omp parallel for schedule(dynamic,1)
                for (std::ptrdiff_t i = 0; i < items; ++i)
                {
                    if ((detectedDataType[i] == DT_CATEGORICAL) | (detectedDataType[i] == DT_COLOR))
                    {
                        for (std::size_t s = 0; s < size; ++s)
                        {
                            std::string value = data_ptr[(s * items) + i];
                            if (value.empty())
                                value = "N/A";
                            cluster_info[i][value].push_back(s);
                        }
                    }
                    for (std::map<std::string, std::vector<unsigned int>>::iterator it = cluster_info[i].begin(); it != cluster_info[i].end(); ++it)
                        std::sort(it->second.begin(), it->second.end());
                    if (detectedDataType[i] == DT_COLOR)
                        nrOfColors[i] = cluster_info[i].size();
                }
            }
            delete[] data_ptr;

            if (nrOfCategoricalItems || nrOfColorItems)
            {
				#pragma omp parallel for schedule(dynamic,1)
                for (std::ptrdiff_t i = 0; i < items; ++i)
                {
                    if (nrOfColors[i] > 0)
                    {
                        for (std::ptrdiff_t offset = 1; offset < items; ++offset) // we look for the items closest to the color
                        {
                            for (std::size_t plus = 0; plus < 2; ++plus)
                            {
                                std::ptrdiff_t j = plus ? i + offset : i - offset;
                                if (j >= 0 && j < items)
                                {
                                    if (nrOfColors[j] == 0) // j should not be a color itself
                                    {
                                        if (nrOfColors[i] <= cluster_info[j].size()) // the number of colors in i should be less or equal to the number of clusters in j;
                                        {
                                            bool success = true;
                                            // test if all color indices match with cluster indices, look for exact matches
                                            
                                            for (auto color_it = cluster_info[i].cbegin(); success && (color_it != cluster_info[i].cend()); ++color_it)
                                            {
                                                bool exact_match = false;
                                                for (auto cluster_it = cluster_info[j].cbegin(); success && (cluster_it != cluster_info[j].cend()); ++cluster_it)
                                                {
                                                    exact_match |= (color_it->second == cluster_it->second);

                                                }
                                                success &= exact_match;
                                            }
                                        	if (success)
                                            {
                                                hasColor[j] = i; // i is a color for j
                                                offset = items;
                                                plus = 2;
                                            }

                                            if(false) // not using inexact matches for now
                                            {
                                                // test if all color indices match with cluster indices. it's ok for mutiple clusters to have the same color
                                                for (auto color_it = cluster_info[i].cbegin(); success && (color_it != cluster_info[i].cend()); ++color_it)
                                                {

                                                    bool found_match = false;
                                                    for (auto cluster_it = cluster_info[j].cbegin(); success && (cluster_it != cluster_info[j].cend()); ++cluster_it)
                                                    {
                                                        found_match |= std::includes(color_it->second.cbegin(), color_it->second.cend(), cluster_it->second.cbegin(), cluster_it->second.cend());
                                                    }
                                                    success &= found_match;
                                                }

                                                if (success)
                                                {
                                                    hasColor[j] = i; // i is a color for j
                                                    offset = items;
                                                    plus = 2;
                                                }
                                            }
                                            
                                        }
                                    }
                                }
                            }
                        }
                    }
                }


                // time to make the clusters, first process the non-colors
                
                for (std::ptrdiff_t i = 0; i < items; ++i)
                {
                    if (detectedDataType[i] == DT_CATEGORICAL)
                    {
                        QString name = clusterNames[i].c_str();
                        
                        Dataset<Clusters> clusterDataset = (_mixedDataHierarchyCheckbox->isChecked() && nrOfNumericalItems) ? _core->addDataset("Cluster", name, pointsDataset) : _core->addDataset("Cluster", name);
                        // Notify others that the dataset was added
                        events().notifyDatasetAdded(clusterDataset);

                        std::ptrdiff_t colorIndex = hasColor[i];
                        std::vector<QColor> generated_colors;
                        if (colorIndex < 0)
                        {
                            CreateColorVector(cluster_info[i].size(), generated_colors);
                        }
                        std::size_t index = 0;
                        for (auto it = cluster_info[i].cbegin(); it != cluster_info[i].cend(); ++it, ++index)
                        {
                            Cluster cluster;
                            cluster.setIndices(it->second);
                            cluster.setName(it->first.c_str());
                            if (colorIndex >= 0)
                            {
                                for (auto color_it = cluster_info[colorIndex].cbegin(); color_it != cluster_info[colorIndex].cend(); ++color_it)
                                {
                                    if (std::includes(color_it->second.cbegin(), color_it->second.cend(), it->second.cbegin(), it->second.cend()))
                                    {
                                        cluster.setColor(QColor(QString(color_it->first.c_str())));
                                    }
                                }
                            }
                            else
                            {
                                cluster.setColor(generated_colors[index]);
                            }

                            clusterDataset->addCluster(cluster);
                        }
                        processed[i] = 1;
                        if (colorIndex >= 0)
                            processed[colorIndex] = 1; // color has been processed

                            // Notify others that the clusters have changed
#if defined(MANIVAULT_API_Old)
                        events().notifyDatasetChanged(clusterDataset);
#elif defined(MANIVAULT_API_New)
                        events().notifyDatasetDataChanged(clusterDataset);
#endif
                    }
                }
            }
			
            // process unused colors
            for (std::ptrdiff_t i = 0; i < items; ++i)
            {
                if (detectedDataType[i] == DT_COLOR)
	            if(processed[i] == 0)
	            {
                    QString name = clusterNames[i].c_str();
                    Dataset<Clusters> clusterDataset = (_mixedDataHierarchyCheckbox->isChecked() && nrOfNumericalItems) ? _core->addDataset("Cluster", name, pointsDataset) : _core->addDataset("Cluster", name);
                    // Notify others that the dataset was added
                            
#if defined(MANIVAULT_API_Old)
                    events().notifyDatasetChanged(clusterDataset);
#elif defined(MANIVAULT_API_New)
                    events().notifyDatasetDataChanged(clusterDataset);
#endif

                    
                    for (auto it = cluster_info[i].cbegin(); it != cluster_info[i].cend(); ++it)
                    {
                        Cluster cluster;
                        cluster.setIndices(it->second);
                        cluster.setName(it->first.c_str());
                        cluster.setColor(QColor(QString(it->first.c_str())));

                        clusterDataset->addCluster(cluster);
                    }
                    processed[i] = 1;
                    
                    // Notify others that the clusters have changed
#if defined(MANIVAULT_API_Old)
                    events().notifyDatasetChanged(clusterDataset);
#elif defined(MANIVAULT_API_New)
                    events().notifyDatasetDataChanged(clusterDataset);
#endif
	            }
            }
        }
    }
}


QIcon CsvLoaderFactory::getIcon(const QColor& color /*= Qt::black*/) const
{
    return createPluginIcon("CSV", color);
}

// =============================================================================
// Factory
// =============================================================================

LoaderPlugin* CsvLoaderFactory::produce()
{
    return new CsvLoader(this);
}

hdps::DataTypes CsvLoaderFactory::supportedDataTypes() const
{
    hdps::DataTypes supportedTypes;
    supportedTypes.append(PointType);
    return supportedTypes;
}
