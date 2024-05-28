# ExtCsvLoader ![Build Status](https://github.com/ManiVaultStudio/ExtCsvLoader/actions/workflows/build.yml/badge.svg?branch=main)
CSV loader plugin for the [ManiVault](https://github.com/ManiVaultStudio/core) visual analytics framework.

```bash
git clone git@github.com:ManiVaultStudio/ExtCsvLoader.git
```

<p align="middle">
  <img src="https://github.com/ManiVaultStudio/ExtCsvLoader/assets/58806453/1477f7d8-adc5-4de4-9144-b02739b9f8cc" align="middle" width="70%" />
</p>

## How to use
- Either right-click an empty area in the data hierachy and select `Import` -> `Extended CSV Loader` or in the main menu, open `File` -> `Import data...` -> `Extended CSV Loader`. A file dialog will open and you can select a `.csv` file
- Specify the value seperator, e.g. the standard `,`
- If the loaded CSV file has column header (e.g. dimension names), toggle "Column headers" in the loader UI. Vice versa, if row headers (e.g. IDs) are present toggle "Row headers"
- Limitations:
  - Missing values are not supported
