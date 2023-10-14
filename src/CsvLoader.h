#pragma once

#include <LoaderPlugin.h>

#include <QFileDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>

using namespace mv::plugin;

// =============================================================================
// View
// =============================================================================

class CsvLoader : public LoaderPlugin
{
    Q_OBJECT

	QFileDialog _fileDialog;

    QLineEdit* _separatorLineEdit;
    QCheckBox* _columnHeaderCheckBox;
    QCheckBox* _rowHeaderCheckBox;
    QCheckBox* _transposeCheckBox;
    QCheckBox* _mixedDataHierarchyCheckbox;
    QComboBox* _sourceTypeComboBox;
    QComboBox* _storageTypeComboBox;

public:
    CsvLoader(const PluginFactory* factory) : LoaderPlugin(factory) { }
    ~CsvLoader(void) override;

    void init() override;

    void loadData() Q_DECL_OVERRIDE;

};


// =============================================================================
// Factory
// =============================================================================

class CsvLoaderFactory : public LoaderPluginFactory
{
    Q_INTERFACES(mv::plugin::LoaderPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "nl.tudelft.ExtCsvLoader"
                      FILE  "CsvLoader.json")

public:
    CsvLoaderFactory(void) {}
    ~CsvLoaderFactory(void) override {}

    /**
     * Get plugin icon
     * @param color Icon color for flat (font) icons
     * @return Icon
     */
    QIcon getIcon(const QColor& color = Qt::black) const override;

    /**
     * Produces the plugin
     * @return Pointer to the produced plugin
     */
    LoaderPlugin* produce() override;

    /**
     * Get the data types that the plugin supports
     * @return Supported data types
     */
    mv::DataTypes supportedDataTypes() const override;
};
