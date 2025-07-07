#include "CsvLoader.h"

#include "csvreader.h"

#include <Dataset.h>

#include <ClusterData/ClusterData.h>
#include <PointData/DimensionsPickerAction.h>
#include <PointData/PointData.h>
#include <util/Icon.h>
#include <util/StyledIcon.h>

#include <QDialogButtonBox>
#include <QMainWindow>
#include <QtCore>

#include <algorithm>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>

Q_PLUGIN_METADATA(IID "nl.lumc.ExtCsvLoader")

// =============================================================================
// Loader
// =============================================================================

using namespace mv;
using namespace mv::gui;

namespace
{
    std::vector<QString> toQStringVector(const std::vector<std::string>& v)
    {
        std::vector<QString> result(v.size());
        for (std::size_t i = 0; i < v.size(); ++i)
            result[i] = v[i].c_str();
        return result;
    }
    QVariantList toQVariantList(const std::vector<std::string>& v)
    {
        QVariantList result(v.size());
        for (std::size_t i = 0; i < v.size(); ++i)
            result[i] = QString::fromStdString(v[i]);
        return result;
    }
    std::vector<std::string> toStringVector(const QVariantList& l)
    {
        std::vector<std::string> result(l.size());
        for (std::size_t i = 0; i < l.size(); ++i)
            result[i] = l[i].toString().toStdString();
        return result;
    }

    Dataset<Points> createPointsDataset(QString dataSetName, Dataset<DatasetImpl> parentDataset = Dataset<DatasetImpl>(), bool makeDerived=false)
    {
        if (parentDataset.isValid())
        {
            if (makeDerived)
            {
                return mv::data().createDerivedDataset(dataSetName, parentDataset);
            }
            else
            {
                return mv::data().createDataset("Points", dataSetName, parentDataset);
            }
        }
        else
        {
            return mv::data().createDataset("Points", dataSetName);
        }
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


CsvLoader::CsvLoader(const PluginFactory* factory) : LoaderPlugin(factory)
, _separatorLineEdit(this, "Separator")
, _columnHeaderCheckBox(this, "Column header")
, _rowHeaderCheckBox(this, "Row header")
, _transposeCheckBox(this, "Transpose")
, _mixedDataHierarchyCheckbox(this, "Mixed Hierarchy")
, _sourceTypeComboBox(this, "Source Data")
, _storageTypeComboBox(this, "Numerical Storage")
, _datasetPickerAction(this, "Parent Dataset")
, _derivedDataCheckBox(this, "Derived dataset")
{

}
CsvLoader::~CsvLoader(void)
{

}

// Alphabetic list of keys used to access settings from QSettings.
namespace Keys
{
    const QString columnHeaderValueKey("columnHeader");
    const QString fileNameKey("fileName");
    const QString hierarchyValueKey("hierarchy");
    const QString rowHeaderValueKey("rowHeader");
    const QString selectedNameFilterKey("selectedNameFilter");
    const QString separatorValueKey("separatorValue");
    const QString sourceValueKey("sourceValue");
    const QString storageValueKey("storageValue");
    const QString transposeValueKey("transposeValue");
    const QString derivedDataValueKey("derivedData");
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

    QGridLayout* fileDialogLayout = dynamic_cast<QGridLayout*>(_fileDialog.layout());

    int rowCount = fileDialogLayout->rowCount();

    auto updateseparatorString = [this]() {
        QString text = _separatorLineEdit.getString();
        if (text.length() > 1) {
            text = text.right(1);
            _separatorLineEdit.setString(text);
        }
        };
    
    {
        const auto separator = getSetting(Keys::separatorValueKey, ",").toChar();
        _separatorLineEdit.setString(separator);
    }
    QLabel* separatorLineEditLabel = new QLabel("Separator:");
    fileDialogLayout->addWidget(separatorLineEditLabel, rowCount, 0);
    fileDialogLayout->addWidget(_separatorLineEdit.createWidget(&_fileDialog), rowCount++, 1);
    connect(&_separatorLineEdit, &StringAction::stringChanged, this, updateseparatorString);
    {
        const auto columnHeaderValue = getSetting(Keys::columnHeaderValueKey).toBool();
        _columnHeaderCheckBox.setChecked(columnHeaderValue);
    }
    QLabel* columnHeaderCheckBoxLabel = new QLabel("Column header:");
    fileDialogLayout->addWidget(columnHeaderCheckBoxLabel, rowCount, 0);
    fileDialogLayout->addWidget(_columnHeaderCheckBox.createWidget(&_fileDialog), rowCount++, 1);
    {
        const auto rowHeaderValue = getSetting(Keys::rowHeaderValueKey).toBool();
        _rowHeaderCheckBox.setChecked(rowHeaderValue);
    }
    QLabel* rowHeaderCheckBoxLabel = new QLabel("Row header:");
    fileDialogLayout->addWidget(rowHeaderCheckBoxLabel, rowCount, 0);
    fileDialogLayout->addWidget(_rowHeaderCheckBox.createWidget(&_fileDialog), rowCount++, 1);
    {
        const auto transposeValue = getSetting(Keys::transposeValueKey).toBool();
        _transposeCheckBox.setChecked(transposeValue);
    }
    QLabel* transposeCheckBoxLabel = new QLabel("Transpose:");
    fileDialogLayout->addWidget(transposeCheckBoxLabel, rowCount, 0);
    fileDialogLayout->addWidget(_transposeCheckBox.createWidget(&_fileDialog), rowCount++, 1);

    _sourceTypeComboBox.initialize({ "Mixed (auto-detect)" ,"Numerical","Categorical"}, "Mixed (auto-detect)");
    QLabel* sourceTypeComboBoxLabel = new QLabel("Source Data:");
    fileDialogLayout->addWidget(sourceTypeComboBoxLabel, rowCount, 0);
    fileDialogLayout->addWidget(_sourceTypeComboBox.createWidget(&_fileDialog), rowCount++, 1);
    {
        const auto hierarchyValue = getSetting(Keys::hierarchyValueKey).toBool();
        _mixedDataHierarchyCheckbox.setChecked(hierarchyValue);
    }
    QLabel* mixedDataHierarchyCheckboxLabel = new QLabel("Mixed Hierarchy:");
    fileDialogLayout->addWidget(mixedDataHierarchyCheckboxLabel, rowCount, 0);
    fileDialogLayout->addWidget(_mixedDataHierarchyCheckbox.createWidget(&_fileDialog), rowCount++, 1);

    QLabel* storageTypeComboBoxLabel = new QLabel("Numerical Storage:");
    _storageTypeComboBox.initialize({"Float (32-bits)" ,"BFloat16 (16-bits)" }, "BFloat16 (16-bits)");
    fileDialogLayout->addWidget(storageTypeComboBoxLabel, rowCount, 0);
    fileDialogLayout->addWidget(_storageTypeComboBox.createWidget(&_fileDialog), rowCount++, 1);

    // Get unique identifier and gui names from all point data sets in the core
    auto dataSets = mv::data().getAllDatasets(std::vector<mv::DataType> {PointType});

    // Assign found dataset(s)
    _datasetPickerAction.setDatasets(dataSets);

    QLabel* datasetPickerActionLabel = new QLabel("Parent Dataset:");
    fileDialogLayout->addWidget(datasetPickerActionLabel, rowCount, 0);
    fileDialogLayout->addWidget(_datasetPickerAction.createWidget(&_fileDialog), rowCount++, 1);

    _derivedDataCheckBox.setChecked(getSetting(Keys::derivedDataValueKey).toBool());
    _derivedDataCheckBox.setEnabled(false);
    QLabel* derivedDataCheckBoxLabel = new QLabel("Derived dataset:");
    fileDialogLayout->addWidget(derivedDataCheckBoxLabel, rowCount, 0);
    fileDialogLayout->addWidget(_derivedDataCheckBox.createWidget(&_fileDialog), rowCount++, 1);

    // Lambda to update checkbox state
    auto updateDerivedDataCheckbox = [this]() {
        bool parentDatasetValid = _datasetPickerAction.getCurrentDataset().isValid();
        bool isNumericalMixed = (_sourceTypeComboBox.getCurrentText() == "Numerical"|| _sourceTypeComboBox.getCurrentText() == "Mixed (auto-detect)");
        bool enable = parentDatasetValid && isNumericalMixed;
        _derivedDataCheckBox.setEnabled(enable);
        if (!enable) {
            _derivedDataCheckBox.setChecked(false);
        }
        };
    auto updateSourceTypeComboBox = [updateDerivedDataCheckbox,this]() {
        updateDerivedDataCheckbox();
        _mixedDataHierarchyCheckbox.setEnabled(_sourceTypeComboBox.getCurrentIndex() == 0);
        };
    // Connect signals to update the checkbox state

    connect(&_sourceTypeComboBox, &OptionAction::currentIndexChanged, this, updateSourceTypeComboBox);

    connect(&_datasetPickerAction, &DatasetPickerAction::currentIndexChanged, this, updateDerivedDataCheckbox);

    // Initial state update
    updateDerivedDataCheckbox();

    const auto selectedNameFilterSetting = getSetting(Keys::selectedNameFilterKey, QVariant());
    if (selectedNameFilterSetting.isValid())
        _fileDialog.selectNameFilter(selectedNameFilterSetting.toString());

    const auto fileNameSetting = getSetting(Keys::fileNameKey, QVariant());
    if (fileNameSetting.isValid())
        _fileDialog.selectFile(fileNameSetting.toString());

    _sourceTypeComboBox.setCurrentIndex(getSetting(Keys::sourceValueKey, 0).toInt());

    const auto onFilterSelected = [this](const QString& nameFilter)
    {
        const bool isTSVSelected{ nameFilter == "TSV (*.tsv)" };
        _separatorLineEdit.setString(isTSVSelected ? "\t" : ",");
        _separatorLineEdit.setEnabled(!isTSVSelected);
    };

    QObject::connect(&_fileDialog, &QFileDialog::filterSelected, onFilterSelected);
    onFilterSelected(_fileDialog.selectedNameFilter());
}


void CsvLoader::loadData()
{
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

        const char sep = _separatorLineEdit.getString().toStdString()[0];
        setSetting(Keys::separatorValueKey, sep);
        setSetting(Keys::columnHeaderValueKey, _columnHeaderCheckBox.isChecked());
        setSetting(Keys::rowHeaderValueKey, _rowHeaderCheckBox.isChecked());

        setSetting(Keys::transposeValueKey, _transposeCheckBox.isChecked());
        setSetting(Keys::sourceValueKey, _sourceTypeComboBox.getCurrentIndex());
        setSetting(Keys::storageValueKey, _storageTypeComboBox.getCurrentIndex());
        setSetting(Keys::fileNameKey, firstFileName);
        setSetting(Keys::selectedNameFilterKey, selectedNameFilter);
        setSetting(Keys::derivedDataValueKey, _derivedDataCheckBox.isChecked());

        char selected_separator = _separatorLineEdit.getString()[0].toLatin1();
        if (selectedNameFilter == "TSV (*.tsv)")
        {
            selected_separator = '\t';
        }
        ExtCsvLoader::CSVReader reader(firstFileName, selected_separator, _columnHeaderCheckBox.isChecked(), _rowHeaderCheckBox.isChecked());
        reader.read();

        const QString sourceType    = _sourceTypeComboBox.getCurrentText();
        const bool transposed   = _transposeCheckBox.isChecked();

        auto parentDataset      = _datasetPickerAction.getCurrentDataset();

        std::vector<std::string> parent_labels;

        if (parentDataset.isValid() && parentDataset->hasProperty("Sample Names"))
        {
            QVariantList parentSampleNameList;
            parentSampleNameList = parentDataset->getProperty("Sample Names").toList();
            parent_labels = toStringVector(parentSampleNameList);
        }

        std::vector<std::string> dimension_labels;
        if(!_transposeCheckBox.isChecked())
        {
            auto loadedColumnHeader = reader.GetColumnHeader();
            std::vector<QString> dimensionNames(loadedColumnHeader.size());
            for (std::size_t i = 0; i < dimensionNames.size(); ++i)
            {
                dimensionNames[i] = loadedColumnHeader[i].c_str();
            }
            Dataset<Points> tempDataset = mv::data().createDataset("Points", "temp");
            tempDataset->getDataHierarchyItem().setVisible(false);
            tempDataset->setData(std::vector<int8_t>(dimensionNames.size()), dimensionNames.size());
            tempDataset->setDimensionNames(dimensionNames);

            QDialog dialog(Application::getMainWindow());
            QGridLayout* layout = new QGridLayout;

            DimensionsPickerAction& dimensionPickerAction = tempDataset->getDimensionsPickerAction();;
            layout->addWidget(new QLabel("Select Dimensions:"));
            layout->addWidget(dimensionPickerAction.createWidget(Application::getMainWindow()));
            auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
            buttonBox->connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            layout->addWidget(buttonBox, 3, 0, 1, 2);
            dialog.setLayout(layout);
            auto result = dialog.exec();
            if (result != 0)
            {
                auto selectedDimensions = dimensionPickerAction.getSelectedDimensions();
                dimension_labels.reserve(selectedDimensions.size());
                for(auto dim : selectedDimensions)
                {
                    dimension_labels.push_back(loadedColumnHeader[dim]);
                }
            }
            mv::data().removeDataset(tempDataset);
        }
        
        std::vector<std::string> column_header;
        std::vector<std::string> row_header;
        if (sourceType == "Numerical")
        {
            QString storageType = _storageTypeComboBox.getCurrentText();

            Dataset<Points> pointsDataset;

            if (storageType == "Float (32-bits)")
            {
                float* data_ptr = reader.get_data<float>(transposed, column_header, row_header, parent_labels, dimension_labels);
                if (data_ptr)
                {
                    pointsDataset = ::createPointsDataset(QFileInfo(firstFileName).baseName(), parentDataset, _derivedDataCheckBox.isChecked());;
                    pointsDataset->setDataElementType<float>();
                    pointsDataset->setData(data_ptr, row_header.size(), column_header.size());

                    events().notifyDatasetDataChanged(pointsDataset);
                    events().notifyDatasetDataDimensionsChanged(pointsDataset);

                    delete[] data_ptr;
                }
                else
                {
                    return;
                }


            }
            else if (storageType == "BFloat16 (16-bits)")
            {
                biovault::bfloat16_t* data_ptr = reader.get_data< biovault::bfloat16_t>(transposed, column_header, row_header, parent_labels, dimension_labels);
                if (data_ptr)
                {
                    pointsDataset = ::createPointsDataset(QFileInfo(firstFileName).baseName(), parentDataset, _derivedDataCheckBox.isChecked());;
                    pointsDataset->setDataElementType<biovault::bfloat16_t>();
                    pointsDataset->setData(data_ptr, row_header.size(), column_header.size());

                    events().notifyDatasetDataChanged(pointsDataset);
                    events().notifyDatasetDataDimensionsChanged(pointsDataset);

                    delete[] data_ptr;
                }
                else
                {
                    return;
                }

            }

            if (pointsDataset.isValid())
            {
                pointsDataset->setDimensionNames(toQStringVector(column_header));
                pointsDataset->setProperty("Sample Names", toQVariantList(row_header));

                // Notify others that the clusters have changed
                events().notifyDatasetDataChanged(pointsDataset);
                events().notifyDatasetDataDimensionsChanged(pointsDataset);
            }

        }
        else
        {
            std::string* data_ptr = reader.get_data<std::string>(transposed, column_header, row_header, parent_labels, dimension_labels);
            if (data_ptr == nullptr)
            {
                return;
            }
            qDebug() << "get_data finished, " << column_header.size() << " x " << row_header.size() << " values retrieved";
            std::ptrdiff_t items = column_header.size();
            std::size_t size = row_header.size();
            std::vector<std::string> clusterNames = column_header;

            enum { DT_UNKNOWN, DT_NUMERICAL, DT_CATEGORICAL, DT_COLOR };

            std::vector<uint8_t> detectedDataType(items, DT_UNKNOWN);

            std::vector<std::map<std::string, std::vector<unsigned int>>> cluster_info(items);
            std::vector<std::ptrdiff_t> nrOfColors(items, 0);
            std::vector<std::ptrdiff_t> hasColor(items,-1);
            std::vector<uint8_t> processed(items, 0);

            if (sourceType == "Mixed (auto-detect)") // autodetect
            {
#pragma omp parallel for schedule(dynamic,1)
                for (std::ptrdiff_t i = 0; i < items; ++i)
                {
                    bool isNumerical = true;
                    bool isColor = true;
                    bool continueLoop = true;
                    for (std::size_t s = 0; continueLoop && (s < size); ++s)
                    {
                        std::string value = data_ptr[(s * items) + i];

                        if (!value.empty())
                        {
                            if (isNumerical)
                                isNumerical &= is_number(value);
                            if (isColor)
                                isColor &= QColor::isValidColor(value.c_str());
                            continueLoop = (isNumerical || isColor);
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
                    for (std::size_t s = 0; isColor && (s < size); ++s)
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
            if (nrOfNumericalItems)
            {
                QString storageType = _storageTypeComboBox.getCurrentText();

                pointsDataset = ::createPointsDataset(QFileInfo(firstFileName).baseName(), parentDataset, _derivedDataCheckBox.isChecked());

                std::vector<std::string> sourceColumnHeader = column_header;
                std::vector<QString> columnHeader(nrOfNumericalItems);

                if (storageType == "Float (32-bits)")
                {
                    pointsDataset->setDataElementType<float>();

                    std::vector<float> temp(nrOfNumericalItems * size);
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
                    pointsDataset->setData(temp.data(), size, nrOfNumericalItems);

                    events().notifyDatasetDataChanged(pointsDataset);
                    events().notifyDatasetDataDimensionsChanged(pointsDataset);
                }
                else
                {
                    pointsDataset->setDataElementType<biovault::bfloat16_t>();
                    std::vector<biovault::bfloat16_t> temp(nrOfNumericalItems * size);
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
                            processed[i] = 1;
                        }
                    }
                    pointsDataset->setData(temp.data(), size, nrOfNumericalItems);

                    events().notifyDatasetDataChanged(pointsDataset);
                    events().notifyDatasetDataDimensionsChanged(pointsDataset);

                }
                pointsDataset->setDimensionNames(columnHeader);
                pointsDataset->setProperty("Sample Names", toQVariantList(row_header));

                events().notifyDatasetDataChanged(pointsDataset);
                events().notifyDatasetDataDimensionsChanged(pointsDataset);

            }

            Dataset<DatasetImpl> parentDatasetOfClusterDataset = parentDataset;
            if (!parentDatasetOfClusterDataset.isValid())
            {
                if (_mixedDataHierarchyCheckbox.isChecked() && nrOfNumericalItems)
                    parentDatasetOfClusterDataset = pointsDataset;
            }
            const std::size_t nrOfCategoricalItems = std::count(detectedDataType.cbegin(), detectedDataType.cend(), DT_CATEGORICAL);
            const std::size_t nrOfColorItems = std::count(detectedDataType.cbegin(), detectedDataType.cend(), DT_COLOR);

            if (nrOfCategoricalItems || nrOfColorItems)
            {
#pragma omp parallel for schedule(dynamic,1)
                for (std::ptrdiff_t i = 0; i < items; ++i)
                {
                    if ((detectedDataType[i] == DT_CATEGORICAL) || (detectedDataType[i] == DT_COLOR))
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

                                            if (false) // not using inexact matches for now
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

                std::vector<Dataset<Clusters>> clusterDataset(items);

                for (std::ptrdiff_t i = 0; i < items; ++i)
                {
                    if (detectedDataType[i] == DT_CATEGORICAL)
                    {
                        QString name = clusterNames[i].c_str();
                       
                        clusterDataset[i] = mv::data().createDataset("Cluster", name, parentDatasetOfClusterDataset);
                    }
                }

#pragma omp parallel for schedule(dynamic,1)
                for (std::ptrdiff_t i = 0; i < items; ++i)
                {
                    if (detectedDataType[i] == DT_CATEGORICAL)
                    {
                        std::ptrdiff_t colorIndex = hasColor[i];
                        std::vector<QColor> generated_colors;
                        if (colorIndex < 0)
                        {
                            CreateColorVector(cluster_info[i].size(), generated_colors);
                        }
                        std::size_t index = 0;

                       // std::ptrdiff_t nrOfClustersToAdd = cluster_info[i].size();
                        //auto currentClusters = clusterDataset[i]->getClusters();
                       // currentClusters.resize(nrOfClustersToAdd);

                        /*
						#pragma omp parallel for schedule(dynamic,1)
                        for(std::ptrdiff_t c=0 ;c< nrOfClustersToAdd; ++c)
                        {
                            auto it = cluster_info[i].cbegin();
                            std::advance(it, c);
                        */
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
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                cluster.setColor(generated_colors[index]);
                            }
                            //currentClusters[c] = cluster;
                            clusterDataset[i]->addCluster(cluster);
                        }
                        processed[i] = 1;
                        
                        if (colorIndex >= 0)
                        {
                            processed[colorIndex] = 1; // color has been processed
                        }
                            
                    }
                }

                for (std::ptrdiff_t i = 0; i < items; ++i)
                {
                    if (detectedDataType[i] == DT_COLOR)
                        if (processed[i] == 0)
                        {
                            QString name = clusterNames[i].c_str();
                            clusterDataset[i] = mv::data().createDataset("Cluster", name, parentDatasetOfClusterDataset);
                        }
                }

                // process unused colors
#pragma omp parallel for schedule(dynamic,1)
                for (std::ptrdiff_t i = 0; i < items; ++i)
                {
                    if (detectedDataType[i] == DT_COLOR)
                        if (processed[i] == 0)
                        {
                            for (auto it = cluster_info[i].cbegin(); it != cluster_info[i].cend(); ++it)
                            {
                                Cluster cluster;
                                cluster.setIndices(it->second);
                                cluster.setName(it->first.c_str());
                                cluster.setColor(QColor(QString(it->first.c_str())));

                                clusterDataset[i]->addCluster(cluster);
                            }
                            processed[i] = 1;

                        }
                }

                // Notify others that the clusters have changed
                for (std::ptrdiff_t i = 0; i < items; ++i)
                    if (clusterDataset[i].isValid())
                        events().notifyDatasetDataChanged(clusterDataset[i]);

            }

        }
    }
}


// =============================================================================
// Factory
// =============================================================================

CsvLoaderFactory::CsvLoaderFactory()
{
    setIcon(util::StyledIcon(createPluginIcon("CSV")));
}

LoaderPlugin* CsvLoaderFactory::produce()
{
    return new CsvLoader(this);
}

mv::DataTypes CsvLoaderFactory::supportedDataTypes() const
{
    mv::DataTypes supportedTypes;
    supportedTypes.append(PointType);
    return supportedTypes;
}
